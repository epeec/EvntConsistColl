
#include <cmath>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <cstring>

#include <EvntConsistColl.hxx>

#include "success_or_die.h"
#include "testsome.h"
#include "queue.h"
#include "waitsome.h"
#include "wait_if_queue_full.h"

/** Broadcast collective operation.
 *
 * @param buf The segment with data for the operation
 * @param offset The offset within the segment
 * @param elem_cnt The number of data elements
 * @param type Type of data (see gaspi_datatype_t)
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t
gaspi_bcast_simple (gaspi_segment_id_t const buf,
                    gaspi_number_t const offset,
                    gaspi_number_t const elem_cnt,
                    const gaspi_datatype_t type,
                    const gaspi_number_t root,
                    const gaspi_queue_id_t queue_id,
                    const gaspi_timeout_t timeout_ms)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    gaspi_notification_id_t data_available = 0;

    // get size of type, see GASPI.h for details
    gaspi_number_t type_size = 0;
    if (type >= 3) 
    	type_size = 8;
    else
	    type_size = 4;
    gaspi_number_t doffset = offset * type_size;
  
    if (iProc == root) {	
	    for(uint k = 0; k < nProc; k++) {
    		if (k == root) 
	    		continue;

	    	WAIT_IF_QUEUE_FULL( 
                gaspi_write_notify(buf, doffset, k
			          , buf, doffset, elem_cnt * type_size
			          , data_available, k+1 // +1 so that the value is not zero
			          , queue_id, timeout_ms
                )
                , queue_id
			);
	    }
    } else {
  	    wait_or_die( buf, data_available, iProc+1 );  
    }

    return GASPI_SUCCESS;
}

/** Eventually consistent broadcast collective operation that is based on (n-1) straight gaspi_write
 *
 * @param buf The segment with data for the operation
 * @param offset The offset within the segment
 * @param elem_cnt The number of data elements
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
gaspi_bcast_simple (gaspi_segment_id_t const buf,
                    gaspi_number_t const offset,
                    gaspi_number_t const elem_cnt,
                    const gaspi_datatype_t type,
                    const gaspi_double threshold,
                    const gaspi_number_t root,
                    const gaspi_queue_id_t queue_id,
                    const gaspi_timeout_t timeout_ms)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    gaspi_notification_id_t data_available = 0;

    // get size of type, see GASPI.h for details
    gaspi_number_t type_size = 0;
    if (type >= 3) 
    	type_size = 8;
    else
	    type_size = 4;
    gaspi_number_t doffset = offset * type_size;
 
    if (iProc == root) {	
	    for(uint k = 0; k < nProc; k++) {
	    	if (k == root) 
		    	continue;

	    	WAIT_IF_QUEUE_FULL( 
			    gaspi_write_notify(buf, doffset, k
			        , buf, doffset, ceil(elem_cnt * threshold) * type_size
			        , data_available, root+1 // +1 so that the value is not zero
			        , queue_id, timeout_ms
                )
                , queue_id
			);
	    }
    } else {
    	wait_or_die( buf, data_available, root+1 );  
    }

    return GASPI_SUCCESS;
}

/** Broadcast collective operation that uses binomial tree.
 *
 * @param buffer Segment with offset of the original data
 * @param elem_cnt The number of data elements
 * @param type Type of data (see gaspi_datatype_t)
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t
gaspi_bcast (segmentBuffer const buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_datatype_t type,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout_ms)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    gaspi_notification_id_t data_available = 0;

    // get size of type, see GASPI.h for details
    gaspi_number_t type_size = 0;
    if (type >= 3) 
    	type_size = 8;
    else
	    type_size = 4;
    gaspi_number_t doffset = buffer.offset * type_size;

    // compute parent
    // this can be omitted as the parent of each process can be found by flipping the leftmost 1-bit of its ID
    int j = 1;
    while (j <= iProc)
        j = j * 2;
    int parent = iProc - j / 2;

    int upper_bound = ceil(log2(nProc));
    for (int i = 0; i < upper_bound; i++) {
        int pow2i = 1 << i;
        if (iProc < pow2i) {
            int dst = iProc + pow2i;
            if (dst < nProc) {
	    	    WAIT_IF_QUEUE_FULL( 
                    gaspi_write_notify( buffer.segment, doffset, dst
                        , buffer.segment, doffset, elem_cnt * type_size
                        , data_available, iProc+1 // +1 so that the value is not zero
                        , queue_id, timeout_ms 
                    )
                    , queue_id
                );
            }
        } else if ((1 << (i+1)) > iProc) {
  	        wait_or_die( buffer.segment, data_available, parent+1 );  
        }
    }

    // TODO: how to eliminate this?
    gaspi_barrier(GASPI_GROUP_ALL, timeout_ms);
 
    return GASPI_SUCCESS;
}

/** Eventually consistent broadcast collective operation that uses binomial tree.
 *
 * @param buffer Segment with offset of the original data
 * @param elem_cnt The number of data elements in the buffer (beware of maximum - use gaspi_allreduce_elem_max).
 * @param operation The type of operations (see gaspi_operation_t).
 * @param type Type of data (see gaspi_datatype_t).
 * @param threshold The threshol for the amount of data to be broadcasted. The value is in [0, 1]
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ms Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
gaspi_return_t
gaspi_bcast (segmentBuffer const buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_datatype_t type,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout_ms)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    gaspi_notification_id_t data_available = 0;

    // get size of type, see GASPI.h for details
    gaspi_number_t type_size = 0;
    if (type >= 3) 
    	type_size = 8;
    else
	    type_size = 4;
    gaspi_number_t doffset = buffer.offset * type_size;

    // compute parent
    // this can be omitted as the parent of each process can be found by flipping the leftmost 1-bit of its ID
    int j = 1;
    while (j <= iProc)
        j = j * 2;
    int parent = iProc - j / 2;

    int upper_bound = ceil(log2(nProc));
    for (int i = 0; i < upper_bound; i++) {
        int pow2i = 1 << i;
        if (iProc < pow2i) {
            int dst = iProc + pow2i;
            if (dst < nProc) {
	    	    WAIT_IF_QUEUE_FULL( 
                    gaspi_write_notify( buffer.segment, doffset, dst
                        , buffer.segment, doffset, ceil(elem_cnt * threshold) * type_size
                        , data_available, iProc+1 // +1 so that the value is not zero
                        , queue_id, timeout_ms
                    )
                    , queue_id
                );
            }
        } else if ((1 << (i+1)) > iProc) {
  	        wait_or_die( buffer.segment, data_available, parent+1 );  
        }
    }

    // TODO: how to eliminate this?
    gaspi_barrier(GASPI_GROUP_ALL, timeout_ms);

    return GASPI_SUCCESS;
}

/** Reduce collective operation that implements binomial tree
 *
 * @param buffer_send Segment with offset of the original data
 * @param buffer_receive Segment with offset of the reduced data
 * @param buffer_tmp Segment with offset of the temprorary part of data (~elem_cnt/nProc)
 * @param elem_cnt The number of data elements in the buffer (beware of maximum - use gaspi_allreduce_elem_max).
 * @param operation The type of operations (see gaspi_operation_t).
 * @param type Type of data (see gaspi_datatype_t).
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ms Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
// TODO: I assume that we deal with all ranks [0,n-1] however it would be better to have them as in gaspi in rank_grp array
gaspi_return_t 
gaspi_reduce (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
   	          segmentBuffer buffer_tmp,
	          const gaspi_number_t elem_cnt,
	          const gaspi_operation_t operation,
	          const gaspi_datatype_t type,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout_ms)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    // get size of type, see GASPI.h for details
    gaspi_number_t type_size = 0;
    if (type >= 3) 
    	type_size = 8;
    else
	    type_size = 4;

    int segment_size = elem_cnt * type_size;

    // compute parent
    bst_struct bst;
    int j = 1;
    while (j <= iProc)
        j = j * 2;
    bst.parent = iProc - j / 2;
    bst.children = (gaspi_rank_t *) valloc(sizeof(gaspi_rank_t) * (ceil(log2(nProc)) - 1));
    bst.isactive = true;

    // compute children
    int children_count = 0;
    int upper_bound = ceil(log2(nProc));
    for (int i = 0; i < upper_bound; i++) {
        if ((iProc == root) || (i > log2(iProc))) {
            int k = iProc + (1 << i);
            if ( k < nProc ) {
                bst.children[children_count] = k;
                children_count++;
            }
        }
    }
    bst.children_count = children_count;

    // auxiliary pointers
    gaspi_pointer_t src_array, rcv_array, buf_array;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send.segment, &src_array) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_receive.segment, &rcv_array) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_tmp.segment, &buf_array) );
    double *src_arr = (double *)((char*)src_array + buffer_send.offset);
    double *rcv_arr = (double *)((char*)rcv_array + buffer_receive.offset);
    double *buf_arr = (double *)((char*)buf_array + buffer_tmp.offset);

    // Copy the data to the output buffer to avoid modifying the input buffer
    std::memcpy((void*) rcv_arr, (void*) src_arr, segment_size);

    // actual reduction
    for (int i = upper_bound - 1; i >= 0; i--) {
        int pow2i = 1 << i;
        if (bst.isactive && (children_count == 0) && (pow2i <= iProc) && (iProc < (1 << (i+1)))) {
            // wait for notification that the data can be sent
            gaspi_notification_id_t data_available = iProc;
            wait_or_die( buffer_receive.segment, data_available, bst.parent + 1 );  

            // send the data to the parent
            data_available = iProc;
	    	WAIT_IF_QUEUE_FULL( 
                gaspi_write_notify( buffer_receive.segment, buffer_receive.offset, bst.parent
                    , buffer_tmp.segment, buffer_tmp.offset, segment_size
                    , data_available, bst.parent+1 // +1 so that the value is not zero
                    , queue_id, timeout_ms
                )
                , queue_id
            );

            bst.isactive = false;

        } else if (bst.isactive && (pow2i > iProc) && ((iProc + pow2i) < nProc)) {

            // need to send notification that the parent is ready to receive the data
            gaspi_rank_t rank = bst.children[children_count-1];        
            gaspi_notification_id_t id = rank;
            gaspi_notification_t val = iProc + 1;

	    	WAIT_IF_QUEUE_FULL( 
                gaspi_notify(buffer_receive.segment
                    , rank, id, val
                    , queue_id, timeout_ms
                )
                , queue_id
            );
        
            // receive data
            val = iProc + 1;
            waitsome_and_reset(buffer_tmp.segment
                , bst.children[0], nProc - bst.children[0]
                , &id, &val
            );
            ASSERT(id >= bst.children[0]);
            ASSERT(id <= bst.children[bst.children_count-1]);

            // reduce
            for (j = 0; j < (int) elem_cnt; j++) {
                rcv_arr[j] += buf_arr[j];
            }

            children_count--;
        }

    }

    return GASPI_SUCCESS;
}

/** Eventually consistent reduce collective operation that implements binomial tree
 *
 * @param buffer_send Segment with offset of the original data
 * @param buffer_receive Segment with offset of the reduced data
 * @param buffer_tmp Segment with offset of the temprorary part of data (~elem_cnt/nProc)
 * @param elem_cnt The number of data elements in the buffer (beware of maximum - use gaspi_allreduce_elem_max).
 * @param operation The type of operations (see gaspi_operation_t).
 * @param datatype Type of data (see gaspi_datatype_t).
 * @param threshol The threshol for the amount of data to be reduced. The value is in [0, 1]
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ms Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
// TODO: I assume that we deal with all ranks [0,n-1] however it would be better to have them as in gaspi in rank_grp array
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
	          const gaspi_timeout_t timeout_ms)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    // get size of type, see GASPI.h for details
    gaspi_number_t type_size = 0;
    if (type >= 3) 
    	type_size = 8;
    else
	    type_size = 4;

    int segment_size = ceil(elem_cnt * threshold) * type_size;

    // compute parent
    bst_struct bst;
    int j = 1;
    while (j <= iProc)
        j = j * 2;
    bst.parent = iProc - j / 2;
    bst.children = (gaspi_rank_t *) valloc(sizeof(gaspi_rank_t) * (ceil(log2(nProc)) - 1));
    bst.isactive = true;

    // compute children
    int children_count = 0;
    int upper_bound = ceil(log2(nProc));
    for (int i = 0; i < upper_bound; i++) {
        if ((iProc == root) || (i > log2(iProc))) {
            int k = iProc + (1 << i);
            if ( k < nProc ) {
                bst.children[children_count] = k;
                children_count++;
            }
        }
    }
    bst.children_count = children_count;

    // auxiliary pointers
    gaspi_pointer_t src_array, rcv_array, buf_array;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send.segment, &src_array) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_receive.segment, &rcv_array) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_tmp.segment, &buf_array) );
    double *src_arr = (double *)((char*)src_array + buffer_send.offset);
    double *rcv_arr = (double *)((char*)rcv_array + buffer_receive.offset);
    double *buf_arr = (double *)((char*)buf_array + buffer_tmp.offset);

    // Copy the data to the output buffer to avoid modifying the input buffer
    std::memcpy((void*) rcv_arr, (void*) src_arr, segment_size);

    // actual reduction
    for (int i = upper_bound - 1; i >= 0; i--) {
        int pow2i = 1 << i;
        if (bst.isactive && (children_count == 0) && (pow2i <= iProc) && (iProc < (1 << (i+1)))) {
            // wait for notification that the data can be sent
            gaspi_notification_id_t data_available = iProc;
            wait_or_die( buffer_receive.segment, data_available, bst.parent + 1 );  

            // send the data to the parent
            data_available = iProc;
	    	WAIT_IF_QUEUE_FULL( 
                gaspi_write_notify( buffer_receive.segment, buffer_receive.offset, bst.parent
                    , buffer_tmp.segment, buffer_tmp.offset, segment_size
                    , data_available, bst.parent+1 // +1 so that the value is not zero
                    , queue_id, timeout_ms
                )
                , queue_id
            );
            bst.isactive = false;

        } else if (bst.isactive && (pow2i > iProc) && ((iProc + pow2i) < nProc)) {

            // need to send notification that the parent is ready to receive the data
            gaspi_rank_t rank = bst.children[children_count-1];        
            gaspi_notification_id_t id = rank;
            gaspi_notification_t val = iProc + 1;
	    	WAIT_IF_QUEUE_FULL( 
                gaspi_notify(buffer_receive.segment
                    , rank, id, val
                    , queue_id, timeout_ms
                )
                , queue_id
            );
        
            // receive data
            val = iProc+1;
            waitsome_and_reset(buffer_tmp.segment
                , bst.children[0], nProc - bst.children[0]
                , &id, &val
            );
            ASSERT(id >= bst.children[0]);
            ASSERT(id <= bst.children[bst.children_count-1]);

            // reduce
            for (j = 0; j < ceil(elem_cnt * threshold); j++) {
                rcv_arr[j] += buf_arr[j];
            }
                        
            children_count--;
        }
    }

    return GASPI_SUCCESS;
}

/** Segmented pipeline ring implementation
 *
 * @param buffer_send Segment with offset of the original data
 * @param buffer_receive Segment with offset of the reduced data
 * @param buffer_tmp Segment with offset of the temprorary part of data (~elem_cnt/nProc)
 * @param elem_cnt Number of data elements in the buffer
 * @param operation Type of operations (see gaspi_operation_t)
 * @param datatype Type of data (see gaspi_datatype_t)
 * @param queue_id Queue id
 * @param timeout_ms Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST)
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout
 */
