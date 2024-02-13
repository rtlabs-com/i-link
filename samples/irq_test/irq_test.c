/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "osal.h"
#include "osal_irq.h"

void isr_func (void* irq_arg)
{
   printf ("\n%s: GPIO pin %d interrupt occurred\n", __func__, (int)irq_arg);
}

/****************************************************************
 * Main
 ****************************************************************/
int main (int argc, char ** argv)
{
   if (argc < 2)
   {
      printf ("Usage: %s <gpio-pin>\n\n", argv[0]);
      printf ("Waits for a change in the GPIO pin voltage level or input on "
              "stdin\n");
      exit (-1);
   }

   _iolink_setup_int (atoi (argv[1]), isr_func, atoi (argv[1]));

   while (true)
   {
   }

   return 0;
}
