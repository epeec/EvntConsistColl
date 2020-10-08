
#ifndef ALLREDUCE_H
#define ALLREDUCE_H

#include <GASPI.h>

#include "DataStructsAndOps.hxx"

/** Segmented pipeline ring implementation
 *
 * @param buffer_send Segment with offset of the original data
 * @param buffer_receive Segment with offset of the reduced data
 * @param buffer_tmp Segment with offset of the temprorary part of data (~elem_cnt/nProc)
 * @param elem_cnt Number of data elements in the buffer
 * @param operation The type of operations (MIN, MAX, SUM)
 * @param datatype Type of data (see gaspi_datatype_t)
 * @param queue_id Queue id
 * @param timeout_ms Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST)
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout
 */
template <typename T> gaspi_return_t 
gaspi_ring_allreduce (const segmentBuffer buffer_send,
                      segmentBuffer buffer_receive,
                      const gaspi_number_t elem_cnt,
                      const Operation & op,
                      const gaspi_queue_id_t queue_id,
                      const gaspi_timeout_t timeout);

#endif // #define ALLREDUCE_H
