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

#ifndef OSAL_DRV_H
#define OSAL_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/types.h> /* ssize_t */

struct drv;

typedef struct drv_ops
{
   int (*open) (struct drv * drv, const char * name, int flags, int mode);
   int (*close) (struct drv * drv, void * arg);
   ssize_t (*read) (struct drv * drv, void * arg, void * buf, size_t nbytes);
   ssize_t (*write) (struct drv * drv, void * arg, const void * buf, size_t nbytes);
   int (*ioctl) (struct drv * drv, void * arg, int req, void * req_arg);
} drv_ops_t;

typedef struct drv
{
   char * name;
   pthread_mutex_t * mtx;
   const drv_ops_t * ops;
} drv_t;

drv_t * fd_get_driver (int fd);
void * fd_get_arg (int fd);
int fd_set_driver (int fd, drv_t * drv, void * arg);
drv_t * find_driver_by_name (const char * name);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_DRV_H */
