
#ifndef DATA_STRUCTS_AND_OPS_H
#define DATA_STRUCTS_AND_OPS_H

#include <GASPI.h>

#define MAX(a,b)  (((a)<(b)) ? (b) : (a))
#define MIN(a,b)  (((a)>(b)) ? (b) : (a))

/**
 * Easy to read operations
 */
enum Operation { SUM
                , MAX
                , MIN };

// structure for segment and offset
struct segmentBuffer {
    gaspi_segment_id_t segment;
    gaspi_offset_t offset;
};

// structure for binomial tree-based algorithms
typedef struct{
    gaspi_rank_t parent;
    gaspi_rank_t *children;
    gaspi_rank_t children_count = 0;
    bool isactive;
} bst_struct;

// reduction functions
template <typename T>
void local_reduce_min(const unsigned int size, T const *input, T *output)
{
    for (unsigned int j = 0; j < size; j++) {
        output[j] = MIN(input[j], output[j]);
    }
}

template <typename T>
void local_reduce_max(const unsigned int size, T const *input, T *output)
{
    for (unsigned int j = 0; j < size; j++) {
        output[j] = MAX(input[j], output[j]);
    }
}

template <typename T>
void local_reduce_sum(const unsigned int size, T const *input, T *output)
{
    for (unsigned int j = 0; j < size; j++) {
        output[j] += input[j];
    }
}

/** 
 * Local reduce
 */
template <typename T>
void local_reduce(const Operation & op, const unsigned int size, T const *input, T *output)
{
    switch (op) {
        case MIN: {
            local_reduce_min<T>(size, input, output);
            break;
        }

        case MAX: {
            local_reduce_max<T>(size, input, output);
            break;
        }

        case SUM: {
            local_reduce_sum<T>(size, input, output);
            break;
        }

        default: {
            throw std::runtime_error ("Unsupported Operation");
        }
    }
}

#endif //#define DATA_STRUCTS_AND_OPS_H
