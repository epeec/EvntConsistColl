#include "queue.h"
#include "success_or_die.h"

void wait_for_flush_queues ()
{
  gaspi_number_t queue_num;

  SUCCESS_OR_DIE (gaspi_queue_num (&queue_num));

  gaspi_queue_id_t queue = 0;
 
  while( queue < queue_num )
  {
    SUCCESS_OR_DIE (gaspi_wait (queue, GASPI_BLOCK));
    ++queue;
  }
}
