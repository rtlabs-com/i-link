#include <stdlib.h>
#include <inttypes.h>
#include "osal.h"
#include "osal_log.h"
/* Needed for hardware driver (iolink_14819_drv_t) */
#include "iolink_max14819_pl.h"
#include "ftd2xx.h"
#include "osal_spi_usb_helpers.h"

#include "osal_irq.h"

#define IRQ_BIT               (1 << 4)
#define IRQ_THREAD_SLEEP_TIME 500
#define RX_BUFFER_SIZE        1
#define TX_BUFFER_SIZE        2
#define THREAD_STACK_SIZE     128
#define THREAD_PRIORITY       6

/* These defines represent hex values for specific commands and operations
   from the D2XX library */
#define MPSSE_CMD_GET_DATA_BITS_LOWBYTE 0x81
#define MPSSE_CMD_SEND_IMMEDIATE        0x87

typedef struct
{
   void * fthandle;
   isr_func_t isr_func;
   void * irq_arg;
} usb_irq_thread_t;

/**
 * This function returns the state of the IRQ pin by sending a read command
 * to the FTDI chip which returns the state of the lowbyte pins (ADBUS0 -
 * ADBUS7). The IRQ Pin is connected to ADBUS4.
 *
 * If the writing to the device fails, the function will issue a LOG_ERROR and
 * return false.
 *
 * @param ftdi_handle  In: A handle to the FTDI device.
 * @return true if the pin is low, else high
 */
static bool read_irq (void * ftdi_handle)
{
   uint32_t status                = 0;
   uint32_t n_bytes_transferred   = 0;
   uint8_t rx_buf[RX_BUFFER_SIZE] = {};
   uint8_t tx_buf[TX_BUFFER_SIZE] = {};

   /* Send MPSSE command to read status of bits in the lowbyte */
   tx_buf[0] = MPSSE_CMD_GET_DATA_BITS_LOWBYTE;
   tx_buf[1] = MPSSE_CMD_SEND_IMMEDIATE;

   status = FT_Write (ftdi_handle, tx_buf, TX_BUFFER_SIZE, &n_bytes_transferred);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "%s: FT_Write failed: %d\n", __func__, status);
      return false;
   }

   n_bytes_transferred = 0;
   /* Read one byte from the FTDI buffer to get the status of the lowbyte */
   status = FT_Read (ftdi_handle, rx_buf, RX_BUFFER_SIZE, &n_bytes_transferred);
   if (status != FT_OK)
   {
      LOG_ERROR (LOG_STATE_ON, "%s: FT_Read failed: %d\n", __func__, status);
      return false;
   }

   /* IRQ pin is active low*/
   if (rx_buf[0] & IRQ_BIT)
   {
      return false;
   }
   else
   {
      return true;
   }
}

/**
 * This function runs in a separate thread and continuously monitors the ADBUS4
 * pin on an FT2232H, which is connected to the IRQ pin on an MAX14819. It uses
 * a mutex to ensure exclusive access to the FT2232H chip during the IRQ check.
 * If an IRQ is detected, it triggers the associated ISR function provided
 * during thread initialization.
 *
 * @param arg  In: Pointer to the usb_irq_thread_t structure containing thread
 * parameter
 *
 * @note The function uses a loop to periodically check for IRQ with the time
 * interval (IRQ_THREAD_SLEEP_TIME) between checks.
 * @note ensure the ftdi_io_mutex is properly initiated before starting this
 * thread.
 */
static void irq_thread (void * arg)
{
   usb_irq_thread_t * irq = (usb_irq_thread_t *)arg;
   bool irq_flag          = false;
   while (1)
   {
      os_mutex_lock (ftdi_io_mutex);
      irq_flag = read_irq (irq->fthandle);
      os_mutex_unlock (ftdi_io_mutex);

      if (irq_flag)
      {
         irq->isr_func (irq->irq_arg);
      }

      os_usleep (IRQ_THREAD_SLEEP_TIME);
   }
}

int _iolink_setup_int (int gpio_pin, isr_func_t isr_func, void * irq_arg)
{
   iolink_14819_drv_t * usb_drv = (iolink_14819_drv_t *)irq_arg;
   static usb_irq_thread_t irqarg;

   irqarg.isr_func = isr_func;
   irqarg.irq_arg  = irq_arg;
   irqarg.fthandle = usb_drv->fd_spi;

   os_thread_t * isr_service_thread;
   isr_service_thread = os_thread_create (
      "isr_service_thread",
      THREAD_PRIORITY,
      THREAD_STACK_SIZE,
      irq_thread,
      &irqarg);
   return 0;
}
