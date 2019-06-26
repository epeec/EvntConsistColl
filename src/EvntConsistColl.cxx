
#include <cmath>
#include <cstdlib>

#include <EvntConsistColl.hxx>

#include "success_or_die.h"
#include "testsome.h"
#include "queue.h"
#include "waitsome.h"

/** Broadcast collective operation.
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
             gaspi_number_t const offset,
             gaspi_number_t const elem_cnt,
             const gaspi_datatype_t type,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id)
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
  
    if (iProc == root) {	
	    for(uint k = 0; k < nProc; k++) {
    		if (k == root) 
	    		continue;

	    	SUCCESS_OR_DIE
			    ( gaspi_write_notify
			      ( buf, offset*type_size, k
			      , buf, offset*type_size, elem_cnt*type_size
			      , data_available, k+1 // +1 so that the value is not zero
			      , queue_id, GASPI_BLOCK
			      )
			    );
	    }
    } else {
  	    wait_or_die ( buf, data_available, iProc+1 );  
    }

    return GASPI_SUCCESS;
}

/** Broadcast collective operation that uses binomial tree.
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
gaspi_return_t
gaspi_bcast_bst (gaspi_segment_id_t const buf,
                 gaspi_number_t const offset,
                 const gaspi_number_t elem_cnt,
                 const gaspi_datatype_t type,
                 const gaspi_number_t root,
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
    gaspi_number_t dsize = type_size * elem_cnt;
    gaspi_number_t doffset = offset * type_size;

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
    for (int i = 0; i < ceil(log2(nProc)); i++) {
        if ((iProc == root) || (i > log2(iProc))) {
            int k = iProc + pow(2,i);
            if ( k < nProc ) {
                bst.children[children_count] = k;
                children_count++;
            }
        }
    }

    for (int i = 0; i < ceil(log2(nProc)); i++) {
        if ((iProc == root) || (i > log2(iProc))) {
            int dst = iProc + pow(2,i);
            if ( dst < nProc ) {
                SUCCESS_OR_DIE
                    ( gaspi_write_notify
                      ( buf, doffset, dst
                      , buf, doffset, dsize
                      , data_available, iProc+1 // +1 so that the value is not zero
                      , 0, GASPI_BLOCK
                      )
                    );
            }
        } 
            
        if (bst.isactive) {
            if ((iProc != root) && (bst.parent <= i)) {
  	            wait_or_die ( buf, data_available, bst.parent+1 );  
                bst.isactive = false;
            }
        }
    }

    return GASPI_SUCCESS;
}

/** Eventually consistent broadcast collective operation that is based on (n-1) straight gaspi_write
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
             gaspi_number_t const offset,
             gaspi_number_t const elem_cnt,
             const gaspi_datatype_t type,
             const gaspi_double threshold,
             const gaspi_number_t root,
             const gaspi_queue_id_t queue_id)
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
 
    if (iProc == root) {	
	    for(uint k = 0; k < nProc; k++) {
	    	if (k == root) 
		    	continue;

	    	SUCCESS_OR_DIE
			    ( gaspi_write_notify
			      ( buf, offset*type_size, k
			      , buf, offset*type_size, ceil(elem_cnt * threshold)*type_size
			      , data_available, root+1 // +1 so that the value is not zero
			      , queue_id, GASPI_BLOCK
			      )
			    );

	    }
    } else {
    	wait_or_die ( buf, data_available, root+1 );  
    }

    return GASPI_SUCCESS;
}

/** Eventually consistent broadcast collective operation that uses binomial tree.
 *
 * @param buf The segment with data for the operation
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
    gaspi_number_t dsize = ceil(elem_cnt * threshold) * type_size;
    gaspi_number_t doffset = offset * type_size;

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
    for (int i = 0; i < ceil(log2(nProc)); i++) {
        if ((iProc == root) || (i > log2(iProc))) {
            int k = iProc + pow(2,i);
            if ( k < nProc ) {
                bst.children[children_count] = k;
                children_count++;
            }
        }
    }

    for (int i = 0; i < ceil(log2(nProc)); i++) {
        if ((iProc == root) || (i > log2(iProc))) {
            int dst = iProc + pow(2,i);
            if ( dst < nProc ) {
                SUCCESS_OR_DIE
                    ( gaspi_write_notify
                      ( buf, doffset, dst
                      , buf, doffset, dsize
                      , data_available, iProc+1 // +1 so that the value is not zero
                      , 0, GASPI_BLOCK
                      )
                    );
            }
        } 
            
        if (bst.isactive) {
            if ((iProc != root) && (bst.parent <= i)) {
  	            wait_or_die ( buf, data_available, bst.parent+1 );  
                bst.isactive = false;
            }
        }
    }

    return GASPI_SUCCESS;
}

/** Reduce collective operation that implements binomial tree
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
// TODO: I assume that we deal with all ranks [0,n-1] however it would be better to have them as in gaspi in rank_grp array
gaspi_return_t 
gaspi_reduce (const gaspi_segment_id_t buffer_send,
	          gaspi_number_t const offset_send,
   	          gaspi_segment_id_t const buffer_receive,
	          gaspi_number_t const offset_recv,
	          const gaspi_number_t elem_cnt,
	          const gaspi_operation_t operation,
	          const gaspi_datatype_t type,
	          const gaspi_number_t root,
	          const gaspi_group_t group,
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
    for (int i = 0; i < ceil(log2(nProc)); i++) {
        if ((iProc == root) || (i > log2(iProc))) {
            int k = iProc + pow(2,i);
            if ( k < nProc ) {
                bst.children[children_count] = k;
                children_count++;
            }
        }
    }
    bst.children_count = children_count;

    // auxiliary pointers
    gaspi_pointer_t src_arr, rcv_arr;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send, &src_arr) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_receive, &rcv_arr) );
    double *src_array = (double *)(src_arr);
    double *rcv_array = (double *)(rcv_arr);

    // actual reduction
    for (int i = ceil(log2(nProc)) - 1; i >= 0; i--) {
        if (bst.isactive) {
            if ((iProc != root) && (children_count == 0) && (log2(iProc) >= i)) {
                gaspi_notification_id_t data_available = iProc;
                SUCCESS_OR_DIE
                    ( gaspi_write_notify
                      ( buffer_send, offset_send * type_size, bst.parent
                      , buffer_receive, offset_recv * type_size, elem_cnt * type_size
                      , data_available, bst.parent+1 // +1 so that the value is not zero
                      , 0, GASPI_BLOCK
                      )
                    );
                bst.isactive = false;
            }
        } 
            
        if (bst.isactive) {
            if (children_count > 0) {
                gaspi_notification_id_t id, range = pow(2, bst.children_count);
                gaspi_notification_t val = iProc+1;

                waitsome_and_reset(buffer_receive
                           , bst.children[0]
                           , range
                           , &id
                           , &val
                           );
                ASSERT(id >= bst.children[0]);
                ASSERT(id <= bst.children[bst.children_count-1]);

                // reduce
                for (j = 0; j < elem_cnt; j++) {
                    src_array[offset_send + j] += rcv_array[offset_recv + j];
                }
                            
                children_count--;
            }
        }
    }

    // copy the results from send to receive buffer
    if (iProc == root) {
        for (j = 0; j < elem_cnt; j++) {
            rcv_array[offset_recv + j] = src_array[offset_send + j];
        }
    }

    return GASPI_SUCCESS;
}

/** Eventually consistent reduce collective operation that implements binomial tree
 *
 * @param buffer_send The buffer with data for the operation.
 * @param buffer_receive The buffer to receive the result of the operation.
 * @param elem_cnt The number of data elements in the buffer (beware of maximum - use gaspi_allreduce_elem_max).
 * @param operation The type of operations (see gaspi_operation_t).
 * @param datatype Type of data (see gaspi_datatype_t).
 * @param threshol The threshol for the amount of data to be reduced. The value is in [0, 1]
 * @param root The process id of the root
 * @param group The group involved in the operation.
 * @param timeout_ms Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST).
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout.
 */
