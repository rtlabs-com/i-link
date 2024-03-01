#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include "osal_spi.h"
#include "osal_log.h"
#include "options.h"
#include "fcntl.h"
#include "unistd.h"

void * _iolink_pl_hw_spi_init (const char * spi_slave_name)
{
   int fd = -1;
   fd     = open (spi_slave_name, O_RDWR);
   if (fd == -1)
   {
      return NULL;
   }
   return (void *)fd;
}

void _iolink_pl_hw_spi_close (void * fd)
{
   close ((int)fd);
}

void _iolink_pl_hw_spi_transfer (
   void * fd,
   void * data_read,
   const void * data_written,
   size_t n_bytes_to_transfer)
{
   int spi_fd = (int)fd;
   // TODO
   int delay = 100;
   int speed = 4 * 1000 * 1000;
   int bits  = 8;

   struct spi_ioc_transfer tr = {
      .tx_buf        = (unsigned long)data_written,
      .rx_buf        = (unsigned long)data_read,
      .len           = n_bytes_to_transfer,
      .delay_usecs   = delay,
      .speed_hz      = speed,
      .bits_per_word = bits,
   };

   if (ioctl (spi_fd, SPI_IOC_MESSAGE (1), &tr) < 1)
   {
      LOG_ERROR (IOLINK_PL_LOG, "%s: failed to send SPI message\n", __func__);
   }
}
