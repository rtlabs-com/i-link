#include <stdlib.h>
#include <inttypes.h>
#include "options.h"
#include "osal_spi.h"
#include "osal_log.h"
#include "ftd2xx.h"
#include "osal_spi_usb_helpers.h"
#include "string.h"

#define CLK_FREQUENCY            2.5 * 1000 * 1000
#define CMD_BUFF_SIZE            3
#define SET_CS_PIN_BUFF_SIZE     3
#define BUFF_SIZE                250
#define CLOCK_SETTINGS_BUFF_SIZE 4
#define PIN_SETUP_BUFF_SIZE      5
#define READ_TIMEOUT_MS          100
#define WRITE_TIMEOUT_MS         100
#define LATENCY                  1
#define DEFAULT_SLEEP_TIME       2

/* Specific commands and operations to the D2XX library */
#define MPSSE_CMD_SET_DATA_BITS_LOWBYTE          0x80
#define MPSSE_CMD_DISABLE_3PHASE_CLOCKING        0x8D
#define MPSSE_CMD_DATA_BYTES_IN_POS_OUT_NEG_EDGE 0x31
#define ENABLE_MPSSE                             0x02
#define MAX_CLOCK_RATE                           30000000
#define DISABLE_ADAPTIVE_CLOCKING                0x97
#define SET_CLOCK_FREQUENCY_CMD                  0x86
#define DISABLE_CLOCK_DIVIDE                     0x8A
#define SET_CS_LOW                               0x00
#define SET_CS_HIGH                              0x08
#define SET_GPIO_CMD                             0x80
#define SET_VALS                                 0x0A
#define SET_DIRECTION                            0x0B

os_mutex_t * ftdi_io_mutex;

/**
 * Set the Chip Select (CS) pin on the FT2232H chip.
 *
 * Since the CS signal is active low, a `true` state lowers the CS
 * signal and a `false` state raises the CS signal.
 *
 * The function logs an error if the write operation fails.
 *
 * @param ftdi_handle   In: A handle to the FTDI device.
 * @param state         In: The CS state (true for low, false for high)
 */
static void set_cs_pin (void * ftdi_handle, bool state)
{
   uint32_t status                   = 0;
   uint8_t buf[SET_CS_PIN_BUFF_SIZE] = {};
   uint32_t n_bytes_transferred      = 0;
   buf[0]                            = MPSSE_CMD_SET_DATA_BITS_LOWBYTE;

   if (state)
   {
      buf[1] = SET_CS_LOW;
   }
   else
   {
      buf[1] = SET_CS_HIGH;
   }

   buf[2] = SET_DIRECTION;
   /*Send MPSSE command to set the bits in lowbyte*/
   status =
      FT_Write (ftdi_handle, buf, SET_CS_PIN_BUFF_SIZE, &n_bytes_transferred);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "APP: %s: Failed to set_cs_pin\n", __func__);
      return;
   }
}

/**
 * Configures the FT2232H chip for MPSSE mode, setting timeouts, latency, flow
 * control, and clock frequency.
 *
 * This function utilizes the ftd2xx API to handle device configuration. In case
 * of failure, it logs an error and returns an appropriate status message, which
 * should be handled accordingly.
 *
 * @param ftdi_handle   In: A handle to the FTDI device.
 * @param clk_freq      In: The clock frequency, with a maximum value of
 *                          30,000,000.
 * @return 0 for success; otherwise, check the FT_STATUS enum in ftd2xx.h for
 * state values.
 */
