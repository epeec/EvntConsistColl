#ifndef _WAIT_IF_QUEUE_FULL_H
#define _WAIT_IF_QUEUE_FULL_H

#include <GASPI.h>
#include <success_or_die.h>

#define WAIT_IF_QUEUE_FULL(f, queue)                      \
{                                                         \
    gaspi_return_t ret;                                   \
    while ((ret = (f)) == GASPI_QUEUE_FULL)               \
    {                                                     \
        SUCCESS_OR_DIE(gaspi_wait((queue), GASPI_BLOCK)); \
    }                                                     \
    ASSERT(ret == GASPI_SUCCESS);                         \
}

#endif
