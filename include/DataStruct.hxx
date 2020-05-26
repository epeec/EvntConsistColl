
#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <GASPI.h>

// structure for segment and offset
struct segmentBuffer {
    gaspi_segment_id_t segment;
    gaspi_offset_t offset;
};

#endif //#define DATA_STRUCT_H
