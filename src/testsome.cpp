#include "testsome.h"

#include "assert.h"
#include "success_or_die.h"

int test_or_die( gaspi_segment_id_t segment_id
		 , gaspi_notification_id_t notification_id
		 , gaspi_notification_t expected
		 )
{
  gaspi_notification_id_t id;
  gaspi_return_t ret;

  if ( ( ret =
         gaspi_notify_waitsome (segment_id, notification_id, 1, &id, GASPI_TEST)
       ) == GASPI_SUCCESS)
    {
      ASSERT (id == notification_id);
      
      gaspi_notification_t value;
      SUCCESS_OR_DIE (gaspi_notify_reset (segment_id, id, &value));
      ASSERT (value == expected);

      return 1;
    }
  else
    {
      ASSERT (ret != GASPI_ERROR);
      
      return 0;
    }
}



void wait_and_reset(gaspi_segment_id_t segment_id
		    , gaspi_notification_t *val
		    , int nid
		    )
{
  gaspi_notification_id_t id;
  SUCCESS_OR_DIE(gaspi_notify_waitsome (segment_id
					, (gaspi_notification_id_t) nid
					, 1
					, &id
					, GASPI_BLOCK
					));
  ASSERT(nid == id);
  SUCCESS_OR_DIE(gaspi_notify_reset (segment_id
				     , id
				     , val
				     ));     
}	  



void waitsome_and_reset(gaspi_segment_id_t segment_id
			, gaspi_notification_id_t id_start
			, gaspi_number_t id_range
			, gaspi_notification_id_t *id
			, gaspi_notification_t *val
			)
{
  SUCCESS_OR_DIE (gaspi_notify_waitsome(segment_id
					, id_start
					, id_range
					, id
					, GASPI_BLOCK
					));
  SUCCESS_OR_DIE (gaspi_notify_reset (segment_id, *id, val));
}



gaspi_return_t testsome_and_reset(gaspi_segment_id_t segment_id
				  , gaspi_notification_id_t id_start
				  , gaspi_number_t id_range
				  , gaspi_notification_id_t *id
				  , gaspi_notification_t *val
				  )
{
  gaspi_return_t ret = GASPI_TIMEOUT;
  gaspi_number_t endA = id_range-id_start;
  gaspi_number_t endB = id_start;

  if ( endA > 0 )
    {
      if ((ret = gaspi_notify_waitsome(segment_id
				       , id_start
				       , endA
				       , id
				       , GASPI_TEST
				       )) == GASPI_SUCCESS)
	{
	  SUCCESS_OR_DIE (gaspi_notify_reset (segment_id, *id, val));
	  if (*val != 0)
	    {
	      return GASPI_SUCCESS;
	    }
	}
    }
  if ( endB > 0 ) 
    {
      if ((ret = gaspi_notify_waitsome(segment_id
				       , 0
				       , endB
				       , id
				       , GASPI_TEST
				       )) == GASPI_SUCCESS)
	{
	  SUCCESS_OR_DIE (gaspi_notify_reset (segment_id, *id, val));
	  if (*val != 0)
	    {
	      return GASPI_SUCCESS;
	    }
	}
    }  
  ASSERT (ret != GASPI_ERROR);
  return GASPI_TIMEOUT;
}




