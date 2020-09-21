
#include <mpi.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
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

template <class T> void fill_array_mpi(const int n, T a[]) {
    int iProc;
    MPI_Comm_rank(MPI_COMM_WORLD, &iProc);

    for (int i=0; i < n; i++) {
        a[i] = i + iProc + 1;
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

void check(const int VLEN, const double* res) {
    int iProc, nProc;
    MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);
    
    bool correct = true;

    for (int i = 0; i < VLEN; i++) {
        double resval = (nProc * (nProc + 1)) / 2 + nProc * i;
        if (res[i] != resval) {
            std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
            correct = false;
        }
    }

    if (iProc == 0) {
        if (correct) 
    	    std::cout << "Successful run!\n";
        else 
    	    std::cout << "Check FAIL!\n";
    }
}

// testing regular mpi allreduce 
void test_allreduce(const int VLEN, const int numIters, const bool checkRes){
  
    int iProc, nProc;
    MPI_Comm_rank(MPI_COMM_WORLD, &iProc);
    MPI_Comm_size(MPI_COMM_WORLD, &nProc);
    int root = 0;

    const int type_size = sizeof (double);

    double * src_arr = (double *) calloc(VLEN, type_size);
    double * rcv_arr = (double *) calloc(VLEN, type_size);

    fill_array_mpi(VLEN, src_arr);

    MPI_Barrier(MPI_COMM_WORLD);

    if (iProc == root) {
        printf("%d \t", VLEN);
    }

    double *t_median = (double *) calloc(numIters, sizeof(double));

    for (int iter=0; iter < numIters; iter++) {
        double time = -now();

        MPI_Allreduce(src_arr, rcv_arr, VLEN, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        time += now();
        t_median[iter] = time;

        MPI_Barrier(MPI_COMM_WORLD);

        if (checkRes) {    
            check(VLEN, rcv_arr);
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

    test_allreduce(VLEN, numIters, checkRes); 

    MPI_Finalize();

    return EXIT_SUCCESS;
}
