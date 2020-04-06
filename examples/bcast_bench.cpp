
#include <GASPI.h>
#include <iostream>
#include <math.h>

#include "EvntConsistColl.hxx"

#include "success_or_die.h"
#include "queue.h"
#include "common.h"

#include "now.h"

template <class T> void check(const int VLEN, const T* res, const int proc) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
    
    bool correct = true;

    for (int i = 0; i < VLEN; i++) {
        T resval = i + 1;
        if (res[i] != resval) {
            std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
            correct = false;
        }
    }

    if (iProc == proc) {
        if (correct) 
    	    std::cout << "\n[Std Broadcast] Successful run!\n";
        else 
    	    std::cout << "\n[Std Broadcast] Check FAIL!\n";
    }
}

template <class T> void check_evnt(const int VLEN, const T* res, const double threshold, const int proc) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
    
    bool correct = true;

    for (int i = 0; i < ceil(threshold * VLEN); i++) {
        T resval = i + 1;
        if (res[i] != resval) {
            std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
            correct = false;
        }
    }

    if (iProc == proc) {
        if (correct) 
    	    std::cout << "\n[EvntConsist Broadcast] Successful run!\n";
        else 
    	    std::cout << "\n[EvntConsist Broadcast] Check FAIL!\n";
    }
}

// testing regular gaspi bcast that is based on binomial tree
void test_bcast(const int VLEN, const int numIters, const bool checkRes){

  gaspi_rank_t iProc, nProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
  SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );

  const int type_size = sizeof(double);
  
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

  gaspi_segment_id_t const segment_id = 0;
  gaspi_size_t       const segment_size = VLEN * type_size;

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_id, segment_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

  segmentBuffer buffer = {segment_id, 0};    
  
  gaspi_pointer_t array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (segment_id, &array) );
  double *arr = (double *)(array);

  if (iProc == root) {
    printf("%d \t", VLEN);
  }

  double *t_median = (double *) calloc(numIters, sizeof(double));
  // measure execution time
  for (int itime = 0; itime < numIters; itime++) { 
    if (iProc == root) 
      fill_array(VLEN, arr);
    else 
      fill_array_zeros(VLEN, arr);

    double time = -now();

    gaspi_bcast(buffer, VLEN, GASPI_TYPE_DOUBLE, root, queue_id, GASPI_BLOCK);

    time += now();
    t_median[itime] = time;

    if (iProc == (nProc - 1) && checkRes) {    
      check(VLEN, arr, nProc - 1);
    }

    //gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
  }
  
  sort_median(&t_median[0],&t_median[numIters-1]);

  if (iProc == root) {
    printf("%10.6f \n", t_median[numIters/2]);
  }
  
  wait_for_flush_queues();
}

// testing eventually consistent gaspi bcast that is based on binomial tree
void test_evnt_consist_bcast(const int VLEN, const int numIters, const bool checkRes){

  gaspi_rank_t iProc, nProc;
  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
  SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );

  const int type_size = sizeof(double);
  
  gaspi_rank_t root = 0;
  gaspi_queue_id_t queue_id = 0;

  gaspi_segment_id_t const segment_id = 2;
  gaspi_size_t       const segment_size = VLEN * type_size;

  SUCCESS_OR_DIE
    ( gaspi_segment_create
      ( segment_id, segment_size
      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
      )
    );

  segmentBuffer buffer = {segment_id, 0};    
    
  gaspi_pointer_t array;
  SUCCESS_OR_DIE( gaspi_segment_ptr (buffer.segment, &array) );
 
  double *arr = (double *)(array);

  // 25% 50% 75% and 100%
  if (iProc == 0) {
    printf("%d \t", VLEN);
  }

  double *t_median = (double *) calloc(numIters, sizeof(double));
  for (int index = 1; index < 5; index++) { 
      gaspi_double threshold = index * 0.25;

      // measure execution time
      for (int itime = 0; itime < numIters; itime++) { 
        if (iProc == root) 
          fill_array(VLEN, arr);
        else 
          fill_array_zeros(VLEN, arr);

        double time = -now();

        gaspi_bcast(buffer, VLEN, GASPI_TYPE_DOUBLE, threshold, root, queue_id, GASPI_BLOCK);
  
        time += now();
        t_median[itime] = time;

        if (iProc == (nProc - 1) && checkRes) {    
          check_evnt(VLEN, arr, threshold, nProc - 1);
        }

        //gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
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

  //test_bcast(VLEN, numIters, checkRes); 

  test_evnt_consist_bcast(VLEN, numIters, checkRes); 
 
  SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

  return EXIT_SUCCESS;
}
