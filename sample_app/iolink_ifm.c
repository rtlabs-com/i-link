/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * http://www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 * See LICENSE file in the project root for full license information.
 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include "osal.h"
#include "osal_log.h"
#include "iolink.h"
#include "iolink_handler.h"
#include "iolink_smi.h"

#define RFID_DATA_SIZE     8

static bool ifmrfid_get_tag_present (uint8_t * data)
{
   return (data[1] & 0x04);
}

static uint8_t ifmrfid_get_error_code (uint8_t * data)
{
   return data[31];
}

static bool is_all_zeroes (uint8_t * data, int length)
{
   int i;

   for (i = 0; i < length; i++)
   {
      if (data[i] != 0)
      {
         return false;
      }
   }

   return true;
}

static void ifmrfid_run (iolink_app_port_ctx_t * app_port)
{
   uint8_t port_index = app_port->portnumber - 1;
   uint8_t pdata[IOLINK_NUM_PORTS][IOLINK_PD_MAX_SIZE];
   static bool tag_is_present[IOLINK_NUM_PORTS] = { false };

   if (do_smi_pdin (app_port, NULL, pdata[port_index]) != IOLINK_PD_MAX_SIZE)
   {
      LOG_WARNING(LOG_STATE_ON, "%s: Failed to get PDIn (RFID port #%u)\n",
                  __func__, app_port->portnumber);
      return;
   }

   uint8_t error_code = ifmrfid_get_error_code(pdata[port_index]);
   if (error_code != 0)
   {
      LOG_ERROR(LOG_STATE_ON, "%s: Got error code %d (RFID port #%u)\n",
                __func__, error_code, app_port->portnumber);
      return;
   }

   if (ifmrfid_get_tag_present(pdata[port_index]))
   {
      if (!tag_is_present[port_index])  // tagPresent transition from 0 to 1
      {
         // check if UID is invalid, i.e. all zeroes (data starts at byte 2)
         if (is_all_zeroes(&pdata[port_index][2], RFID_DATA_SIZE))
         {
            LOG_ERROR(LOG_STATE_ON, "%s: Got tag_present with UID = 0 (RFID port #%u)\n",
                      __func__, app_port->portnumber);
            return;
         }

         tag_is_present[port_index] = true;
         char UUID[30];
         char *p = UUID;
         int i;
         for (i = 0; i < RFID_DATA_SIZE; i++)
         {
            p += sprintf (p, "%02X", pdata[port_index][2 + i]);
         }
         LOG_INFO(LOG_STATE_ON, "Port %u: RFID Tag data: %s\n", app_port->portnumber, UUID);
      }
   }
   else if (tag_is_present[port_index])  // tagPresent transition from 1 to 0
   {
      tag_is_present[port_index] = false;
      LOG_INFO(LOG_STATE_ON, "Port %u: RFID Tag not present\n", app_port->portnumber);
   }
}

void ifmrfid_setup (iolink_app_port_ctx_t * app_port)
{
   uint8_t data[2] = {0, 0};

   LOG_INFO(LOG_STATE_ON, "Set up RFID on port %u\n", app_port->portnumber);

   // Blocksize 4
   data[0] = 4;
   if (do_smi_device_write (app_port, 1900, 0, 1, data) != IOLINK_ERROR_NONE)
   {
      LOG_ERROR(LOG_STATE_ON, "Set up RFID blocksize failed on port %u\n",
                app_port->portnumber);
      return;
   }

   // Data order normal
   data[0] = 0;
   if (do_smi_device_write (app_port, 1901, 0, 1, data) != IOLINK_ERROR_NONE)
   {
      LOG_ERROR(LOG_STATE_ON, "Set up RFID data order failed on port %u\n",
                app_port->portnumber);
      return;
   }

   // Data hold time
   data[0] = 0;
   data[1] = 50;
   if (do_smi_device_write (app_port, 1902, 0, 2, data) != IOLINK_ERROR_NONE)
   {
      LOG_ERROR(LOG_STATE_ON, "Set up RFID data hold failed on port %u\n",
                app_port->portnumber);
      return;
   }

   // Auto read/write length
   data[0] = 4;
   if (do_smi_device_write (app_port, 1904, 0, 1, data) != IOLINK_ERROR_NONE)
   {
      LOG_ERROR(LOG_STATE_ON, "Set up RFID auto read/write failed on port %u\n",
                app_port->portnumber);
      return;
   }

   app_port->type = IFM_RFID;

   app_port->run_function = ifmrfid_run;

   app_port->app_port_state = IOL_STATE_RUNNING;
}

// ifmMHI Process data offsets
#define VALUE_LINE_1_MSB   0
#define VALUE_LINE_1_LSB   1
#define VALUE_LINE_2_MSB   2
#define VALUE_LINE_2_LSB   3
#define COLOR_LINE_1       8
#define COLOR_LINE_2       9
#define LEDS               12
#define LAYOUT             13

