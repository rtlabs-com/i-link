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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "osal_drv.h"

int _iolink_pl_hw_open (const char * name)
{
   // Register driver?
   // Register argument to already existing fd?
   char local_name[101];
#if (__SIZEOF_POINTER__ == 4)
   u_int32_t arg;
#elif (__SIZEOF_POINTER__ == 8)
   u_int64_t arg;
#endif

   strncpy (local_name, name, 100);

   char * argp = strrchr (local_name, '/');
   if (argp == NULL)
   {
      printf ("%s: '/' not found in file name.\n", __func__);
      return 0;
   }

   arg    = atoi (argp + 1);
   *argp  = '\0';
   int fd = open (local_name, O_RDWR, 0);

   if (fd < 1)
   {
      return -1;
   }

   drv_t * drv = find_driver_by_name (local_name);

   printf (
      "%s: Driver %sfound for %lu: %s\n",
      __func__,
      (drv == NULL) ? "NOT " : "",
      arg,
      local_name);

   if (drv == NULL)
   {
      return -1;
   }

   fd_set_driver (fd, drv, (void *)arg);

   return fd;
}
