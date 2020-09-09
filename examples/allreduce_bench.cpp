
#include <GASPI.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

#include "Allreduce.hxx"

#include "success_or_die.h"
#include "queue.h"
#include "common.h"

#include "now.h"

template <typename T>
void check(const int VLEN, const T* res) {
    gaspi_rank_t iProc, nProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num (&nProc) );
    
    bool correct = true;

    for (int i = 0; i < VLEN; i++) {
        T resval = (nProc * (nProc + 1)) / 2 + nProc * i;
        if (res[i] != resval) {
            //std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
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

// testing the gaspi pipelined ring allreduce
template <typename T>
void test_ring_allreduce(const int VLEN, const int numIters, const bool checkRes){
  
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );
    int root = 0;

    const int type_size = sizeof(T);
    gaspi_segment_id_t const segment_send = 0;
    gaspi_segment_id_t const segment_recv = 1;
    gaspi_size_t       const segment_size = VLEN * type_size;

    SUCCESS_OR_DIE
      ( gaspi_segment_create
        ( segment_send, segment_size
        , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
        )
      );

    SUCCESS_OR_DIE
      ( gaspi_segment_create
        ( segment_recv, segment_size + 2 * ((VLEN + nProc - 1) / nProc) * type_size
        , GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED
        )
      );

    segmentBuffer buffer_send = {segment_send, 0};    
    segmentBuffer buffer_recv = {segment_recv, 0};    
    segmentBuffer buffer_temp = {segment_recv, segment_size}; // large offset because buffer_temp is within the same segment   

    gaspi_pointer_t send_array, recv_array;
    SUCCESS_OR_DIE( gaspi_segment_ptr (segment_send, &send_array) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (segment_recv, &recv_array) );

    T * src_arr = (T *)(send_array);
    T * rcv_arr = (T *)(recv_array);

    fill_array(VLEN, src_arr);

    gaspi_queue_id_t queue_id = 0;

    if (iProc == root) {
        printf("%d \t", VLEN);
    }

    double *t_median = (double *) calloc(numIters, sizeof(double));
    for (int iter=0; iter < numIters; iter++) {
        fill_array_zeros(VLEN, rcv_arr);

        double time = -now();

        gaspi_ring_allreduce<T>(buffer_send, buffer_recv, buffer_temp, VLEN, Operation::SUM, queue_id, GASPI_BLOCK);

        time += now();
        t_median[iter] = time;

        if (checkRes) {    
            check<T>(VLEN, rcv_arr);
        }

        //gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
    }
  
    sort_median(&t_median[0],&t_median[numIters-1]);
    double mean = calculateMean(numIters, &t_median[0]);
    double confidenceLevel = calculateConfidenceLevel(numIters, &t_median[0], mean);

    if (iProc == root) {
        printf("%10.6f \t", t_median[numIters/2]);
        printf("%10.6f \t", mean);
        printf("%10.6f \n", confidenceLevel);
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

    static const int VLEN = atoi(argv[1]);
    const int numIters = atoi(argv[2]);
    const bool checkRes = (argc==4)?true:false;

    SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );

    test_ring_allreduce<double>(VLEN, numIters, checkRes); 

    SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

    return EXIT_SUCCESS;
}
