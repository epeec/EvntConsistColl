
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

template <typename T>
void local_reduce_min(const unsigned int size, T const *input, T *output)
{
    for (unsigned int j = 0; j < size; j++) {
        output[j] = MIN(input[j], output[j]);
    }
}

template <typename T>
void local_reduce_max(const unsigned int size, T const *input, T *output)
{
    for (unsigned int j = 0; j < size; j++) {
        output[j] = MAX(input[j], output[j]);
    }
}

template <typename T>
void local_reduce_sum(const unsigned int size, T const *input, T *output)
{
    for (unsigned int j = 0; j < size; j++) {
        output[j] += input[j];
    }
}

/** 
 * Local reduce
 */
template <typename T>
void local_reduce(const Operation & op, const unsigned int size, T const *input, T *output)
{
    switch (op) {
        case MIN: {
            local_reduce_min<T>(size, input, output);
            break;
        }

        case MAX: {
            local_reduce_max<T>(size, input, output);
            break;
        }

        case SUM: {
            local_reduce_sum<T>(size, input, output);
            break;
        }

        default: {
            throw std::runtime_error ("[Allreduce] Unsupported Operation");
        }
    }
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
 * @param timeout Timeout in milliseconds (or GASPI_BLOCK/GASPI_TEST)
 *
 * @return GASPI_SUCCESS in case of success, GASPI_ERROR in case of
 * error, GASPI_TIMEOUT in case of timeout
 */
template <typename T> gaspi_return_t 
gaspi_ring_allreduce (const segmentBuffer buffer_send,
                      segmentBuffer buffer_receive,
                      segmentBuffer buffer_tmp,
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

    // Allocate a temporary buffer to store incoming data
    gaspi_pointer_t buf_arr;
    SUCCESS_OR_DIE( gaspi_segment_ptr (buffer_tmp.segment, &buf_arr) );
    T *buf_array = (T *)((char*)buf_arr + buffer_tmp.offset);

    // Receive from left neighbor
    const int recv_from = (iProc - 1 + nProc) % nProc;

    // Send to right neigbor
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
        int extra_offset = (i%2) * segment_sizes[send_chunk] * type_size;
        write_notify_and_wait(buffer_receive.segment
                , buffer_receive.offset + segment_start * type_size // offset
                , send_to, buffer_tmp.segment, buffer_tmp.offset + extra_offset // offset
                , segment_sizes[send_chunk] * type_size, data
                , i + iProc + 1 // notification value: +1 to avoid 0. It equals to recvfrom + 1 on receiver side
                , queue_id, GASPI_BLOCK
        );

        // wait for notification that the data has arrived
        gaspi_notification_id_t data_arr = recv_from * nProc + iProc + i;
        wait_or_die( buffer_tmp.segment, data_arr, i + recv_from + 1 );  

        // local reduce
        extra_offset = (i%2) * segment_sizes[recv_chunk] * type_size;
        buf_array = (T *)((char*)buf_arr + buffer_tmp.offset + extra_offset);
        segment_start = segment_ends[recv_chunk] - segment_sizes[recv_chunk];
        local_reduce<T>(op, segment_sizes[recv_chunk], &buf_array[0], &rcv_array[segment_start]);

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
        int recv_chunk = (iProc - i + nProc) % nProc;
        
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
        int extra_offset = (i%2) * segment_sizes[send_chunk] * type_size;
        write_notify_and_wait(buffer_receive.segment
                , buffer_receive.offset + segment_start * type_size // offset
                , send_to, buffer_tmp.segment, buffer_tmp.offset + extra_offset // offset 
                , segment_sizes[send_chunk] * type_size, data
                , i + iProc + 1 // notification value: +1 to avoid 0. It equals to recvfrom + 1 on receiver side
                , queue_id, GASPI_BLOCK
        );

        // wait for notification that the data has arrived
        gaspi_notification_id_t data_arr = recv_from * nProc + iProc + i;
        wait_or_die( buffer_tmp.segment, data_arr, i + recv_from + 1 );  

        // copy
        extra_offset = (i%2) * segment_sizes[recv_chunk] * type_size;
        buf_array = (T *)((char*)buf_arr + buffer_tmp.offset + extra_offset);
        segment_start = segment_ends[recv_chunk] - segment_sizes[recv_chunk];
        for (unsigned int j = 0; j < segment_sizes[recv_chunk]; j++) {
            rcv_array[segment_start + j] = buf_array[j];           
        }         

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
                              segmentBuffer buffer_tmp,
                              const gaspi_number_t elem_cnt,
                              const Operation & op,
                              const gaspi_queue_id_t queue_id,
                              const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_ring_allreduce<float> (const segmentBuffer buffer_send,
                              segmentBuffer buffer_receive,
                              segmentBuffer buffer_tmp,
                              const gaspi_number_t elem_cnt,
                              const Operation & op,
                              const gaspi_queue_id_t queue_id,
                              const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_ring_allreduce<int> (const segmentBuffer buffer_send,
                              segmentBuffer buffer_receive,
                              segmentBuffer buffer_tmp,
                              const gaspi_number_t elem_cnt,
                              const Operation & op,
                              const gaspi_queue_id_t queue_id,
                              const gaspi_timeout_t timeout);

template gaspi_return_t 
gaspi_ring_allreduce<unsigned int> (const segmentBuffer buffer_send,
                              segmentBuffer buffer_receive,
                              segmentBuffer buffer_tmp,
                              const gaspi_number_t elem_cnt,
                              const Operation & op,
                              const gaspi_queue_id_t queue_id,
                              const gaspi_timeout_t timeout);
