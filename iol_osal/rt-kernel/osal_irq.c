#include "osal.h"
#include "osal_irq.h"
#include "kern/int.h"

int _iolink_setup_int (int irq_pin, isr_func_t isr_func, void* irq_arg)
{
    int_connect(irq_pin, isr_func, irq_arg);
    int_enable(irq_pin);
    return 0;
}
