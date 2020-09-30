
#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <GASPI.h>

#define MAX(a,b)  (((a)<(b)) ? (b) : (a))
#define MIN(a,b)  (((a)>(b)) ? (b) : (a))

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

#endif //#define DATA_STRUCT_H
