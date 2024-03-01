#include <inttypes.h>

/**
 * Calculate transfer size.
 *
 * If the difference between n_bytes_to_transfer and n_bytes_transferred is
 * greater than (UINT16_MAX + 1), (UINT16_MAX + 1) will be returned. Otherwise,
 * the difference is returned.
 *
 * @param n_bytes_transfer     In: The number of bytes to be transferred.
 * @param n_bytes_transferred  In: The number of bytes already transferred.
 * @return The number of bytes currently to be transferred, a value between 0 -
 * (UINT16_MAX + 1)
 */
uint32_t _iolink_calc_current_transfer_size (
   uint32_t n_bytes_to_transfer,
   uint32_t n_bytes_transferred)
{
   return ((n_bytes_to_transfer - n_bytes_transferred) > 64 * 1024)
             ? 64 * 1024
             : (n_bytes_to_transfer - n_bytes_transferred);
}
