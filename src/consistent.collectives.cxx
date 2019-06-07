
#include <cmath>

#include <consistent.collectives.hxx>

#include "success_or_die.h"
#include "waitsome.h"

//gaspi_return_t
void
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
			      , data_available, root+1 // +1 so that the value is not zero
			      , queue_id, GASPI_BLOCK
			      )
			    );

	    }
    } else {
  	wait_or_die ( buf, data_available, root+1 );  
    }

    return;
}

// eventually consistent broadcast
//   initial version
void
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

    return;
}