gaspi_return_t 
gaspi_ring_allreduce (const segmentBuffer buffer_send,
   	                  segmentBuffer buffer_receive,
   	                  segmentBuffer buffer_tmp,
                      const gaspi_number_t elem_cnt,
                      const gaspi_operation_t operation,
                      const gaspi_datatype_t type,
                      const gaspi_queue_id_t queue_id,
                      const gaspi_timeout_t timeout_ms)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    // type size
    int type_size = sizeof(double);

    if (nProc <= 1)
        return GASPI_SUCCESS;

    // auxiliary pointers
    gaspi_pointer_t src_arr, rcv_arr;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send.segment, &src_arr) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_receive.segment, &rcv_arr) );
    double *src_array = (double *)((char*)src_arr + buffer_send.offset);
    double *rcv_array = (double *)((char*)rcv_arr + buffer_receive.offset);

    // Partition elements of array into nProc chunks
    const unsigned int segment_size = elem_cnt / nProc;
    std::vector<unsigned int> segment_sizes(nProc, segment_size);

    int segment_residual = elem_cnt % nProc;
    for (int i = 0; i < segment_residual; i++) 
        segment_sizes[i]++;

    // Compute where each chunk ends
    std::vector<unsigned int> segment_ends(nProc);
    segment_ends[0] = segment_sizes[0];
    for (int i = 1; i < nProc; i++) 
        segment_ends[i] = segment_sizes[i] + segment_ends[i-1];
   
    // The last segment should end at the end of segment
    ASSERT (segment_ends[nProc - 1] == elem_cnt);

    // Copy the data to the output buffer to avoid modifying the input buffer
    std::memcpy((void*) rcv_array, (void*) src_array, elem_cnt * type_size);

    // Allocate a temporary buffer to store incoming data
    gaspi_pointer_t buf_arr;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_tmp.segment, &buf_arr) );
    double *buf_array = (double *)((char*)buf_arr + buffer_tmp.offset);

    // Receive from left neighbor
    const int recv_from = (iProc - 1 + nProc) % nProc;

    // Send to righ`t neigbor
    const int send_to = (iProc + 1) % nProc;

    // notifications
    gaspi_notification_id_t notif = 0; 

    // scatter-reduce phase
    // At every step, for every rank, we iterate through
    // segments with wraparound and send and recv from our neighbors and reduce
    // locally. At the i'th iteration, iProc sends segment (rank - i) and receives
    // segment (rank - i - 1)
    for (int i = 0; i < nProc - 1; i++) {

        int recv_chunk = (iProc - i - 1 + nProc) % nProc;
        int send_chunk = (iProc - i + nProc) % nProc;

        int segment_start = segment_ends[send_chunk] - segment_sizes[send_chunk];

	    WAIT_IF_QUEUE_FULL( 
            gaspi_write_notify(buffer_receive.segment
                , buffer_receive.offset + segment_start * type_size // offset
                , send_to, buffer_tmp.segment, buffer_tmp.offset // offset
                , segment_sizes[send_chunk] * type_size, notif
                , i + iProc + 1 // notification value: +1 to avoid 0. It equals to recvfrom + 1 on receiver side
                , queue_id, GASPI_BLOCK
            )
            , queue_id
        );

        // wait for notification that the data arrived
        wait_or_die( buffer_tmp.segment, notif, i + recv_from + 1 );  

        // reduce
        segment_start = segment_ends[recv_chunk] - segment_sizes[recv_chunk];
        for (unsigned int j = 0; j < segment_sizes[recv_chunk]; j++) {
            rcv_array[segment_start + j] += buf_array[j];
        }

        // TODO: avoid this barrier
        gaspi_barrier(GASPI_GROUP_ALL, timeout_ms);
    }

    // pipelined ring allgather
    // At every step, for every rank, we iterate through
    // segments with wraparound and send and recv from our neighbors.
    // At the i'th iteration, iProc sends segment (rank + 1 - i) and receives
    // segment (rank - i)
    for (int i = 0; i < nProc-1; i++) {
        
        int send_chunk = (iProc - i + 1 + nProc) % nProc;
        int recv_chunk = (iProc - i + nProc) % nProc;

        int segment_start = segment_ends[send_chunk] - segment_sizes[send_chunk];

	    WAIT_IF_QUEUE_FULL( 
            gaspi_write_notify(buffer_receive.segment
                , buffer_receive.offset + segment_start * type_size // offset
                , send_to, buffer_tmp.segment, buffer_tmp.offset // offset 
                , segment_sizes[send_chunk] * type_size, notif
                , i + iProc + 1 // notification value: +1 to avoid 0. It equals to recvfrom + 1 on receiver side
                , queue_id, GASPI_BLOCK
            )
            , queue_id
        );

        // wait for notification that the data arrived
        wait_or_die( buffer_tmp.segment, notif, i + recv_from + 1 );  

        // copy
        segment_start = segment_ends[recv_chunk] - segment_sizes[recv_chunk];
        for (unsigned int j = 0; j < segment_sizes[recv_chunk]; j++) {
            rcv_array[segment_start + j] = buf_array[j];           
        }         

        // TODO: avoid this barrier
        gaspi_barrier(GASPI_GROUP_ALL, timeout_ms);
    }

    return GASPI_SUCCESS;
}
