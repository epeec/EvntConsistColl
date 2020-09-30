
#include <GASPI.h>
#include <iostream>
#include <math.h>

#include "EvntConsistColl.hxx"

#include "success_or_die.h"
#include "queue.h"
#include "common.h"

#include "now.h"

template <typename T>
void check_min(const int VLEN, const T* res, const double threshold) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
    
    bool correct = true;

    for (int i = 0; i < ceil(threshold * VLEN); i++) {
        T resval = i + 1;
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
        if (correct) {
    	    std::cout << "Successful run!\n";
        } else { 
    	    std::cout << "Check FAIL!\n";
        }
    }
}

template <typename T>
void check_max(const int VLEN, const T* res, const double threshold) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
    
    bool correct = true;

    for (int i = 0; i < ceil(threshold * VLEN); i++) {
        T resval = i + nProc;
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
        if (correct) {
    	    std::cout << "Successful run!\n";
        } else { 
    	    std::cout << "Check FAIL!\n";
        }
    }
}

template <typename T>
void check_sum(const int VLEN, const T* res, const double threshold) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
    
    bool correct = true;

    for (int i = 0; i < ceil(threshold * VLEN); i++) {
        T resval = (nProc * (nProc + 1)) / 2 + nProc * i;
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
        if (correct) {
    	    std::cout << "Successful run!\n";
        } else { 
    	    std::cout << "Check FAIL!\n";
        }
    }
}

template <typename T>
void check(const Operation &op, const int VLEN, const T* res, const double threshold) {
    switch (op) {
        case MIN: {
            check_min<T>(VLEN, res, threshold);
            break;
        }

        case MAX: {
            check_max<T>(VLEN, res, threshold);
            break;
        }

        case SUM: {
            check_sum<T>(VLEN, res, threshold);
            break;
        }

        default: {
            throw std::runtime_error ("[Reduce] Unsupported Operation");
        }
    }
}

// testing regular gaspi reduce that is based on binomial tree
template <typename T>
void test_reduce(const Operation &op, const int VLEN, const int numIters, const bool checkRes){

  gaspi_rank_t iProc, nProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
  SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );

  const int type_size = sizeof(T);
  
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
 
  T * src_arr = (T *)(send_array);
  T * rcv_arr = (T *)(recv_array);

  fill_array(VLEN, src_arr);

  if (iProc == root) {
    printf("%d \t", VLEN);
  }

  double *t_median = (double *) calloc(numIters, sizeof(double));
  // measure execution time
  for (int itime = 0; itime < numIters; itime++) { 
    fill_array_zeros(VLEN, rcv_arr);

    double time = -now();

    gaspi_reduce<T>(buffer_send, buffer_recv, buffer_temp, VLEN, op, root, queue_id, GASPI_BLOCK);

    time += now();
    t_median[itime] = time;

    if (iProc == root && checkRes) {    
      check(op, VLEN, rcv_arr, 1.0);
    }

    //gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
  }
  
  sort_median(&t_median[0],&t_median[numIters-1]);

  if (iProc == root) {
    printf("%10.6f \n", t_median[numIters/2]);
  }
  
  wait_for_flush_queues();
}

// testing eventually consistent gaspi reduce that is based on binomial tree
template <typename T>
void test_evnt_consist_reduce(const Operation &op, const int VLEN, const int numIters, const bool checkRes){

  gaspi_rank_t iProc, nProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
  SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );

  const int type_size = sizeof(T);
  
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
 
  T *src_arr = (T *)(send_array);
  T *rcv_arr = (T *)(recv_array);

  fill_array(VLEN, src_arr);

  if (iProc == 0) {
    printf("%d \t", VLEN);
  }

  double *t_median = (double *) calloc(numIters, sizeof(double));
  for (int index = 1; index < 5; index++) { 
      // 25% 50% 75% and 100%
      gaspi_double threshold = index * 0.25;

      // measure execution time
      for (int itime = 0; itime < numIters; itime++) { 
        fill_array_zeros(VLEN, rcv_arr);

        double time = -now();

        gaspi_reduce<T>(buffer_send, buffer_recv, buffer_temp, VLEN, op, threshold, root, queue_id, GASPI_BLOCK);
  
        time += now();
        t_median[itime] = time;

        if (iProc == root && checkRes) {    
          check(op, VLEN, rcv_arr, threshold);
        }

        //gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
      }
      
      sort_median(&t_median[0],&t_median[numIters-1]);
      double mean = calculateMean(numIters, &t_median[0]);
      double confidenceLevel = calculateConfidenceLevel(numIters, &t_median[0], mean);

      if (iProc == 0) {
        printf("%10.6f \t", t_median[numIters/2]);
        printf("%10.6f \t", mean);
        printf("%10.6f \t", confidenceLevel);
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

  //test_reduce<double>(Operation::SUM, VLEN, numIters, checkRes); 

  test_evnt_consist_reduce<double>(Operation::SUM, VLEN, numIters, checkRes); 
 
  SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

  return EXIT_SUCCESS;
}
