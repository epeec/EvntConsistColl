
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

/** Broadcast collective operation that is based on (n-1) writes.
 *
 * @param buffer Segment with offset of the original data
 * @param elem_cnt The number of data elements in the buffer
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
template <typename T> gaspi_return_t 
gaspi_bcast_simple (segmentBuffer const buffer,
                    const gaspi_number_t elem_cnt,
                    const gaspi_number_t root,
                    const gaspi_queue_id_t queue_id,
                    const gaspi_timeout_t timeout)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    // type size
    int type_size = sizeof(T);

    int segment_size = elem_cnt * type_size;

    gaspi_number_t doffset = buffer.offset * type_size;
  
    if (iProc == root) {	
	    for(uint k = 0; k < nProc; k++) {
    		if (k == root) 
	    		continue;

            gaspi_notification_id_t data_available = k;
            write_notify_and_wait( buffer.segment, doffset, k
			          , buffer.segment, doffset, segment_size
			          , data_available, k+1 // +1 so that the value is not zero
			          , queue_id, timeout
			);
	    }
    } else {
        gaspi_notification_id_t data_available = iProc;
  	    wait_or_die( buffer.segment, data_available, iProc+1 );  
        // ackowledge parent that the data has arrived
        gaspi_notification_id_t id = nProc + iProc + 1;
        notify_and_wait( buffer.segment
                , root, id, iProc+1
                , queue_id, timeout
        );
    }

    // wait for acknowledgement notifications 
    if (iProc == root) {	
	    for(uint k = 0; k < nProc; k++) {
	    	if (k == root) 
		    	continue;
            gaspi_notification_id_t id = nProc + k + 1;
  	        wait_or_die( buffer.segment, id, k+1 );
        } 
    }

    return GASPI_SUCCESS;
}

/** Weakly consistent broadcast collective operation that is based on (n-1) straight gaspi_write
 *
 * @param buffer Segment with offset of the original data
 * @param elem_cnt The number of data elements in the buffer
 * @param threshold The threshold for the amount of data to be broadcasted. The value is in [0, 1]
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout_ws Time out: ms, GASPI_BLOCK or GASPI_TEST
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
template <typename T> gaspi_return_t 
gaspi_bcast_simple (segmentBuffer const buffer,
                    const gaspi_number_t elem_cnt,
                    const gaspi_double threshold,
                    const gaspi_number_t root,
                    const gaspi_queue_id_t queue_id,
                    const gaspi_timeout_t timeout)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    // type size
    int type_size = sizeof(T);

    int segment_size = ceil(elem_cnt * threshold) * type_size;

    gaspi_number_t doffset = buffer.offset * type_size;
 
    if (iProc == root) {	
	    for(uint k = 0; k < nProc; k++) {
	    	if (k == root) 
		    	continue;

            gaspi_notification_id_t data_available = k;
			write_notify_and_wait( buffer.segment, doffset, k
			        , buffer.segment, doffset, segment_size
			        , data_available, root+1 // +1 so that the value is not zero
			        , queue_id, timeout
			);
	    }
    } else {
        gaspi_notification_id_t data_available = iProc;
    	wait_or_die( buffer.segment, data_available, root+1 );  

        // ackowledge parent that the data has arrived
        gaspi_notification_id_t id = nProc + iProc + 1;
        notify_and_wait( buffer.segment
                , root, id, iProc+1
                , queue_id, timeout
        );
    }

    // wait for acknowledgement notifications 
    if (iProc == root) {	
	    for(uint k = 0; k < nProc; k++) {
	    	if (k == root) 
		    	continue;
            gaspi_notification_id_t id = nProc + k + 1;
  	        wait_or_die( buffer.segment, id, k+1 );
        } 
    }

    return GASPI_SUCCESS;
}

/** Broadcast collective operation that uses binomial tree.
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
gaspi_bcast (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    // type size
    int type_size = sizeof(T);

    int segment_size = elem_cnt * type_size;

    gaspi_number_t doffset = buffer.offset * type_size;

    // compute parent
    // this can be omitted as the parent of each process can be found by flipping the leftmost 1-bit of its ID
    int j = 1;
    while (j <= iProc)
        j = j * 2;
    int parent = iProc - j / 2;

    // broadcast
    int upper_bound = ceil(log2(nProc));
    for (int i = 0; i < upper_bound; i++) {
        int pow2i = 1 << i;
        if (iProc < pow2i) {
            int dst = iProc + pow2i;
            if (dst < nProc) {
                // wait for notification that the data can be sent
                gaspi_notification_id_t id = dst;
                wait_or_die( buffer.segment, id, dst );  

                // send the data
                gaspi_notification_id_t data_available = iProc * nProc + dst;
                write_notify_and_wait( buffer.segment, doffset, dst
                        , buffer.segment, doffset, segment_size
                        , data_available, iProc + 1// +1 so that the value is not zero
                        , queue_id, timeout 
                );
            }
        } else if ((1 << (i+1)) > iProc) {

            // need to send notification that the child is ready to receive the data
            gaspi_notification_id_t id = iProc;
            gaspi_notification_t val = iProc;
            notify_and_wait(buffer.segment
                    , parent, id, val
                    , queue_id, timeout
            );

            // wait for data to arrive
            gaspi_notification_id_t data_available = parent * nProc + iProc;
  	        wait_or_die( buffer.segment, data_available, parent+1 );  
          
            if (i == (upper_bound - 1)) {
                // ackowledge parent that the data has arrived
                gaspi_notification_id_t id = iProc * nProc + parent;
                gaspi_notification_t val = iProc;
                notify_and_wait(buffer.segment
                        , parent, id, val
                        , queue_id, timeout
                );
            }
        }
    }

    // waiting for acknowledgement notifications from children (only on the leaves)
    int pow2i = 1 << (upper_bound - 1);
    if (iProc < pow2i) {
        int src = iProc + pow2i;
        if (src < nProc) {
            gaspi_notification_id_t id = src * nProc + iProc;
  	        wait_or_die( buffer.segment, id, src );  
        }
    }
 
    return GASPI_SUCCESS;
}

/** Weakly consistent broadcast collective operation that uses binomial tree.
 *
 * @param buffer Segment with offset of the original data
 * @param elem_cnt The number of data elements in the buffer
 * @param threshold The threshold for the amount of data to be broadcasted. The value is in [0, 1]
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
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
             const gaspi_timeout_t timeout)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    // type size
    int type_size = sizeof(T);

    int segment_size = ceil(elem_cnt * threshold) * type_size;

    gaspi_number_t doffset = buffer.offset * type_size;

    // compute parent
    // this can be omitted as the parent of each process can be found by flipping the leftmost 1-bit of its ID
    int j = 1;
    while (j <= iProc)
        j = j * 2;
    int parent = iProc - j / 2;

    // broadcast
    int upper_bound = ceil(log2(nProc));
    for (int i = 0; i < upper_bound; i++) {
        int pow2i = 1 << i;
        if (iProc < pow2i) {
            int dst = iProc + pow2i;
            if (dst < nProc) {
                // wait for notification that the data can be sent
                gaspi_notification_id_t id = dst;
                wait_or_die( buffer.segment, id, dst );  

                // send the data
                gaspi_notification_id_t data_available = iProc * nProc + dst;
                write_notify_and_wait( buffer.segment, doffset, dst
                        , buffer.segment, doffset, segment_size
                        , data_available, iProc+1 // +1 so that the value is not zero
                        , queue_id, timeout
                );
            }
        } else if ((1 << (i+1)) > iProc) {

            // need to send notification that the child is ready to receive the data
            gaspi_notification_id_t id = iProc;
            gaspi_notification_t val = iProc;
            notify_and_wait(buffer.segment
                    , parent, id, val
                    , queue_id, timeout
            );

            // wait for data to arrive
            gaspi_notification_id_t data_available = parent * nProc + iProc;
  	        wait_or_die( buffer.segment, data_available, parent+1 );  
          
            if (i == (upper_bound - 1)) {
                // ackowledge parent that the data has arrived
                gaspi_notification_id_t id = iProc * nProc + parent;
                gaspi_notification_t val = iProc;
                notify_and_wait(buffer.segment
                        , parent, id, val
                        , queue_id, timeout
                );
            }
        }
    }

    // waiting for acknowledgement notificaitons from children (only on the leaves)
    int pow2i = 1 << (upper_bound - 1);
    if (iProc < pow2i) {
        int src = iProc + pow2i;
        if (src < nProc) {
            gaspi_notification_id_t id = src * nProc + iProc;
  	        wait_or_die( buffer.segment, id, src );  
        }
    }

    return GASPI_SUCCESS;
}

/** Reduce collective operation that implements binomial tree
 *
 * @param buffer_send Segment with offset of the original data
 * @param buffer_receive Segment with offset of the reduced data
 * @param elem_cnt The number of data elements in the buffer (beware of maximum - use gaspi_allreduce_elem_max).
 * @param operation The type of operations (MIN, MAX, SUM)
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
// TODO: I assume that we deal with all ranks [0,n-1] however it would be better to have them as in gaspi in rank_grp array
template <typename T> gaspi_return_t 
gaspi_reduce (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    // type size
    int type_size = sizeof(T);

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
    gaspi_pointer_t src_array, rcv_array;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send.segment, &src_array) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_receive.segment, &rcv_array) );
    T *src_arr = (T *)((char*)src_array + buffer_send.offset);
    T *rcv_arr = (T *)((char*)rcv_array + buffer_receive.offset);

    // add temporary buffer
    T *tmp_arr = NULL;
    
    // Copy the data to the output buffer to avoid modifying the input buffer
    if (!children_count)
        std::memcpy((void*) rcv_arr, (void*) src_arr, segment_size);
    else {
        tmp_arr = (T *) malloc(segment_size);
        std::memcpy((void*) tmp_arr, (void*) src_arr, segment_size);
    }

    // actual reduction
    for (int i = upper_bound - 1; i >= 0; i--) {
        int pow2i = 1 << i;
        if (bst.isactive && (children_count == 0) && (pow2i <= iProc) && (iProc < (1 << (i+1)))) {
            // wait for notification that the data can be sent
            gaspi_rank_t rank = iProc * nProc + bst.parent;
            gaspi_notification_id_t id = rank;
            wait_or_die( buffer_send.segment, id, rank );

            // write the data to the parent
            gaspi_notification_id_t data_available = iProc;
            write_notify_and_wait( buffer_receive.segment, buffer_receive.offset, bst.parent
                    , buffer_receive.segment, buffer_receive.offset, segment_size
                    , data_available, bst.parent + 1 // +1 so that the value is not zero
                    , queue_id, timeout
            );
            
            // wait for acknowledgement notification
            gaspi_notification_id_t ack = bst.parent + 1;
            wait_or_die( buffer_send.segment, ack, bst.parent + 1 );  

            bst.isactive = false;

            if (tmp_arr)
                free(tmp_arr);

        } else if (bst.isactive && (pow2i > iProc) && ((iProc + pow2i) < nProc)) {

            // need to send notification that the parent is ready to receive the data
            gaspi_rank_t rank = bst.children[children_count-1] * nProc + iProc;        
            gaspi_notification_id_t id = rank;
            notify_and_wait(buffer_send.segment
                    , bst.children[children_count-1], id, rank
                    , queue_id, timeout
            );
        
            // receive data
            gaspi_notification_t val;
            waitsome_and_reset(buffer_receive.segment
                , bst.children[0], nProc - bst.children[0]
                , &id, &val
            );
            ASSERT(id >= bst.children[0]);
            ASSERT(id <= bst.children[bst.children_count-1]);

            // local reduce
            local_reduce<T>(op, elem_cnt, &rcv_arr[0], &tmp_arr[0]);

            // ackowledge child that the data has arrived
            gaspi_notification_id_t ack = iProc + 1;
            notify_and_wait(buffer_send.segment
                    , bst.children[children_count - 1], ack, iProc + 1
                    , queue_id, timeout
            );

            children_count--;

            // copy results to the receive buffer
            if (!children_count)
                std::memcpy((void*) rcv_arr, (void*) tmp_arr, segment_size);
        }
    }

    return GASPI_SUCCESS;
}

/** Weakly consistent reduce collective operation that implements binomial tree
 *
 * @param buffer_send Segment with offset of the original data
 * @param buffer_receive Segment with offset of the reduced data
 * @param elem_cnt The number of data elements in the buffer (beware of maximum - use gaspi_allreduce_elem_max).
 * @param operation The type of operations (MIN, MAX, SUM)
 * @param threshold The threshold for the amount of data to be reduced. The value is in [0, 1]
 * @param root The process id of the root
 * @param queue_id The queue id
 * @param timeout Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
// TODO: I assume that we deal with all ranks [0,n-1] however it would be better to have them as in gaspi in rank_grp array
template <typename T> gaspi_return_t 
gaspi_reduce (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
              const gaspi_double threshold,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout)
{
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );

    if (nProc <= 1)
        return GASPI_SUCCESS;

    // type size
    int type_size = sizeof(T);

    int num_elem = ceil(elem_cnt * threshold);
    int segment_size = num_elem * type_size;

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
    gaspi_pointer_t src_array, rcv_array;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send.segment, &src_array) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_receive.segment, &rcv_array) );
    T *src_arr = (T *)((char*)src_array + buffer_send.offset);
    T *rcv_arr = (T *)((char*)rcv_array + buffer_receive.offset);

    // add temporary buffer
    T *tmp_arr = NULL;

    // Copy the data to the output buffer to avoid modifying the input buffer
    if (!children_count)
        std::memcpy((void*) rcv_arr, (void*) src_arr, segment_size);
    else {
        tmp_arr = (T *) malloc(segment_size);
        std::memcpy((void*) tmp_arr, (void*) src_arr, segment_size);
    }

    // actual reduction
    for (int i = upper_bound - 1; i >= 0; i--) {
        int pow2i = 1 << i;
        if (bst.isactive && (children_count == 0) && (pow2i <= iProc) && (iProc < (1 << (i+1)))) {
            // wait for notification that the data can be sent
            gaspi_rank_t rank = iProc * nProc + bst.parent;
            gaspi_notification_id_t id = rank;
            wait_or_die( buffer_send.segment, id, rank );

            // write the data to the parent
            gaspi_notification_id_t data_available = iProc;
            write_notify_and_wait( buffer_receive.segment, buffer_receive.offset, bst.parent
                    , buffer_receive.segment, buffer_receive.offset, segment_size
                    , data_available, bst.parent + 1 // +1 so that the value is not zero
                    , queue_id, timeout
            );
            
            // wait for acknowledgement notification
            gaspi_notification_id_t ack = bst.parent + 1;
            wait_or_die( buffer_send.segment, ack, bst.parent + 1 );  

            bst.isactive = false;

            if (tmp_arr)
                free(tmp_arr);

        } else if (bst.isactive && (pow2i > iProc) && ((iProc + pow2i) < nProc)) {

            // need to send notification that the parent is ready to receive the data
            gaspi_rank_t rank = bst.children[children_count-1] * nProc + iProc;        
            gaspi_notification_id_t id = rank;
            notify_and_wait(buffer_send.segment
                    , bst.children[children_count-1], id, rank
                    , queue_id, timeout
            );
        
            // receive data
            gaspi_notification_t val;
            waitsome_and_reset(buffer_receive.segment
                , bst.children[0], nProc - bst.children[0]
                , &id, &val
            );
            ASSERT(id >= bst.children[0]);
            ASSERT(id <= bst.children[bst.children_count-1]);

            // local reduce
            local_reduce<T>(op, num_elem, &rcv_arr[0], &tmp_arr[0]);

            // ackowledge child that the data has arrived
            gaspi_notification_id_t ack = iProc + 1;
            notify_and_wait(buffer_send.segment
                    , bst.children[children_count - 1], ack, iProc + 1
                    , queue_id, timeout
            );

            children_count--;

            // copy results to the receive buffer
            if (!children_count)
                std::memcpy((void*) rcv_arr, (void*) tmp_arr, segment_size);
        }
    }

    return GASPI_SUCCESS;
}

// explicit template instantiation
// consistent bcast
template gaspi_return_t 
gaspi_bcast<double> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast<float> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast<int> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast<unsigned int> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

// weakly consistent bcast
template gaspi_return_t 
gaspi_bcast<double> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast<float> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast<int> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast<unsigned int> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

// weakly consistent bcast -- simple version
template gaspi_return_t 
gaspi_bcast_simple<double> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast_simple<float> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast_simple<int> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast_simple<unsigned int> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

// consistent bcast -- simple version
template gaspi_return_t 
gaspi_bcast_simple<double> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast_simple<float> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast_simple<int> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_bcast_simple<unsigned int> (segmentBuffer buffer,
             const gaspi_number_t elem_cnt,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
             const gaspi_timeout_t timeout);

// weakly consistent reduce
template gaspi_return_t 
gaspi_reduce<double> (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
              const gaspi_double threshold,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_reduce<float> (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
              const gaspi_double threshold,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_reduce<int> (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
              const gaspi_double threshold,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_reduce<unsigned int> (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
              const gaspi_double threshold,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout);

// consistent reduce
template gaspi_return_t 
gaspi_reduce<double> (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_reduce<float> (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_reduce<int> (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_reduce<unsigned int> (const segmentBuffer buffer_send,
   	          segmentBuffer buffer_receive,
	          const gaspi_number_t elem_cnt,
              const Operation & op,
	          const gaspi_number_t root,
              const gaspi_queue_id_t queue_id,
	          const gaspi_timeout_t timeout);

