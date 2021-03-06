#ifndef WAITSOME_H
#define WAITSOME_H

#include <GASPI.h>

#include "assert.h"
#include "success_or_die.h"

void wait_or_die
  ( gaspi_segment_id_t segment_id
  , gaspi_notification_id_t notification_id
  , gaspi_notification_t expected
  );

#endif
