/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>

#include "osal.h"
#include "iolink.h"
#include "iolink_max14819.h"
#include "iolink_handler.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

#define IOLINK_MASTER_THREAD_STACK_SIZE  (4 * 1024)
#define IOLINK_MASTER_THREAD_PRIO         6
#define IOLINK_DL_THREAD_STACK_SIZE       1500
#define IOLINK_DL_THREAD_PRIO            (IOLINK_MASTER_THREAD_PRIO + 1)

typedef void(*isr_func_t)(void *);

typedef struct
{
   int spi_fd;
   int irq_fd;
   isr_func_t isr_func;
} irq_thread_t;

void *irq_thread(void *arg)
{
   irq_thread_t *irq = (irq_thread_t *)arg;
   struct pollfd p_fd;
   char buf[MAX_BUF];

   while (true)
   {
      memset(&p_fd, 0, sizeof(p_fd));

      p_fd.fd = irq->irq_fd;
      p_fd.events = POLLPRI;

      int rc = poll(&p_fd, 1, 5000);

      if (rc < 0)
	  {
         printf("\npoll() failed!\n");
      }
      else if (rc == 0)
      {
         printf(".");
      }
      else if (p_fd.revents & POLLPRI)
      {
         lseek(p_fd.fd, 0, SEEK_SET);
         if (read(p_fd.fd, buf, MAX_BUF) > 0)
		 {
            irq->isr_func(&irq->spi_fd);
		 }
      }

      fflush(stdout);
  }
  close (irq->irq_fd);
}

static pthread_t thread;

pthread_t *setup_int(unsigned int gpio_pin, isr_func_t isr_func, int spi_fd)
{
   char buf[MAX_BUF];
   pthread_attr_t attr;
   static irq_thread_t irqarg;

   // Add gpio_pin to exported pins
   int fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);

   if (fd < 0)
   {
      perror("gpio/export");
      return NULL;
   }

   int n = write(fd, buf, snprintf(buf, sizeof(buf), "%d", gpio_pin));
   close(fd);

   if (n < 0)
   {
      perror("setup_int(): Failed to write gpio_pin to gpio/export");
   }

   // Set direction of pin to input
   snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/direction", gpio_pin);
   fd = open(buf, O_WRONLY);

   if (fd < 0)
   {
      perror("gpio/direction");
      return NULL;
   }

   n = write(fd, "in", sizeof("in") + 1);
   close(fd);

   if (n < 0)
   {
      perror("write direction");
      return NULL;
   }

   // Set edge detection to falling
   snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio_pin);
   fd = open(buf, O_WRONLY);

   if (fd < 0)
   {
      perror("gpio/edge");
      return NULL;
   }

   n = write(fd, "falling", sizeof("falling") + 1);
   close(fd);

   if (n < 0)
   {
      perror("write edge");
      return NULL;
   }

   // Open file and return file descriptor
   snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio_pin);
   fd = open(buf, O_RDONLY | O_NONBLOCK);

   if (fd < 0)
   {
      perror("gpio/value");
   }

   // Create isr service thread
   irqarg.isr_func = isr_func;
   irqarg.spi_fd = spi_fd;
   irqarg.irq_fd = fd;
   pthread_attr_init(&attr);
   pthread_create(&thread, &attr, irq_thread, &irqarg);

   return &thread;
}

static iolink_pl_mode_t mode_ch_0 = iolink_mode_SDCI;
static iolink_pl_mode_t mode_ch_1 = iolink_mode_INACTIVE;

int main (int argc, char **argv)
{
   iolink_14819_cfg_t iol_14819_cfg =
   {
         .chip_address   = 0x00,
         .spi_slave_name = "/dev/spidev0.0",
         .CQCfgA         = 0x22,
         .LPCnfgA        = 0x01,
         .IOStCfgA       = 0x05,
         .DrvCurrLim     = 0x00,
         .Clock          = 0x01,
         .pin            = {0, 0},
   };

   iolink_port_cfg_t port_cfgs [] =
   {
      {
         .name = "/dev/spidev0.0/0",
         .mode = &mode_ch_0,
      },
      {
         .name = "/dev/spidev0.0/1",
         .mode = &mode_ch_1,
      },
   };

   iolink_m_cfg_t iolink_cfg =
   {
      .cb_arg = NULL,
      .cb_smi = NULL,
      .cb_pd = NULL,
      .port_cnt = NELEMENTS (port_cfgs),
      .port_cfgs = port_cfgs,
      .master_thread_prio = IOLINK_MASTER_THREAD_PRIO,
      .master_thread_stack_size = IOLINK_MASTER_THREAD_STACK_SIZE,
      .dl_thread_prio = IOLINK_DL_THREAD_PRIO,
      .dl_thread_stack_size = IOLINK_DL_THREAD_STACK_SIZE,
   };

   int fd = iolink_14819_init ("iolink", &iol_14819_cfg);
   if (fd < 0)
   {
      printf("Failed to open SPI\n");
      return -1;
   }

   if (setup_int(25, iolink_14819_isr, fd) == NULL)
   {
      printf("Failed to setup interrupt\n");
      return -1;
   }

   usleep(200 * 1000);

   iolink_handler (iolink_cfg);

   return 0;
}
