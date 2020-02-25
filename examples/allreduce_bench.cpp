
#include <GASPI.h>
#include <iostream>

#include "EvntConsistColl.hxx"

#include "success_or_die.h"
#include "queue.h"

#include "now.h"

void check(const int VLEN, const double* res) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );

    
    double resval = nProc * (nProc - 1) / 2;
    bool correct = true;

    for (int i = 0; i < VLEN; i++) {
        if (res[i] != resval) {
            std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
            correct = false;
        }
    }

    if (iProc == 0 && correct)
    	std::cout << "Successful run!\n";
}

// testing regular gaspi allreduce that is based on binomial tree
void test_ring_allreduce(const int VLEN){
  
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

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
    
  gaspi_pointer_t send_array, recv_array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_send_id, &send_array) );
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_recv_id, &recv_array) );
 
  double * src_arr = (double *)(send_array);
  double * rcv_arr = (double *)(recv_array);

  for (int j = 0; j < VLEN; ++j)
  {
      src_arr[j] = (double) (iProc);
  }
  
  gaspi_queue_id_t queue_id = 0;

  gaspi_ring_allreduce(segment_send_id, 0, segment_recv_id, 0, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, queue_id, GASPI_BLOCK);

  check(VLEN, rcv_arr);
  
  wait_for_flush_queues();
}


int main( )
{
  
  SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );

  static const int VLEN = 1000;

  test_ring_allreduce(VLEN); 
 
  SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

  return EXIT_SUCCESS;
}
