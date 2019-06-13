#ifndef TESTSOME_H
#define TESTSOME_H

#include <GASPI.h>

void wait_and_reset(gaspi_segment_id_t segment_id
		    , gaspi_notification_t *val
		    , int nid
		    );

void waitsome_and_reset(gaspi_segment_id_t segment_id
			, gaspi_notification_id_t id_start
			, gaspi_number_t id_range
			, gaspi_notification_id_t *id
			, gaspi_notification_t *val
			);

gaspi_return_t testsome_and_reset(gaspi_segment_id_t segment_id
				  , gaspi_notification_id_t id_start
				  , gaspi_number_t id_range
				  , gaspi_notification_id_t *id
				  , gaspi_notification_t *val
				  );


int test_or_die ( gaspi_segment_id_t
                , gaspi_notification_id_t
                , gaspi_notification_t expected
                );

#endif
