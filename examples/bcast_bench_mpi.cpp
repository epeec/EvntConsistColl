
#include <mpi.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "now.h"

template <class T> static void swap(T *a, T *b) {
  T tmp = *a;
  *a = *b;
  *b = tmp;
}

template <class T> void sort_median(T *begin, T *end) {
  T *ptr;
  T *split;
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
    int iProc;
    MPI_Comm_rank(MPI_COMM_WORLD, &iProc);

    for (int i=0; i < n; i++) {
        a[i] = i + iProc + 1;
    }
}

template <class T> void fill_array_zeros(const int n, T a[]) {
    for (int i=0; i < n; i++) {
        a[i] = 0;
    }
}

template <class T> double calculateMean(const int n, const T* a) {
    double sum = 0.0, mean;

    // compute mean
    int i;
    for(i = 0; i < n; ++i) {
        sum += a[i];
    }
    mean = sum / n;

    return mean;
}

template <class T> double calculateConfidenceLevel(const int n, const T* a, const double mean) {
    double standardDeviation = 0.0, confidenceLevel;

    int i;
    // compute standard deviation
    for(i = 0; i < n; ++i)
        standardDeviation += pow(a[i] - mean, 2);
    standardDeviation = sqrt(standardDeviation / n);

    // compute confidence level of 95%
    confidenceLevel = 1.96 * (standardDeviation / sqrt((double) n));
    
    return confidenceLevel;
}


template <class T> void check(const int VLEN, const T* res, const int proc) {
    int iProc, nProc;
    MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);
    
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
    int iProc, nProc;
    MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);
    
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

  int iProc, nProc;
  MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
  MPI_Comm_size(MPI_COMM_WORLD, &nProc);
  int root = 0;

  const int type_size = sizeof(double);
  
  double *arr = (double *) calloc(VLEN, type_size);

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

    MPI_Bcast(arr, VLEN, MPI_DOUBLE, root, MPI_COMM_WORLD);

    time += now();
    t_median[itime] = time;

    MPI_Barrier(MPI_COMM_WORLD);

    if (iProc == (nProc - 1) && checkRes) {    
      check(VLEN, arr, nProc - 1);
    }
  }
  
  sort_median(&t_median[0],&t_median[numIters-1]);
  double mean = calculateMean(numIters, &t_median[0]);
  double confidenceLevel = calculateConfidenceLevel(numIters, &t_median[0], mean);

  if (iProc == root) {
    printf("%10.6f \t", t_median[numIters/2]);
    printf("%10.6f \t", mean);
    printf("%10.6f \n", confidenceLevel);
  }
}

//// testing eventually consistent gaspi bcast that is based on binomial tree
//void test_evnt_consist_bcast(const int VLEN, const int numIters, const bool checkRes){
//
//  gaspi_rank_t iProc, nProc;
//  SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
//  SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
//
//  const int type_size = sizeof(double);
//  
//  gaspi_rank_t root = 0;
//  gaspi_queue_id_t queue_id = 0;
//
//  gaspi_segment_id_t const segment_id = 2;
//  gaspi_size_t       const segment_size = VLEN * type_size;
//
//  SUCCESS_OR_DIE
//    ( gaspi_segment_create
//      ( segment_id, segment_size
//      , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
//      )
//    );
//
//  segmentBuffer buffer = {segment_id, 0};    
//    
//  gaspi_pointer_t array;
//  SUCCESS_OR_DIE( gaspi_segment_ptr (buffer.segment, &array) );
// 
//  double *arr = (double *)(array);
//
//  // 25% 50% 75% and 100%
//  if (iProc == 0) {
//    printf("%d \t", VLEN);
//  }
//
//  double *t_median = (double *) calloc(numIters, sizeof(double));
//  for (int index = 1; index < 5; index++) { 
//      gaspi_double threshold = index * 0.25;
//
//      // measure execution time
//      for (int itime = 0; itime < numIters; itime++) { 
//        if (iProc == root) 
//          fill_array(VLEN, arr);
//        else 
//          fill_array_zeros(VLEN, arr);
//
//        double time = -now();
//
//        gaspi_bcast(buffer, VLEN, GASPI_TYPE_DOUBLE, threshold, root, queue_id, GASPI_BLOCK);
//  
//        time += now();
//        t_median[itime] = time;
//
//        if (iProc == (nProc - 1) && checkRes) {    
//          check_evnt(VLEN, arr, threshold, nProc - 1);
//        }
//
//        //gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
//      }
//      
//      sort_median(&t_median[0],&t_median[numIters-1]);
//
//      if (iProc == 0) {
//        printf("%10.6f \t", t_median[numIters/2]);
//      }
//  }
//
//  if (iProc == 0) {
//    printf("\n");
//  }
//
//  wait_for_flush_queues();
//}


int main(int argc, char** argv) {

  if ((argc < 3) || (argc > 4)) {
    std::cerr << argv[0] << ": Usage " << argv[0] << " <length in elements>"
              << " <num iterations> [check]"
              << std::endl;
    return -1;
  }
  
  static const int VLEN = atoi(argv[1]);
  const int numIters = atoi(argv[2]);
  const bool checkRes = (argc==4)?true:false;

  MPI_Init(&argc, &argv);

  test_bcast(VLEN, numIters, checkRes); 

  //test_evnt_consist_bcast(VLEN, numIters, checkRes); 
 
  MPI_Finalize();

  return EXIT_SUCCESS;
}
