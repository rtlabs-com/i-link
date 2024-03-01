/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef OSAL_PL_HW_SPI_H
#define OSAL_PL_HW_SPI_H

#include <sys/types.h> /* size_t */
#include <inttypes.h>  /* uint32_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function that opens an SPI channel, initiates it for SPI communication,
 * and returns a file descriptor/handle.
 *
 * If the opening of an SPI channel fails, NULL will be returned.
 *
 * @note This function's behavior may vary depending on the underlying operating
 * system.
 * @note When compiling with rt-kernel or Linux without USB,
 * the spi_slave_name should be a null-terminated string with the file path to
 * the SPI device wanted to be opened. Example: spi_slave_name =
 * "/dev/spidev0.0"
 * @note In USB mode, a mutex (ftdi_io_mutex) is created and used when
 * reading/writing to the FTDI chip. If the opening of a device fails, this
 * mutex will be destroyed before returning NULL.
 * @note In USB mode, only one channel is supported, and it is opened with
 * spi_slave_name = "0" as a null-terminated string.
 *
 * @param spi_slave_name   In: A null-terminated string with the SPI slave
 * identifier.
 * @return File descriptor, or NULL on failure.
 */
void * _iolink_pl_hw_spi_init (const char * spi_slave_name);

/**
 * Function used for closing the SPI channel.
 *
 * This function releases the resources associated with the specified file
 * descriptor/handle.
 *
 * @note The behavior of this function may vary depending on the underlying
 * operating system.
 * @note In USB mode, the function destroys the mutex (ftdi_io_mutex) used for
 * exclusive locking during SPI communication.
 *
 * @param fd   In: File descriptor/handle for the SPI channel
 */
void _iolink_pl_hw_spi_close (void * fd);

/**
 * Reads and writes data from/to an SPI slave device.
 *
 * This function transfer data in both directions between an SPI
 * master and a slave. During each clock cycle, one bit is clocked out from the
 * master, and one is clocked in to the master. If the SPI transmission fails, a
 * LOG_ERROR will be issued.
 *
 * @note The behavior of this function may vary depending on the underlying
 * operating system.
 * @note In USB mode, the ftd2xx API will be used to communicate with the device
 * @note In USB mode, a mutex (ftdi_io_mutex) is locked during communication
 * with the FTDI chip to ensure exclusive access.
 *
 * @param fd                  In: File descriptor/handle for the SPI channel
 * @param data_read           Out: Pointer to the buffer that receives the data
 * @param data_written        In: Pointer to the buffer that contains the data
 * to be written
 * @param n_bytes_to_transfer In: The number of bytes to be transferred
 */
void _iolink_pl_hw_spi_transfer (
   void * fd,
   void * data_read,
   const void * data_written,
   size_t n_bytes_to_transfer);

/* Functions exposed for unit testing */
uint32_t _iolink_calc_current_transfer_size (
   uint32_t n_bytes_to_transfer,
   uint32_t n_bytes_transferred);

#ifdef __cplusplus
}
#endif

#endif