static uint32_t cfg_ftdi_prt (void * ftdi_handle, uint32_t clk_freq)
{
   uint8_t tx_buf[CLOCK_SETTINGS_BUFF_SIZE] = {};
   uint8_t val_L                            = 0;
   uint8_t val_H                            = 0;
   uint32_t status                          = 0;
   uint32_t n_bytes_written                 = 0;
   uint32_t val                             = 0;

   status = FT_ResetDevice (ftdi_handle);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "APP: %s: Failed to reset the device\n", __func__);
      return status;
   }

   status = FT_SetTimeouts (ftdi_handle, READ_TIMEOUT_MS, WRITE_TIMEOUT_MS);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "APP: %s: Failed to set timeouts\n", __func__);
      return status;
   }

   status = FT_SetLatencyTimer (ftdi_handle, LATENCY);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "APP: %s: Failed to set latency timer\n", __func__);
      return status;
   }

   status = FT_SetFlowControl (ftdi_handle, FT_FLOW_NONE, 0, 0);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "APP: %s: Failed to set flow control\n", __func__);
      return status;
   }

   status = FT_SetBitMode (ftdi_handle, 0x00, 0x00);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "APP: %s: Failed to reset bit mode\n", __func__);
      return status;
   }

   status = FT_SetBitMode (ftdi_handle, 0x00, ENABLE_MPSSE);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "APP: %s: Failed to set bit mode\n", __func__);
      return status;
   }

   val       = (MAX_CLOCK_RATE / clk_freq) - 1;
   val_L     = (uint8_t)val;
   val_H     = (uint8_t)(val >> 8);
   tx_buf[0] = DISABLE_CLOCK_DIVIDE;
   tx_buf[1] = SET_CLOCK_FREQUENCY_CMD;
   tx_buf[2] = val_L;
   tx_buf[3] = val_H;
   /*Send command to set clock frequency*/
   status =
      FT_Write (ftdi_handle, tx_buf, CLOCK_SETTINGS_BUFF_SIZE, &n_bytes_written);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "APP: %s: Failed to set clock frequency\n", __func__);
      return status;
   }

   return status;
}

/**
 * This function sets up the pins of the FT2232H chip
 * to act as a SPI Master and a pin for the IRQ.
 *
 * The pins of the FT2232H chip is setup as followed:
 * - ADBUS0 (SCLK) - Output, Clock Idle Low
 * - ADBUS1 (MOSI) - Output, Default Low
 * - ADBUS2 (MISO) - Input
 * - ADBUS3 (CS) -  Output, Default High
 * - ADBUS4 (IRQ) - Input
 *
 * The 3 phase clocking and adaptive clocking is also disabled.
 *
 * @param ftdi_handle   In: A handle to the FTDI device.
 * @return              0 for success; otherwise, check the FT_STATUS enum in
 * ftd2xx.h for state values.
 */
static uint32_t mpsse_setup (void * ftdi_handle)
{
   uint32_t status                     = 0;
   uint8_t tx_buf[PIN_SETUP_BUFF_SIZE] = {};
   uint32_t n_bytes_to_transfer        = 0;
   uint32_t n_bytes_written            = 0;
   tx_buf[0]                           = MPSSE_CMD_DISABLE_3PHASE_CLOCKING;
   tx_buf[1]                           = DISABLE_ADAPTIVE_CLOCKING;
   tx_buf[2]                           = SET_GPIO_CMD;
   tx_buf[3]                           = SET_VALS;
   tx_buf[4]                           = SET_DIRECTION;
   /*Send command and options to set up FTDI to handle SPI and ABUS4 (IRQ-pin)
    * to GPIO*/
   status = FT_Write (ftdi_handle, tx_buf, PIN_SETUP_BUFF_SIZE, &n_bytes_written);
   if (status != FT_OK)
   {
      LOG_ERROR (
         IOLINK_APP_LOG,
         "APP: %s: Failed to configure pin directions\n",
         __func__);
      return status;
   }

   os_usleep (DEFAULT_SLEEP_TIME);
   return status;
}

