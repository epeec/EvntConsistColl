
#include <GASPI.h>

#include "evntl.consist.coll.hxx"

#include "success_or_die.h"
#include "queue.h"

// testing regular gaspi bcast
void test_bcast(const int VLEN, gaspi_segment_id_t const segment_id){
  
  gaspi_rank_t iProc, nProc, root = 0;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
  SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );
    
  gaspi_pointer_t array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_id, &array) );
 
  double * src_array = (double *)(array);

  for (int j = 0; j < VLEN; ++j)
  {
      if (iProc == root) 
          src_array[j]= (double)( iProc * VLEN + j );
  }

  gaspi_queue_id_t queue_id = 0;

  gaspi_bcast(segment_id, 0, VLEN, GASPI_TYPE_DOUBLE, root, queue_id);

  for (int j = 0; j < VLEN; ++j)
  {
      printf("rank %d rcv elem %d: %f \n", iProc, j, src_array[j] );
  }
  printf("\n");
  
  wait_for_flush_queues();
}


// testing eventually consistent gaspi bcast
void test_consist_bcast(const int VLEN, gaspi_segment_id_t const segment_id, const double threshold){
 
  gaspi_rank_t iProc, nProc, root = 0;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
  SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );
    
  gaspi_pointer_t array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_id, &array) );
 
  double * src_array = (double *)(array);

  for (int j = 0; j < VLEN; ++j)
  {
      if (iProc == root) 
          src_array[j]= (double)( iProc * VLEN + j );
  }

  gaspi_queue_id_t queue_id = 0;

  gaspi_bcast(segment_id, 0, VLEN, GASPI_TYPE_DOUBLE, threshold, root, queue_id);

  for (int j = 0; j < VLEN; ++j)
  {
      printf("rank %d rcv elem %d: %f \n", iProc, j, src_array[j] );
  }
  printf("\n");
  
  wait_for_flush_queues();
}


int main( )
{
  
  SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );

  static const int VLEN = 8;

  gaspi_segment_id_t const segment_id = 0;
  gaspi_size_t       const segment_size = VLEN * sizeof (double);

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_id, segment_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

  gaspi_double const threshold = 0.6;
  test_consist_bcast(VLEN, segment_id, threshold); 

  test_bcast(VLEN, segment_id); 
 
  SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

  return EXIT_SUCCESS;
}
