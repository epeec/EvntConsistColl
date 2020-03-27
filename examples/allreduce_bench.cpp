
#include <GASPI.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "EvntConsistColl.hxx"

#include "success_or_die.h"
#include "queue.h"

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

template <class T> void fill_array(T a[],
         const int n) {
    gaspi_rank_t iProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );

    for (int i=0; i < n; i++) {
        a[i] = i + iProc + 1;
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
            std::cerr << i << ' ' << res[i] << ' ' << resval << '\n';
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
void test_ring_allreduce(const int VLEN, const int numIters, const bool checkRes){
  
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );
    int root = 0;

    const int type_size = sizeof (double);
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

    double * src_arr = (double *)(send_array);
    double * rcv_arr = (double *)(recv_array);

    fill_array(src_arr, VLEN);

    gaspi_queue_id_t queue_id = 0;

    if (iProc == root) {
        printf("%d \t", VLEN);
    }

    double *t_median = (double *) calloc(numIters, sizeof(double));
    for (int iter=0; iter < numIters; iter++) {
        double time = -now();

        gaspi_ring_allreduce(buffer_send, buffer_recv, buffer_temp, VLEN, GASPI_OP_SUM, GASPI_TYPE_DOUBLE, queue_id, GASPI_BLOCK);

        time += now();
        t_median[iter] = time;

        if (checkRes) {    
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

    test_ring_allreduce(VLEN, numIters, checkRes); 

    SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );

    return EXIT_SUCCESS;
}
