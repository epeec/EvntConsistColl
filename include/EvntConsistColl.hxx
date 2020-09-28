
#ifndef EVNT_CONSIST_COLL_H
#define EVNT_CONSIST_COLL_H

#include <GASPI.h>

#include "DataStruct.hxx"


/** Broadcast collective operation that is based on (n-1) straight gaspi_write
 *
 * @param buffer Segment with offset of the original data
 * @param elem_cnt The number of data elements in the buffer
 * @param type Type of data (see gaspi_datatype_t)
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t
gaspi_bcast_simple (segmentBuffer const buffer,
                    const gaspi_number_t elem_cnt,
                    const gaspi_datatype_t type,
                    const gaspi_number_t root,
                    const gaspi_queue_id_t queue_id,
                    const gaspi_timeout_t timeout_ms);

/** Weakly consistent broadcast collective operation that is based on (n-1) straight gaspi_write
 *
 * @param buffer Segment with offset of the original data
 * @param elem_cnt The number of data elements in the buffer
 * @param type Type of data (see gaspi_datatype_t)
 * @param threshold The threshol for the amount of data to be broadcasted. The value is in [0, 1]
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t
gaspi_bcast_simple (segmentBuffer const buffer,
                    const gaspi_number_t elem_cnt,
                    const gaspi_datatype_t type,
                    const gaspi_double threshold,
                    const gaspi_number_t root,
                    const gaspi_queue_id_t queue_id,
                    const gaspi_timeout_t timeout_ms);

// structure for binomial tree-based algorithms
typedef struct{
    gaspi_rank_t parent;
    gaspi_rank_t *children;
    gaspi_rank_t children_count = 0;
    bool isactive;
} bst_struct;

/** Broadcast collective operation that uses binomial tree
 *
 * @param buffer Segment with offset of the original data
 * @param elem_cnt The number of data elements
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
template <typename T> gaspi_return_t 
gaspi_bcast (segmentBuffer const buffer,
	         const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout_ms);

/** Weakly consistent broadcast collective operation that uses binomial tree.
 *
 * @param buffer Segment with offset of the original data
 * @param elem_cnt The number of data elements
 * @param threshold The threshol for the amount of data to be broadcasted. The value is in [0, 1]
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
template <typename T> gaspi_return_t 
gaspi_bcast (segmentBuffer const buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout_ms);

/** Reduce collective operation that implements binomial tree
 *
 * @param buffer_send Segment with offset of the original data
 * @param buffer_receive Segment with offset of the reduced data
 * @param buffer_tmp Segment with offset of the temprorary part of data (~elem_cnt/nProc)
 * @param operation The type of operations (see gaspi_operation_t).
 * @param type Type of data (see gaspi_datatype_t).
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ms Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t 
gaspi_reduce (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
   	          segmentBuffer buffer_tmp,
              const gaspi_number_t elem_cnt,
              const gaspi_operation_t operation,
              const gaspi_datatype_t type,
              const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
              const gaspi_timeout_t timeout_ms);

/** Weakly consistent reduce collective operation that implements binomial tree
 *
 * @param buffer_send Segment with offset of the original data
 * @param buffer_receive Segment with offset of the reduced data
 * @param buffer_tmp Segment with offset of the temprorary part of data (~elem_cnt/nProc)
 * @param elem_cnt The number of data elements in the buffer (beware of maximum - use gaspi_allreduce_elem_max).
 * @param operation The type of operations (see gaspi_operation_t).
 * @param type Type of data (see gaspi_datatype_t).
 * @param threshold The threshol for the amount of data to be reduced. The value is in [0, 1]
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ms Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t 
gaspi_reduce (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
   	          segmentBuffer buffer_tmp,
              const gaspi_number_t elem_cnt,
              const gaspi_operation_t operation,
              const gaspi_datatype_t type,
              const gaspi_double threshold,
              const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
              const gaspi_timeout_t timeout_ms);

#endif //#define EVNT_CONSIST_COLL_H
