
#include <GASPI.h>

#include "evntl.consist.coll.hxx"

#include "success_or_die.h"
#include "queue.h"

// testing regular gaspi reduce
void test_reduce(const int VLEN, gaspi_segment_id_t const segment_id){
  
  gaspi_rank_t iProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    
  gaspi_pointer_t array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_id, &array) );
 
  double * src_array = (double *)(array);
  double * rcv_array = src_array + VLEN;

  for (int j = 0; j < VLEN; ++j)
  {
      src_array[j] = (double)( iProc * VLEN + j );
  }

  //gaspi_allreduce(src_array, src_array, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, GASPI_GROUP_ALL, GASPI_BLOCK);
  gaspi_reduce(segment_id, 0, segment_id, VLEN, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, 0, GASPI_GROUP_ALL, GASPI_BLOCK);

  for (int j = 0; j < 2 * VLEN; ++j)
  {
      printf("rank %d rcv elem %d: %f \n", iProc, j, src_array[j] );
  }
  printf("\n");
  
  wait_for_flush_queues();
}


int main( )
{
  
  SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );

  static const int VLEN = 4;

  gaspi_segment_id_t const segment_id = 0;
  gaspi_size_t       const segment_size = VLEN * sizeof (double);

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_id, segment_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

//  gaspi_double const threshold = 0.6;

  test_reduce(VLEN, segment_id); 
 
  SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

  return EXIT_SUCCESS;
}
