
#include <cmath>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <cstring>

#include <Allreduce.hxx>

#include "success_or_die.h"
#include "testsome.h"
#include "queue.h"
#include "waitsome.h"

/** Segmented pipeline ring implementation
 *
 * @param buffer_send Segment with offset of the original data
 * @param buffer_receive Segment with offset of the reduced data
 * @param elem_cnt Number of data elements in the buffer
 * @param operation The type of operations (MIN, MAX, SUM).
 * @param datatype Type of data (see gaspi_datatype_t)
 * @param queue_id Queue id
 * @param timeout Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST)
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout
 */
template <typename T> gaspi_return_t 
gaspi_ring_allreduce (const segmentBuffer buffer_send,
                      segmentBuffer buffer_receive,
                      const gaspi_number_t elem_cnt,
                      const Operation & op,
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

    // auxiliary pointers
    gaspi_pointer_t src_arr, rcv_arr;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_send.segment, &src_arr) );
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_receive.segment, &rcv_arr) );
    T *src_array = (T *)((char*)src_arr + buffer_send.offset);
    T *rcv_array = (T *)((char*)rcv_arr + buffer_receive.offset);

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

    // Receive from left neighbor
    const int recv_from = (iProc - 1 + nProc) % nProc;

    // Send to right neighbor
    const int send_to = (iProc + 1) % nProc;

    // scatter-reduce phase
    // At every step, for every rank, we iterate through
    // segments with wraparound and send and recv from our neighbors and reduce
    // locally. At the i'th iteration, iProc sends segment (rank - i) and receives
    // segment (rank - i - 1)
    for (int i = 0; i < nProc - 1; i++) {

        int recv_chunk = (iProc - i - 1 + nProc) % nProc;
        int send_chunk = (iProc - i + nProc) % nProc;
        
        // waive that it is ready to receive
        gaspi_notification_id_t ready = iProc + i;
        notify_and_wait(buffer_send.segment
                , recv_from, ready, iProc + 1
                , queue_id, timeout
        );

        // wait for notification that the data can be sent
        gaspi_notification_id_t ready_arr = send_to + i;
        wait_or_die( buffer_send.segment, ready_arr, send_to + 1 );  

        // write data
        int segment_start = segment_ends[send_chunk] - segment_sizes[send_chunk];
        gaspi_notification_id_t data = iProc * nProc + send_to + i; 
        write_notify_and_wait(buffer_receive.segment
                , buffer_receive.offset + segment_start * type_size // offset
                , send_to, buffer_receive.segment, buffer_receive.offset + segment_start * type_size // offset
                , segment_sizes[send_chunk] * type_size, data
                , i + iProc + 1 // notification value: +1 to avoid 0. It equals to recvfrom + 1 on receiver side
                , queue_id, GASPI_BLOCK
        );

        // wait for notification that the data has arrived
        gaspi_notification_id_t data_arr = recv_from * nProc + iProc + i;
        wait_or_die( buffer_receive.segment, data_arr, i + recv_from + 1 );  

        // local reduce
        segment_start = segment_ends[recv_chunk] - segment_sizes[recv_chunk];
        local_reduce<T>(op, segment_sizes[recv_chunk], &src_array[segment_start], &rcv_array[segment_start]);

        // ackowledge that the data has arrived
        gaspi_notification_id_t ack = i + recv_from + 1;
        notify_and_wait(buffer_receive.segment
                , recv_from, ack, iProc + 1
                , queue_id, timeout
        );
            
        // wait for acknowledgement notification
        gaspi_notification_id_t ack_arr = i + iProc + 1;
        wait_or_die( buffer_receive.segment, ack_arr, send_to + 1 );  
    }

    // pipelined ring allgather
    // At every step, for every rank, we iterate through
    // segments with wraparound and send and recv from our neighbors.
    // At the i'th iteration, iProc sends segment (rank + 1 - i) and receives
    // segment (rank - i)
    for (int i = 0; i < nProc-1; i++) {
        
        int send_chunk = (iProc - i + 1 + nProc) % nProc;
        
        // ready to receive
        gaspi_notification_id_t ready = iProc + i;
        notify_and_wait(buffer_send.segment
                , recv_from, ready, iProc + 1
                , queue_id, timeout
        );

        // wait for notification that the data can be sent
        gaspi_notification_id_t ready_arr = send_to + i;
        wait_or_die( buffer_send.segment, ready_arr, send_to + 1 );  
   
        // write data 
        int segment_start = segment_ends[send_chunk] - segment_sizes[send_chunk];
        gaspi_notification_id_t data = iProc * nProc + send_to + i; 
        write_notify_and_wait(buffer_receive.segment
                , buffer_receive.offset + segment_start * type_size // offset
                , send_to, buffer_receive.segment, buffer_receive.offset + segment_start * type_size // offset 
                , segment_sizes[send_chunk] * type_size, data
                , i + iProc + 1 // notification value: +1 to avoid 0. It equals to recvfrom + 1 on receiver side
                , queue_id, GASPI_BLOCK
        );

        // wait for notification that the data has arrived
        gaspi_notification_id_t data_arr = recv_from * nProc + iProc + i;
        wait_or_die( buffer_receive.segment, data_arr, i + recv_from + 1 );  

        // ackowledge that the data has arrived
        gaspi_notification_id_t ack = i + recv_from + 1;
        notify_and_wait(buffer_receive.segment
                , recv_from, ack, iProc + 1
                , queue_id, timeout
        );
            
        // wait for acknowledgement notification
        gaspi_notification_id_t ack_arr = i + iProc + 1;
        wait_or_die( buffer_receive.segment, ack_arr, send_to + 1 );  
    }

    return GASPI_SUCCESS;
}

// explicit template instantiation
template gaspi_return_t 
gaspi_ring_allreduce<double> (const segmentBuffer buffer_send,
                              segmentBuffer buffer_receive,
                              const gaspi_number_t elem_cnt,
                              const Operation & op,
                              const gaspi_queue_id_t queue_id,
                              const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_ring_allreduce<float> (const segmentBuffer buffer_send,
                              segmentBuffer buffer_receive,
                              const gaspi_number_t elem_cnt,
                              const Operation & op,
                              const gaspi_queue_id_t queue_id,
                              const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_ring_allreduce<int> (const segmentBuffer buffer_send,
                              segmentBuffer buffer_receive,
                              const gaspi_number_t elem_cnt,
                              const Operation & op,
                              const gaspi_queue_id_t queue_id,
                              const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_ring_allreduce<unsigned int> (const segmentBuffer buffer_send,
                              segmentBuffer buffer_receive,
                              const gaspi_number_t elem_cnt,
                              const Operation & op,
                              const gaspi_queue_id_t queue_id,
                              const gaspi_timeout_t timeout);
