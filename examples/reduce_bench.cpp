
#include <GASPI.h>

#include "EvntConsistColl.hxx"

#include "success_or_die.h"
#include "queue.h"

#include "now.h"

static void swap(double *a, double *b)
{
  double tmp = *a;
  *a = *b;
  *b = tmp;
}

void sort_median(double *begin, double *end)
{
  double *ptr;
  double *split;
  if (end - begin <= 1)
    return;
  ptr = begin;
  split = begin + 1;
  while (++ptr != end) {
    if (*ptr < *begin) {
      swap(ptr, split);
      ++split;
    }
  }
  swap(begin, split - 1);
  sort_median(begin, split - 1);
  sort_median(split, end);
}

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
      src_arr[j] = (double) iProc;
  }
  
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

  gaspi_reduce(segment_send_id, 0, segment_recv_id, 0, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, root, queue_id, GASPI_BLOCK);
  
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
      src_arr[j] = (double) iProc;
  }
  
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

  gaspi_reduce(segment_send_id, 0, segment_recv_id, 0, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, threshold, root, queue_id, GASPI_BLOCK);
  
  wait_for_flush_queues();
}


int main( )
{
  
  SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );

  static const int VLEN = M_SZ;

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

  gaspi_rank_t iProc, nProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
  SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );

  // 25% 50% 75% and 100%
  if (iProc == 0) {
    printf("%d \t", nProc);
  }
  for (int index = 1; index < 5; index++) { 
      gaspi_double threshold = index * 0.25;
      double t_median[N_SA];

      // measure execution time
      for (int itime = 0; itime < N_SA; itime++) { 
        double time = -now();
        test_evnt_consist_reduce(VLEN, segment_send_id, segment_recv_id, threshold); 
        time += now();

        t_median[itime] = time;
      }
      
      sort_median(&t_median[0],&t_median[N_SA-1]);

      if (iProc == 0) {
        printf("%10.6f \t", t_median[N_SA/2]);
      }
  }
  if (iProc == 0) {
    printf("\n");
  }
 
  SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

  return EXIT_SUCCESS;
}
