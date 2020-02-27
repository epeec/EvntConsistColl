
#include <mpi.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

template <class T> void fill_array(T a[],
         const int n) {
    int iProc;
    MPI_Comm_rank(MPI_COMM_WORLD, &iProc);

    for (int i=0; i < n; i++) {
        a[i] = i + iProc + 1;
    }
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

    const int type_size = sizeof (double);

    double * src_arr = (double *) calloc(VLEN, type_size);
    double * rcv_arr = (double *) calloc(VLEN, type_size);

    fill_array(src_arr, VLEN);

    MPI_Barrier(MPI_COMM_WORLD);

    timeval timeStart, timeStop;
    gettimeofday(&timeStart, NULL);

    for (int iter=0; iter < numIters; iter++) {
        MPI_Allreduce(src_arr, rcv_arr, VLEN, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    gettimeofday(&timeStop, NULL);

    if (checkRes) {    
        check(VLEN, rcv_arr);
    }

    if (iProc == 0) {
        const double numGigaBytes
            = 2. * VLEN * type_size * (nProc - 1) / nProc  * numIters
             / 1024. / 1024. / 1024.;
        const double seconds = timeStop.tv_sec - timeStart.tv_sec
                           + 1e-6 * (timeStop.tv_usec - timeStart.tv_usec);
        std::cout << "Total runtime " << seconds << " seconds, "
                << seconds / numIters << " seconds per reduce, "
                << numGigaBytes / seconds << " GiB/s." << std::endl;
    }
}


int main(int argc, char** argv) {

    if ((argc < 3) || (argc > 4)) {
      std::cerr << argv[0] << ": Usage " << argv[0] << " <length in bytes>"
                << " <num iterations> [check]"
                << std::endl;
      return -1;
    }

    static const int VLEN = atoi(argv[1]) / sizeof(double);
    const int numIters = atoi(argv[2]);
    const bool checkRes = (argc==4)?true:false;

    MPI_Init(&argc, &argv);

    test_allreduce(VLEN, numIters, checkRes); 

    MPI_Finalize();

    return EXIT_SUCCESS;
}