void * _iolink_pl_hw_spi_init (const char * spi_slave_name)
{
   ftdi_io_mutex = os_mutex_create();
   void * ftdi_handle;
   uint32_t status = 0;

   /* TODO: This function supports opening multiple devices by setting
      deviceNumber to 0, 1, etc. However, it does not provide the capability to
      open a specific device by name. For opening named devices, consider using
      FT_OpenEx instead.
   */
   status = FT_Open (atoi (spi_slave_name), &ftdi_handle);
   if (status != FT_OK)
   {
      LOG_ERROR (IOLINK_APP_LOG, "APP: %s: Failed to open device\n", __func__);
      os_mutex_destroy (ftdi_io_mutex);
      return NULL;
   }

   status = cfg_ftdi_prt (ftdi_handle, CLK_FREQUENCY);
   if (status != FT_OK)
   {
      LOG_ERROR (IOLINK_APP_LOG, "APP: %s: Failed to config port\n", __func__);
      os_mutex_destroy (ftdi_io_mutex);
      return NULL;
   }

   status = mpsse_setup (ftdi_handle);
   if (status != FT_OK)
   {
      LOG_ERROR (IOLINK_APP_LOG, "APP: %s: Failed to to setup MPSSE\n", __func__);
      os_mutex_destroy (ftdi_io_mutex);
      return NULL;
   }

   return ftdi_handle;
}

void _iolink_pl_hw_spi_close (void * ftdi_handle)
{
   if (FT_Close (ftdi_handle) != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "APP: %s: Failed to close handle\n", __func__);
      return;
   }

   os_mutex_destroy (ftdi_io_mutex);
}

void _iolink_pl_hw_spi_transfer (
   void * ftdi_handle,
   void * data_read,
   const void * data_written,
   size_t n_bytes_to_transfer)
{
   uint32_t status                = 0;
   uint8_t cmd_buf[CMD_BUFF_SIZE] = {};
   uint32_t n_bytes_written       = 0;
   uint32_t current_transfer_size = 0;
   uint32_t n_bytes_transferred   = 0;
   uint32_t n_bytes_read          = 0;

   os_mutex_lock (ftdi_io_mutex);
   set_cs_pin (ftdi_handle, TRUE);

   while (n_bytes_transferred < (uint32_t)n_bytes_to_transfer)
   {
      current_transfer_size = _iolink_calc_current_transfer_size (
         (uint32_t)n_bytes_to_transfer,
         n_bytes_transferred);
      cmd_buf[0] = MPSSE_CMD_DATA_BYTES_IN_POS_OUT_NEG_EDGE;
      /* length LSB */
      cmd_buf[1] = (uint8_t)((current_transfer_size - 1) & 0x000000FF);
      /* length MSB */
      cmd_buf[2] = (uint8_t)(((current_transfer_size - 1) & 0x0000FF00) >> 8);

      n_bytes_written = 0;
      /*Send the command buffer to prepare for read and write operation*/
      status = FT_Write (ftdi_handle, cmd_buf, CMD_BUFF_SIZE, &n_bytes_written);
      if (status != FT_OK)
      {
         LOG_ERROR (LOG_STATE_ON, "%s: failed to send SPI message\n", "");
         return;
      }

      n_bytes_written = 0;
      /*Write data*/
      status = FT_Write (
         ftdi_handle,
         &((uint8_t *)data_written)[n_bytes_transferred],
         current_transfer_size,
         &n_bytes_written);
      if (status != FT_OK)
      {
         LOG_ERROR (LOG_STATE_ON, "%s: failed to send SPI message\n", __func__);
         return;
      }

      n_bytes_read = 0;
      /*Read data*/
      status = FT_Read (
         ftdi_handle,
         &((uint8_t *)data_read)[n_bytes_transferred],
         n_bytes_written,
         &n_bytes_read);
      if (status != FT_OK)
      {
         LOG_ERROR (LOG_STATE_ON, "%s: failed to send SPI message\n", __func__);
         return;
      }

      /*Should never happen*/
      if (n_bytes_read != n_bytes_written)
      {
         LOG_ERROR (
            LOG_STATE_ON,
            "%s: Not the same amount written as read\n",
            __func__);
         return;
      }

      n_bytes_transferred += n_bytes_read;
   }
   set_cs_pin (ftdi_handle, FALSE);
   os_mutex_unlock (ftdi_io_mutex);
   return;
}
