
#include <cmath>

#include <consistent.collectives.hxx>

#include "success_or_die.h"
#include "queue.h"
#include "waitsome.h"

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
// TODO: I assume that we deal with all ranks [0,n-1] however it would be better to have them as in gaspi in rank_grp array
gaspi_return_t 
gaspi_reduce (const gaspi_pointer_t buffer_send,
   	          gaspi_pointer_t const buffer_receive,
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

	int tmprank, jmp, mask, lastmask, tmpdst, dst, rest;
	lastmask = 0x1;  
	mask = lastmask & 0x7fffffff;
	jmp = lastmask >> 31;
	tmprank = iProc;
	rest = 0;

    // get size of type, see GASPI.h for details
    gaspi_number_t type_size = 0;
    if (type >= 3) 
    	type_size = 8;
    else
	    type_size = 4;

    gaspi_number_t dsize = type_size * elem_cnt;

	while ( mask < nProc ) {
		tmpdst = tmprank ^ mask;
		dst = (tmpdst < rest) ? tmpdst * 2 + 1 : tmpdst + rest;

		if ( jmp ) {
			jmp = 0;
//			goto J2;
		}

        // TODO:  _gaspi_allreduce_write_and_sync(gctx, g, send_ptr, dsize, dst, bid, timeout_ms) != GASPI_SUCCESS 
//        SUCCESS_OR_DIE
//            ( gaspi_write_notify
//              ( buf, 0, k
//              , buf, , ceil(elem_cnt * threshold)*type_size
//              , data_available, root+1 // +1 so that the value is not zero
//              , queue_id, GASPI_BLOCK
//              )
//            );
//        gaspi_notification_id_t notification = iProc;
//        write_notify_and_wait(buffer_send, 
//                              0,
//                              dst,
//                              buffer_receive,
//                              0,
//                              dsize,
//                              notification,
//                              dst,
//                              0);
//
//        J2:
//        gaspi_notification_id_t = id;
//        gaspi_notification_t val = 1; 
//        waitsome_and_reset(buffer_receive, 0, nProc, &id, &val);
//        ASSERT(id >= 0);
//        ASSERT(id <= nProc);
//
//        if (1) {
//            lastmask = (mask | 0x80000000);
//            return GASPI_TIMEOUT;
//        }
//        // _gaspi_sync_wait
//        
        mask <= 1;
	}

	return GASPI_SUCCESS;
}
