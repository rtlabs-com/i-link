#include "osal_spi.h"

#include <drivers/spi/spi.h>

void _iolink_pl_hw_spi_transfer (
   int fd,
   void * data_read,
   const void * data_written,
   size_t n_bytes_to_transfer)
{
   spi_select (fd);
   spi_bidirectionally_transfer (fd, data_read, data_written, n_bytes_to_transfer);
   spi_unselect (fd);
}
