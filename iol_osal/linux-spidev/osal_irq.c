#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>

#include "osal_irq.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF        64

typedef struct
{
   void* irq_arg;
   int irq_fd;
   isr_func_t isr_func;
} irq_thread_t;

static void * irq_thread (void * arg)
{
   irq_thread_t * irq = (irq_thread_t *)arg;
   struct pollfd p_fd;
   char buf[MAX_BUF];

   while (1)
   {
      memset (&p_fd, 0, sizeof (p_fd));

      p_fd.fd     = irq->irq_fd;
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
         if (read (p_fd.fd, buf, MAX_BUF) > 0)
         {
            irq->isr_func (irq->irq_arg);
         }
      }

      fflush (stdout);
   }
   close (irq->irq_fd);
}

static pthread_t thread;

int _iolink_setup_int (int gpio_pin, isr_func_t isr_func, void* irq_arg)
{
   char buf[MAX_BUF];
   pthread_attr_t attr;
   static irq_thread_t irqarg;

   // Add gpio_pin to exported pins
   int fd = open (SYSFS_GPIO_DIR "/export", O_WRONLY);

   if (fd < 0)
   {
      perror ("gpio/export");
      return -1;
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
      return -1;
   }

   n = write (fd, "in", sizeof ("in"));
   close (fd);

   if (n < 0)
   {
      perror ("write direction");
      return -1;
   }

   // Set edge detection to falling
   snprintf (buf, sizeof (buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio_pin);
   fd = open (buf, O_WRONLY);

   if (fd < 0)
   {
      perror ("gpio/edge");
      return -1;
   }

   n = write (fd, "falling", sizeof ("falling"));
   close (fd);

   if (n < 0)
   {
      perror ("write edge");
      return -1;
   }

   // Open file and return file descriptor
   snprintf (buf, sizeof (buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio_pin);
   fd = open (buf, O_RDONLY | O_NONBLOCK);

   if (fd < 0)
   {
      perror ("gpio/value");
      return -1;
   }

   // Create isr service thread
   irqarg.isr_func = isr_func;
   irqarg.irq_arg  = irq_arg;
   irqarg.irq_fd   = fd;
   pthread_attr_init (&attr);
   pthread_create (&thread, &attr, irq_thread, &irqarg);

   return 0;
}