// ifmMHI Process data values
#define DOWN_KEY_PRESSED   BIT(0)
#define UP_KEY_PRESSED     BIT(1)
#define ENTER_KEY_PRESSED  BIT(2)
#define ESC_KEY_PRESSED    BIT(3)
#define BLACK_WHITE        0
#define RED                1
#define GREEN              2
#define YELLOW             3
#define BLINK              40
#define DISPLAY_TEXT       80
#define LED_1_ON           BIT(0)
#define LED_1_OFF          0
#define LED_2_ON           BIT(2)
#define LED_2_OFF          0
#define ONE_LINE           1
#define TWO_LINES          2
#define TEXT_ID(n)        (n)

#define IFM_MHI_PD_SIZE    16

static void ifmHMI_run (iolink_app_port_ctx_t * app_port)
{
   static uint32_t itr = 0;
   static uint8_t prev_hmi_pdin = 0;
   static uint8_t hmi_pdin = 0;
   bool pdin_valid = false;

   itr++;

   if ((itr % 100) == 0)
   {
      static uint8_t addr = 0;
      static uint8_t data[12] = {'A', ' ', ' ', ' ', ' ', ' ',
                                 ' ', ' ', ' ', ' ', ' ', ' ', };
      static uint8_t i = 0;

      /* 90 or 91, description of line 1 and 2 */
      uint16_t index = 90 + (addr % 2);

      if (do_smi_device_write (app_port, index, 0, sizeof(data), data) != IOLINK_ERROR_NONE)
      {
         LOG_WARNING(LOG_STATE_ON, "%s: Failed to write on port %u\n",
                     __func__, app_port->portnumber);
      }

      if (data[i] == 'Z')
      {
         i++;
         i = i % 12;
         data[i] = 'A';
      }
      else
      {
         data[i]++;
      }
      addr++;
   }

   if (do_smi_pdin (app_port, &pdin_valid, &hmi_pdin) != sizeof(hmi_pdin) != IOLINK_ERROR_NONE)
   {
      LOG_WARNING(LOG_STATE_ON, "%s: Failed to get PDIn from port %u\n",
                  __func__, app_port->portnumber);
   }
   else
   {
      if (!pdin_valid)
      {
         LOG_WARNING(LOG_STATE_ON, "%s: PDIn data is invalid for port %u\n",
                     __func__, app_port->portnumber);
      }
      else
      {
         if (prev_hmi_pdin != hmi_pdin)
         {
            LOG_INFO(LOG_STATE_ON, "\nUp: %u\nDown: %u\nEnter: %u\nEsc: %u\n\n",
                     hmi_pdin & UP_KEY_PRESSED ? 1 : 0,
                     hmi_pdin & DOWN_KEY_PRESSED ? 1 : 0,
                     hmi_pdin & ENTER_KEY_PRESSED ? 1 : 0,
                     hmi_pdin & ESC_KEY_PRESSED ? 1 : 0);
            prev_hmi_pdin = hmi_pdin;
         }
      }
   }

   if ((itr % 512) == 0)
   {
      static bool show_text = false;

      uint8_t data[IFM_MHI_PD_SIZE];
      bzero (data, sizeof(data));

      if (show_text)
      {
         data[VALUE_LINE_1_LSB] = TEXT_ID(4);
         data[COLOR_LINE_1]     = YELLOW | DISPLAY_TEXT;
         data[LEDS]             = LED_1_OFF | LED_2_ON;
         data[LAYOUT]           = ONE_LINE;
      }
      else
      {
		 uint32_t val = itr - 512;
         data[VALUE_LINE_1_MSB] = val >> 8;
         data[VALUE_LINE_1_LSB] = val;
         data[VALUE_LINE_2_MSB] = val >> 18;
         data[VALUE_LINE_2_LSB] = val >> 10;
         data[COLOR_LINE_1]     = GREEN;
         data[COLOR_LINE_2]     = RED | BLINK;
         data[LEDS]             = LED_1_ON | LED_2_OFF;
         data[LAYOUT]           = TWO_LINES;
      }

      do_smi_pdout (app_port, true, sizeof(data), data);
      show_text = !show_text;
   }

   if ((itr % 2048) == 0)
   {
      if (do_smi_device_read (app_port, 90, 0, 17, NULL, NULL) != IOLINK_ERROR_NONE)
      {
         LOG_WARNING(LOG_STATE_ON, "%s: Failed to read from port %u\n",
                     __func__, app_port->portnumber);
      }
   }

   if ((itr % 8192) == 0)
   {
      if (do_smi_pdinout (app_port) != IOLINK_ERROR_NONE)
      {
         LOG_WARNING(LOG_STATE_ON, "%s: PDInOut failed on port %u\n",
                     __func__, app_port->portnumber);
      }
   }
}

void ifmHMI_setup (iolink_app_port_ctx_t * app_port)
{
   uint8_t data[IFM_MHI_PD_SIZE];
   bzero (data, sizeof(data));

   LOG_INFO(LOG_STATE_ON, "Set up HMI on port %u\n", app_port->portnumber);

   data[VALUE_LINE_1_LSB] = TEXT_ID(3);
   data[COLOR_LINE_1]     = BLACK_WHITE | DISPLAY_TEXT;
   data[LEDS]             = LED_1_ON | LED_2_OFF;
   data[LAYOUT]           = TWO_LINES;

   do_smi_pdout (app_port, true, sizeof(data), data);

   app_port->type = IFM_HMI;

   app_port->run_function = ifmHMI_run;

   app_port->app_port_state = IOL_STATE_RUNNING;
}
