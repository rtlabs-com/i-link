/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2018 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "osal.h"
#include "osal_drv.h"
#include <string.h>
#define DRIVERS_SIZE 4

typedef struct
{
   int fd;
   drv_t * drv;
   void * arg;
} drv_list_t;

drv_list_t drivers[DRIVERS_SIZE] = {0};

drv_t * fd_get_driver (int fd)
{
   int i;
   for (i = 0; i < DRIVERS_SIZE; i++)
   {
      if (drivers[i].fd == fd)
      {
         return drivers[i].drv;
      }
      else if (drivers[i].fd == 0)
      {
         break;
      }
   }
   return NULL;
}

void * fd_get_arg (int fd)
{
   int i;
   for (i = 0; i < DRIVERS_SIZE; i++)
   {
      if (drivers[i].fd == fd)
      {
         return drivers[i].arg;
      }
      else if (drivers[i].fd == 0)
      {
         break;
      }
   }
   return NULL;
}

int fd_set_driver (int fd, drv_t * drv, void * arg)
{
   int i;
   for (i = 0; i < DRIVERS_SIZE; i++)
   {
      if (drivers[i].fd == 0)
      {
         drivers[i].fd  = fd;
         drivers[i].drv = drv;
         drivers[i].arg = arg;
         drv->mtx       = os_mutex_create();
         return 0;
      }
   }
   return -1;
}

drv_t * find_driver_by_name (const char * name)
{
   int i;
   for (i = 0; i < DRIVERS_SIZE; i++)
   {
      if (drivers[i].drv == NULL)
      {
         break;
      }
      if (!strcmp (drivers[i].drv->name, name))
      {
         return drivers[i].drv;
      }
   }
   return NULL;
}
