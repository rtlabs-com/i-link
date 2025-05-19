#include "iolink_options.h"
#include "osal.h"
#include <gtest/gtest.h>

#include "test_util.h"
#include "osal_spi.h"

extern "C" uint32_t _iolink_calc_current_transfer_size (
   uint32_t n_bytes_to_transfer,
   uint32_t n_bytes_transferred);

class SPI_USB_Test : public TestBase
{
 protected:
   // Override default setup
   virtual void SetUp()
   {
      TestBase::SetUp(); // Re-use default setup
   };
};
