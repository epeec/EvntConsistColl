
#include <GASPI.h>

#include "EvntConsistColl.hxx"

#include "success_or_die.h"
#include "queue.h"

// testing regular gaspi reduce that is based on binomial tree
void test_reduce(const int VLEN, gaspi_segment_id_t const segment_send_id, gaspi_segment_id_t segment_recv_id){
  
  gaspi_rank_t iProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    
  gaspi_pointer_t send_array, recv_array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_send_id, &send_array) );
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_recv_id, &recv_array) );
 
  double * src_arr = (double *)(send_array);
  double * rcv_arr = (double *)(recv_array);

  for (int j = 0; j < VLEN; ++j)
  {
      src_arr[j] = (double)( iProc * VLEN + j );
  }
  
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

  gaspi_reduce(segment_send_id, 0, segment_recv_id, 0, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, root, queue_id, GASPI_BLOCK);
  //gaspi_allreduce(src_array, src_array, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, GASPI_GROUP_ALL, GASPI_BLOCK);

  if (iProc == root) {
      printf("=== Result ===\n");
      for (int j = 0; j < VLEN; ++j)
      {
          printf("rank %d rcv elem %d: %f \n", iProc, j, rcv_arr[j] );
      }
      printf("======\n");
  }
  
  wait_for_flush_queues();
}

// testing eventually consistent gaspi reduce that is based on binomial tree
void test_evnt_consist_reduce(const int VLEN, gaspi_segment_id_t const segment_send_id, gaspi_segment_id_t segment_recv_id, const double threshold){
  
  gaspi_rank_t iProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    
  gaspi_pointer_t send_array, recv_array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_send_id, &send_array) );
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_recv_id, &recv_array) );
 
  double * src_arr = (double *)(send_array);
  double * rcv_arr = (double *)(recv_array);

  for (int j = 0; j < VLEN; ++j)
  {
      src_arr[j] = (double)( iProc * VLEN + j );
  }
  
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

  gaspi_reduce(segment_send_id, 0, segment_recv_id, 0, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, threshold, root, queue_id, GASPI_BLOCK);

  for (int j = 0; j < VLEN; ++j)
  {
      printf("rank %d rcv elem %d: %f \n", iProc, j, src_arr[j] );
  }
  printf("\n");

  if (iProc == root) {
      printf("=== Result ===\n");
      for (int j = 0; j < VLEN; ++j)
      {
          printf("rank %d rcv elem %d: %f \n", iProc, j, rcv_arr[j] );
      }
      printf("======\n");
  }
  
  wait_for_flush_queues();
}


int main( )
{
  
  SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );

  static const int VLEN = 4;

  gaspi_segment_id_t const segment_send_id = 0;
  gaspi_segment_id_t const segment_recv_id = 1;
  gaspi_size_t       const segment_size = VLEN * sizeof (double);

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_send_id, segment_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_recv_id, segment_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

  gaspi_double const threshold = 0.5;

  test_evnt_consist_reduce(VLEN, segment_send_id, segment_recv_id, threshold); 

  //test_reduce(VLEN, segment_send_id, segment_recv_id); 
 
  SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

  return EXIT_SUCCESS;
}
