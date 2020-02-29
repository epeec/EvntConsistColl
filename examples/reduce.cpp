
#include <GASPI.h>

#include <iostream>
#include <math.h>
#include "EvntConsistColl.hxx"

#include "success_or_die.h"
#include "queue.h"

template <class T> void fill_array(const int n, T a[]) {
    gaspi_rank_t iProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );

    for (int i=0; i < n; i++) {
        a[i] = (T) iProc;
    }
}

void check(const int VLEN, const double* res) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
    
    bool correct = true;

    double resval = nProc * (nProc - 1) / 2;
    for (int i = 0; i < VLEN; i++) {
        if (res[i] != resval) {
            std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
            correct = false;
        }
    }

    if (iProc == 0) {
        if (correct) 
    	    std::cout << "\n[Std Reduce] Successful run!\n";
        else 
    	    std::cout << "\n[Std Reduce] Check FAIL!\n";
    }
}

void check_evnt(const int VLEN, const double* res, const double threshold) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
    
    bool correct = true;

    double resval = nProc * (nProc - 1) / 2;
    for (int i = 0; i < ceil(threshold * VLEN); i++) {
        if (res[i] != resval) {
            std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
            correct = false;
        }
    }
    for (int i = ceil(threshold * VLEN); i < VLEN; i++) {
        if (res[i] != 0.0) {
            std::cerr << i << ' ' << res[i] << ' ' << 0.0 << '\n';
            correct = false;
        }
    }

    if (iProc == 0) {
        if (correct) 
    	    std::cout << "\n[EvntConsist Reduce] Successful run!\n";
        else 
    	    std::cout << "\n[EvntConsist Reduce] Check FAIL!\n";
    }
}

// testing regular gaspi reduce that is based on binomial tree
void test_reduce(const int VLEN){

  const int type_size = sizeof(double);

  gaspi_segment_id_t const segment_send_id = 0;
  gaspi_segment_id_t const segment_recv_id = 1;
  gaspi_size_t       const segment_size = VLEN * type_size;

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_send_id, segment_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_recv_id, 2 * segment_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

  segmentBuffer buffer_send = {segment_send_id, 0};    
  segmentBuffer buffer_recv = {segment_recv_id, 0};    
  segmentBuffer buffer_temp = {segment_recv_id, segment_size}; // large offset because buffer_temp is within the same segment   
  
  gaspi_rank_t iProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    
  gaspi_pointer_t send_array, recv_array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send.segment, &send_array) );
  SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_recv.segment, &recv_array) );
 
  double *src_arr = (double *)(send_array);
  double *rcv_arr = (double *)(recv_array);

  fill_array(VLEN, src_arr);
 
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

  gaspi_reduce(buffer_send, buffer_recv, buffer_temp, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, root, queue_id, GASPI_BLOCK);

  if (iProc == root) {
      check(VLEN, rcv_arr);
  }
  
  wait_for_flush_queues();
}

// testing eventually consistent gaspi reduce that is based on binomial tree
void test_evnt_consist_reduce(const int VLEN, const double threshold){

  const int type_size = sizeof(double);
  
  gaspi_rank_t iProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );

  gaspi_segment_id_t const segment_send_id = 2;
  gaspi_segment_id_t const segment_recv_id = 3;
  gaspi_size_t       const segment_size = VLEN * type_size;

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_send_id, segment_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_recv_id, segment_size + ceil(threshold * VLEN) * type_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

  segmentBuffer buffer_send = {segment_send_id, 0};    
  segmentBuffer buffer_recv = {segment_recv_id, 0};    
  segmentBuffer buffer_temp = {segment_recv_id, segment_size}; // large offset because buffer_temp is within the same segment   
    
  gaspi_pointer_t send_array, recv_array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send.segment, &send_array) );
  SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_recv.segment, &recv_array) );
 
  double *src_arr = (double *)(send_array);
  double *rcv_arr = (double *)(recv_array);

  fill_array(VLEN, src_arr);
  
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

  gaspi_reduce(buffer_send, buffer_recv, buffer_temp, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, threshold, root, queue_id, GASPI_BLOCK);

  if (iProc == root) {
      check_evnt(VLEN, rcv_arr, threshold);
  }
  
  wait_for_flush_queues();
}


int main(int argc, char** argv) {

  if ((argc < 3) || (argc > 3)) {
    std::cerr << argv[0] << ": Usage " << argv[0] << " <length in elements> <threshold in [0,1]>"
              << std::endl;
    return -1;
  }

  static const int VLEN = atoi(argv[1]);
  static const double threshold = atof(argv[2]);

  if (threshold <= 0.0 || threshold > 1.0) {
    std::cerr << argv[0] << ": Usage " << argv[0] << " <length in elements> <threshold in [0,1]>"
              << std::endl;
    return -1;
  } 
  
  SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );

  test_reduce(VLEN); 

  test_evnt_consist_reduce(VLEN, threshold); 
 
  SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

  return EXIT_SUCCESS;
}
