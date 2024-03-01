#include "options.h"
#include "osal.h"
#include <gtest/gtest.h>

#include "test_util.h"
#include "osal_spi.h"

class SPI_USB_Test : public TestBase
{
 protected:
   // Override default setup
   virtual void SetUp()
   {
      TestBase::SetUp(); // Re-use default setup
   };
};


TEST_F (SPI_USB_Test, spi_usb_calc_current_transfer_size_test){
    uint32_t max_allowed_return_value = UINT16_MAX + 1;

    /* Happy case */
    EXPECT_EQ(0, _iolink_calc_current_transfer_size(0, 0));
    EXPECT_EQ(54, _iolink_calc_current_transfer_size(64, 10));

    /* Too large already transferred */
    EXPECT_EQ(max_allowed_return_value, _iolink_calc_current_transfer_size(0, 1)); /*Illegal*/
    EXPECT_EQ(max_allowed_return_value, _iolink_calc_current_transfer_size(0, 2)); /*Illegal*/

    EXPECT_EQ(2, _iolink_calc_current_transfer_size(1000, 998));
    EXPECT_EQ(1, _iolink_calc_current_transfer_size(1000, 999));
    EXPECT_EQ(0, _iolink_calc_current_transfer_size(1000, 1000));
    EXPECT_EQ(max_allowed_return_value, _iolink_calc_current_transfer_size(1000, 1001)); /*Illegal*/
    EXPECT_EQ(max_allowed_return_value, _iolink_calc_current_transfer_size(1000, 1002)); /*Illegal*/

    /* Transfer sizes larger than allowed value */
    EXPECT_EQ(UINT16_MAX - 3, _iolink_calc_current_transfer_size(UINT16_MAX - 2, 1));
    EXPECT_EQ(UINT16_MAX - 2, _iolink_calc_current_transfer_size(UINT16_MAX - 1, 1));
    EXPECT_EQ(UINT16_MAX - 1, _iolink_calc_current_transfer_size(UINT16_MAX, 1));
    EXPECT_EQ(UINT16_MAX, _iolink_calc_current_transfer_size(UINT16_MAX + 1, 1));
    EXPECT_EQ(max_allowed_return_value, _iolink_calc_current_transfer_size(UINT16_MAX + 2, 1));
    EXPECT_EQ(max_allowed_return_value, _iolink_calc_current_transfer_size(UINT16_MAX + 2, 1));
}
