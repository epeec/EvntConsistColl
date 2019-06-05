

#ifndef CONSISTENT_COLLECTIVES_H
#define CONSISTENT_COLLECTIVES_H

#include <GASPI.h>

//gaspi_return_t
void
gaspi_bcast (gaspi_segment_id_t const buf,
	     const gaspi_number_t offset,
	     const gaspi_number_t elem_cnt,
	     const gaspi_datatype_t type,
	     const gaspi_number_t root,
             const gaspi_queue_id_t queue_id,
	     const gaspi_timeout_t timeout_ms);
	     //const gaspi_group_t g,

#endif //#define CONSISTENT_COLLECTIVES_H
