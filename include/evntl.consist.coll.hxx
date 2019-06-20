
#ifndef EVNTL_CONSIST_COLL_H
#define EVNTL_CONSIST_COLL_H

#include <GASPI.h>

/** Broadcast collective operation that is based on (n-1) straight gaspi_write
 *
 *
 * @param buf The segment with data for the operation
 * @param offset The offset within the segment
 * @param elem_cnt The number of data elements
 * @param datatyp Type of data (see gaspi_datatype_t)
 * @param root The process id of the root
 * @param queue_id The queue id
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t
gaspi_bcast (gaspi_segment_id_t const buf,
	     const gaspi_number_t offset,
	     const gaspi_number_t elem_cnt,
	     const gaspi_datatype_t type,
	     const gaspi_number_t root,
         const gaspi_queue_id_t queue_id);
	     //const gaspi_timeout_t timeout_ms);
	     //const gaspi_group_t g,

// structure for binomial algs
typedef struct{
    gaspi_rank_t parent;
    gaspi_rank_t *children;
    gaspi_rank_t children_count = 0;
    bool isactive;
} bst_struct;

/** Broadcast collective operation that uses binomial tree.
 *
 *
 * @param buf The segment with data for the operation
 * @param elem_cnt The number of data elements
 * @param datatyp Type of data (see gaspi_datatype_t)
 * @param root The process id of the root
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
//gaspi_return_t
//gaspi_bcast (gaspi_pointer_t const buf,
//	     const gaspi_number_t elem_cnt,
//	     const gaspi_datatype_t type,
//	     const gaspi_number_t root,
//	     const gaspi_timeout_t timeout_ms);
gaspi_return_t
gaspi_bcast_bst (gaspi_segment_id_t const buf,
	     gaspi_number_t const offset,
	     const gaspi_number_t elem_cnt,
	     const gaspi_datatype_t type,
	     const gaspi_number_t root,
	     const gaspi_timeout_t timeout_ms);

/** Eventually consistent broadcast collective operation that is based on (n-1) straight gaspi_write
 *
 *
 * @param buf The segment with data for the operation
 * @param offset The offset within the segment
 * @param elem_cnt The number of data elements
 * @param datatyp Type of data (see gaspi_datatype_t)
 * @param threshol The threshol for the amount of data to be broadcasted. The value is in [0, 1]
 * @param root The process id of the root
 * @param queue_id The queue id
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t
gaspi_bcast (gaspi_segment_id_t const buf,
	     const gaspi_number_t offset,
	     const gaspi_number_t elem_cnt,
	     const gaspi_datatype_t type,
	     const gaspi_double threshold,
	     const gaspi_number_t root,
         const gaspi_queue_id_t queue_id);

/** Eventually consistent broadcast collective operation that uses binomial tree.
 *
 *
 * @param buf The segment with data for the operation
 * @param offset The offset within the segment
 * @param elem_cnt The number of data elements
 * @param datatyp Type of data (see gaspi_datatype_t)
 * @param threshol The threshol for the amount of data to be broadcasted. The value is in [0, 1]
 * @param root The process id of the root
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t
gaspi_bcast_bst (gaspi_segment_id_t const buf,
	     gaspi_number_t const offset,
	     const gaspi_number_t elem_cnt,
	     const gaspi_datatype_t type,
	     const gaspi_double threshold,
	     const gaspi_number_t root,
	     const gaspi_timeout_t timeout_ms);

/** Reduce collective operation that implements binomial tree.
 *
 *
 * @param buffer_send The buffer with data for the operation.
 * @param offset The offset within the segment
 * @param buffer_receive The buffer to receive the result of the operation.
 * @param offset The offset within the segment
 * @param elem_cnt The number of data elements in the buffer (beware of maximum - use gaspi_allreduce_elem_max).
 * @param operation The type of operations (see gaspi_operation_t).
 * @param datatype Type of data (see gaspi_datatype_t).
 * @param root The process id of the root
 * @param group The group involved in the operation.
 * @param timeout_ms Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t 
gaspi_reduce (const gaspi_segment_id_t buffer_send,
	      gaspi_number_t const offset_send,
   	      gaspi_segment_id_t const buffer_receive,
	      gaspi_number_t const offset_recv,
	      const gaspi_number_t elem_cnt,
	      const gaspi_operation_t operation,
	      const gaspi_datatype_t datatype,
	      const gaspi_number_t root,
	      const gaspi_group_t group,
	      const gaspi_timeout_t timeout_ms);

#endif //#define EVNTL_CONSIST_COLL_H