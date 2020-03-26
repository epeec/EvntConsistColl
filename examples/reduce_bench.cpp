
#include <GASPI.h>
#include <iostream>
#include <math.h>

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

template <class T> void fill_array(const int n, T a[]) {
    gaspi_rank_t iProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );

    for (int i=0; i < n; i++) {
        a[i] = i + iProc + 1;
    }
}

template <class T> void fill_array_zeros(const int n, T a[]) {
    gaspi_rank_t iProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );

    for (int i=0; i < n; i++) {
        a[i] = 0;
    }
}

void check(const int VLEN, const double* res) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
    
    bool correct = true;

    for (int i = 0; i < VLEN; i++) {
        double resval = (nProc * (nProc + 1)) / 2 + nProc * i;
        if (res[i] != resval) {
            //std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
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

    for (int i = 0; i < ceil(threshold * VLEN); i++) {
        double resval = (nProc * (nProc + 1)) / 2 + nProc * i;
        if (res[i] != resval) {
            //std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
            correct = false;
        }
    }
    for (int i = ceil(threshold * VLEN); i < VLEN; i++) {
        if (res[i] != 0.0) {
            //std::cerr << i << ' ' << res[i] << ' ' << 0.0 << '\n';
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
void test_reduce(const int VLEN, const int numIters, const bool checkRes){

  gaspi_rank_t iProc, nProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
  SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );

  const int type_size = sizeof(double);
  
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

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
  
  gaspi_pointer_t send_array, recv_array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_send_id, &send_array) );
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_recv_id, &recv_array) );
 
  double * src_arr = (double *)(send_array);
  double * rcv_arr = (double *)(recv_array);

  fill_array(VLEN, src_arr);

  if (iProc == root) {
    printf("%d \t", VLEN);
  }

  double *t_median = (double *) calloc(numIters, sizeof(double));
  // measure execution time
  for (int itime = 0; itime < numIters; itime++) { 
    fill_array_zeros(VLEN, rcv_arr);

    double time = -now();

    gaspi_reduce(buffer_send, buffer_recv, buffer_temp, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, root, queue_id, GASPI_BLOCK);

    time += now();
    t_median[itime] = time;

    if (iProc == root && checkRes) {    
      check(VLEN, rcv_arr);
    }

    gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
  }
  
  sort_median(&t_median[0],&t_median[numIters-1]);

  if (iProc == root) {
    printf("%10.6f \n", t_median[numIters/2]);
  }
  
  wait_for_flush_queues();
}

// testing eventually consistent gaspi reduce that is based on binomial tree
void test_evnt_consist_reduce(const int VLEN, const int numIters, const bool checkRes){

  gaspi_rank_t iProc, nProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
  SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );

  const int type_size = sizeof(double);
  
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

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
      ( segment_recv_id, 2 * segment_size
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

  // 25% 50% 75% and 100%
  if (iProc == 0) {
    printf("%d \t", VLEN);
  }

  double *t_median = (double *) calloc(numIters, sizeof(double));
  for (int index = 1; index < 5; index++) { 
      gaspi_double threshold = index * 0.25;

      // measure execution time
      for (int itime = 0; itime < numIters; itime++) { 
        fill_array_zeros(VLEN, rcv_arr);

        double time = -now();

        gaspi_reduce(buffer_send, buffer_recv, buffer_temp, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, threshold, root, queue_id, GASPI_BLOCK);
  
        time += now();
        t_median[itime] = time;

        if (iProc == root && checkRes) {    
          check_evnt(VLEN, rcv_arr, threshold);
        }

        gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
      }
      
      sort_median(&t_median[0],&t_median[numIters-1]);

      if (iProc == 0) {
        printf("%10.6f \t", t_median[numIters/2]);
      }
  }

  if (iProc == 0) {
    printf("\n");
  }

  wait_for_flush_queues();
}


int main(int argc, char** argv) {

  if ((argc < 3) || (argc > 4)) {
    std::cerr << argv[0] << ": Usage " << argv[0] << " <length in elements>"
              << " <num iterations> [check]"
              << std::endl;
    return -1;
  }
  
  SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );

  static const int VLEN = atoi(argv[1]);
  const int numIters = atoi(argv[2]);
  const bool checkRes = (argc==4)?true:false;

  //test_reduce(VLEN, numIters, checkRes); 

  test_evnt_consist_reduce(VLEN, numIters, checkRes); 
 
  SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

  return EXIT_SUCCESS;
}
