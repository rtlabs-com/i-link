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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF        64

typedef void (*isr_func_t) (int pin);

typedef struct
{
   int fd;
   int gpio;
   isr_func_t isr_func;
   pthread_mutex_t mtx;
} irq_thread_t;

void isr_func (int pin)
{
   printf ("\n%s: GPIO pin %d interrupt occurred\n", __func__, pin);
}

void * irq_thread (void * arg)
{
   irq_thread_t * irq = (irq_thread_t *)arg;
   struct pollfd p_fd;
   char buf[MAX_BUF];

   while (true)
   {
      memset (&p_fd, 0, sizeof (p_fd));

      p_fd.fd     = irq->fd;
      p_fd.events = POLLPRI;

      int rc = poll (&p_fd, 1, 5000);

      if (rc < 0)
      {
         printf ("\npoll() failed!\n");
      }
      else if (rc == 0)
      {
         printf (".");
      }
      else if (p_fd.revents & POLLPRI)
      {
         lseek (p_fd.fd, 0, SEEK_SET);
         int len = read (p_fd.fd, buf, MAX_BUF);
         (void)len;
         irq->isr_func (irq->gpio);
      }

      fflush (stdout);
   }

   close (irq->fd);
}

pthread_t * setup_int (unsigned int gpio_pin, isr_func_t isr_func)
{
   char buf[MAX_BUF];
   pthread_attr_t attr;
   static pthread_t thread;
   static irq_thread_t irqarg;

   // Add gpio_pin to exported pins
   int fd = open (SYSFS_GPIO_DIR "/export", O_WRONLY);

   if (fd < 0)
   {
      perror ("gpio/export");
      return NULL;
   }

   int n = write (fd, buf, snprintf (buf, sizeof (buf), "%d", gpio_pin));
   close (fd);

   if (n < 0)
   {
      perror ("setup_int(): Failed to write gpio_pin to gpio/export");
   }

   // Set direction of pin to input
   snprintf (buf, sizeof (buf), SYSFS_GPIO_DIR "/gpio%d/direction", gpio_pin);
   fd = open (buf, O_WRONLY);

   if (fd < 0)
   {
      perror ("gpio/direction");
      return NULL;
   }

   n = write (fd, "in", sizeof ("in") + 1);
   close (fd);

   if (n < 0)
   {
      perror ("write direction");
      return NULL;
   }

   // Set edge detection to falling
   snprintf (buf, sizeof (buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio_pin);
   fd = open (buf, O_WRONLY);

   if (fd < 0)
   {
      perror ("gpio/edge");
      return NULL;
   }

   n = write (fd, "falling", sizeof ("falling") + 1);
   close (fd);

   if (n < 0)
   {
      perror ("write edge");
      return NULL;
   }

   // Open file and return file descriptor
   snprintf (buf, sizeof (buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio_pin);
   fd = open (buf, O_RDONLY | O_NONBLOCK);

   if (fd < 0)
   {
      perror ("gpio/value");
      return NULL;
   }

   // Create isr service thread
   irqarg.isr_func = isr_func;
   irqarg.fd       = fd;
   irqarg.gpio     = gpio_pin;
   pthread_attr_init (&attr);
   pthread_create (&thread, &attr, irq_thread, &irqarg);

   return &thread;
}

/****************************************************************
 * Main
 ****************************************************************/
int main (int argc, char ** argv)
{
   if (argc < 2)
   {
      printf ("Usage: %s <gpio-pin>\n\n", argv[0]);
      printf ("Waits for a change in the GPIO pin voltage level or input on "
              "stdin\n");
      exit (-1);
   }

   setup_int (atoi (argv[1]), isr_func);

   while (true)
   {
   }

   return 0;
}
