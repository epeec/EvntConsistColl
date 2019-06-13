

#ifndef CONSISTENT_COLLECTIVES_H
#define CONSISTENT_COLLECTIVES_H

#include <GASPI.h>

  /** Broadcast collective operation.
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

  /** Broadcast collective operation.
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

  /** Reduce collective operation.
   *
   *
   * @param buffer_send The buffer with data for the operation.
   * @param buffer_receive The buffer to receive the result of the operation.
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
gaspi_reduce (const gaspi_pointer_t buffer_send,
   	      gaspi_pointer_t const buffer_receive,
	      const gaspi_number_t elem_cnt,
	      const gaspi_operation_t operation,
	      const gaspi_datatype_t datatype,
	      const gaspi_number_t root,
	      const gaspi_group_t group,
	      const gaspi_timeout_t timeout_ms);

#endif //#define CONSISTENT_COLLECTIVES_H
