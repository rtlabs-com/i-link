#include "osal_spi.h"

#include <drivers/spi/spi.h>

void * _iolink_pl_hw_spi_init (const char * spi_slave_name)
{
   int fd = -1;
   fd = open(spi_slave_name, O_RDWR);
   if (fd == -1)
   {
      return NULL;
   }
   return (void*)fd;
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
   spi_select ((int)fd);
   spi_bidirectionally_transfer ((int)fd, data_read, data_written, n_bytes_to_transfer);
   spi_unselect ((int)fd);
}