// TODO: I assume that we deal with all ranks [0,n-1] however it would be better to have them as in gaspi in rank_grp array
gaspi_return_t 
gaspi_reduce (const gaspi_segment_id_t buffer_send,
	          gaspi_number_t const offset_send,
   	          gaspi_segment_id_t const buffer_receive,
	          gaspi_number_t const offset_recv,
	          const gaspi_number_t elem_cnt,
	          const gaspi_operation_t operation,
	          const gaspi_datatype_t type,
              const gaspi_double threshold,
	          const gaspi_number_t root,
	          const gaspi_group_t group,
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
    for (int i = 0; i < ceil(log2(nProc)); i++) {
        if ((iProc == root) || (i > log2(iProc))) {
            int k = iProc + pow(2,i);
            if ( k < nProc ) {
                bst.children[children_count] = k;
                children_count++;
            }
        }
    }
    bst.children_count = children_count;

    // auxiliary pointers
    gaspi_pointer_t src_arr, rcv_arr;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send, &src_arr) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_receive, &rcv_arr) );
    double *src_array = (double *)(src_arr);
    double *rcv_array = (double *)(rcv_arr);

    // actual reduction
    for (int i = ceil(log2(nProc)) - 1; i >= 0; i--) {
        if (bst.isactive) {
            if ((iProc != root) && (children_count == 0) && (log2(iProc) >= i)) {
                gaspi_notification_id_t data_available = iProc;
                SUCCESS_OR_DIE
                    ( gaspi_write_notify
                      ( buffer_send, offset_send * type_size, bst.parent
                      , buffer_receive, offset_recv * type_size, ceil(elem_cnt * threshold) * type_size
                      , data_available, bst.parent+1 // +1 so that the value is not zero
                      , 0, GASPI_BLOCK
                      )
                    );
                bst.isactive = false;
            }
        } 
            
        if (bst.isactive) {
            if (children_count > 0) {
                gaspi_notification_id_t id, range = pow(2, bst.children_count);
                gaspi_notification_t val = iProc+1;

                waitsome_and_reset(buffer_receive
                           , bst.children[0]
                           , range
                           , &id
                           , &val
                           );
                ASSERT(id >= bst.children[0]);
                ASSERT(id <= bst.children[bst.children_count-1]);

                // reduce
                for (j = 0; j < ceil(elem_cnt * threshold); j++) {
                    src_array[offset_send + j] += rcv_array[offset_recv + j];
                }
                            
                children_count--;
            }
        }
    }

    // copy the results from send to receive buffer
    if (iProc == root) {
        for (j = 0; j < ceil(elem_cnt * threshold); j++) {
            rcv_array[offset_recv + j] = src_array[offset_send + j];
        }
    }

    return GASPI_SUCCESS;
}

