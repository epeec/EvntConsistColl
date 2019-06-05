

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
             const gaspi_queue_id_t queue_id,
	     const gaspi_timeout_t timeout_ms)
{
	
    gaspi_notification_id_t data_available = 0;
    gaspi_rank_t iProc, nProc; 
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );
    SUCCESS_OR_DIE( gaspi_proc_num(&nProc) );
    printf("nProc = %d\n", nProc);
  
    if (iProc == root) {	
	    for(int k = 0; k < nProc; k++) {
		if (k == root) 
			continue;

		//SUCCESS_OR_DIE(gaspi_write(buf, offset, k, buf, offset, size, 0, GASPI_BLOCK));
		SUCCESS_OR_DIE
			    ( gaspi_write_notify
			      ( buf, offset, k
			      , buf, offset, elem_cnt
			      , data_available, root+1
			      , queue_id, GASPI_BLOCK
			      )
			    );

	    }
    } else {
  	wait_or_die ( buf, data_available, root+1 );  
    }

    return;
}
