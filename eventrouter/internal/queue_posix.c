#include "queue_.h"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/time.h>

//==============================================================================
// Type Definitions
//==============================================================================

typedef struct
{
    // Manage concurrency.
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
    // Manage contents.
    int m_idx;      //< The next index to read from.
    size_t m_size;  //< The number of elements in the queue.
    // Manage storage.
    size_t m_capacity;    //< The maximum number of elements the queue can hold.
    ErEvent_t* m_data[];  //< The space where the data is held.
} Queue_t;

//==============================================================================
// Local Functions
//==============================================================================

static ErEvent_t* read(Queue_t* a_queue)
{
    ErEvent_t* result = a_queue->m_data[a_queue->m_idx];
    a_queue->m_idx    = (a_queue->m_idx + 1) % a_queue->m_capacity;
    a_queue->m_size -= 1;
    return result;
}

static void write(Queue_t* a_queue, ErEvent_t* a_event)
{
    int write_idx = (a_queue->m_idx + a_queue->m_size) % a_queue->m_capacity;
    a_queue->m_data[write_idx] = a_event;
    a_queue->m_size += 1;
}

/// Returns a `struct timespec` corresponding to the time `a_ms` in the future.
static struct timespec future(int64_t a_ms)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    const int64_t nanos = (tv.tv_sec * 1e9) + (a_ms * 1e6) + (tv.tv_usec * 1e3);

    struct timespec ts;
    ts.tv_sec  = nanos / 1000000000;
    ts.tv_nsec = nanos % 1000000000;
    return ts;
}

//==============================================================================
// Public Functions
//==============================================================================

ErQueue_t ErQueueNew(size_t a_capacity)
{
    Queue_t* const result =
        malloc(sizeof(*result) + (sizeof(result->m_data[0]) * a_capacity));

    pthread_mutex_init(&result->m_mutex, NULL);
    pthread_cond_init(&result->m_cond, NULL);
    result->m_idx      = 0;
    result->m_size     = 0;
    result->m_capacity = a_capacity;

    return result;
}

void ErQueueFree(ErQueue_t a_queue)
{
    assert(a_queue != NULL);
    Queue_t* q = a_queue;
    pthread_cond_destroy(&q->m_cond);
    pthread_mutex_destroy(&q->m_mutex);
    free(q);
}

ErEvent_t* ErQueuePopFront(ErQueue_t a_queue)
{
    assert(a_queue != NULL);

    Queue_t* q        = a_queue;
    ErEvent_t* result = 0;

    // Block until there is data in the queue.
    while (1)
    {
        // Check the condition and while holding the mutex to avoid a race.
        pthread_mutex_lock(&q->m_mutex);
        if (q->m_size == 0)
        {
            pthread_cond_wait(&q->m_cond, &q->m_mutex);
        }
        // At this point one of two things is true:
        //
        //   1. m_size was greater than zero.
        //   2. m_size was initially zero and then the condition was signaled.
        //
        // Since we don't know which happened we must check the condition again;
        // `pthread_cond_broadcast()` wakes more than one thread and another
        // thread could have read the data first.
        //
        // If there is data in the queue consume it, release the lock and break
        // out of the loop. If there isn't any data (because another thread read
        // it) then go back to waiting on the condition variable.
        if (q->m_size > 0)
        {
            result = read(q);
            pthread_cond_broadcast(&q->m_cond);  // Notify blocked writers.
            pthread_mutex_unlock(&q->m_mutex);
            break;
        }
        else
        {
            pthread_mutex_unlock(&q->m_mutex);
        }
    }

    return result;
}

// NOTE: The structure and motivations of this function are similar to that
// of `ErQueuePopFront()`; please read those comments to understand this.
void ErQueuePushBack(ErQueue_t a_queue, ErEvent_t* a_event)
{
    assert(a_queue != NULL);

    Queue_t* q = a_queue;

    while (1)
    {
        pthread_mutex_lock(&q->m_mutex);
        if (q->m_size == q->m_capacity)
        {
            pthread_cond_wait(&q->m_cond, &q->m_mutex);
        }
        if (q->m_size < q->m_capacity)
        {
            write(q, a_event);
            pthread_cond_broadcast(&q->m_cond);  // Notify blocked readers.
            pthread_mutex_unlock(&q->m_mutex);
            break;
        }
        else
        {
            pthread_mutex_unlock(&q->m_mutex);
        }
    }
}

// NOTE: The structure and motivations of this function are similar to that of
// `ErQueuePopFront()`; please read those comments to understand this.
bool ErQueueTimedPopFront(ErQueue_t a_queue, ErEvent_t** a_event, int64_t a_ms)
{
    assert(a_queue != NULL);

    struct timespec ts = future(a_ms);
    bool result        = false;
    Queue_t* q         = a_queue;

    while (1)
    {
        pthread_mutex_lock(&q->m_mutex);
        if (q->m_size == 0)
        {
            if (pthread_cond_timedwait(&q->m_cond, &q->m_mutex, &ts) ==
                ETIMEDOUT)
            {
                result = false;
                pthread_mutex_unlock(&q->m_mutex);
                break;
            }
        }
        if (q->m_size > 0)
        {
            *a_event = read(q);
            pthread_cond_broadcast(&q->m_cond);  // Notify blocked writers.
            pthread_mutex_unlock(&q->m_mutex);
            result = true;
            break;
        }
        else
        {
            pthread_mutex_unlock(&q->m_mutex);
        }
    }

    return result;
}

// NOTE: The structure and motivations of this function are similar to that of
// `ErQueuePopFront()`; please read those comments to understand this.
bool ErQueueTimedPushBack(ErQueue_t a_queue, ErEvent_t* a_event, int64_t a_ms)
{
    assert(a_queue != NULL);

    struct timespec ts = future(a_ms);
    bool result        = false;
    Queue_t* q         = a_queue;

    while (1)
    {
        pthread_mutex_lock(&q->m_mutex);
        if (q->m_size == q->m_capacity)
        {
            if (pthread_cond_timedwait(&q->m_cond, &q->m_mutex, &ts) ==
                ETIMEDOUT)
            {
                result = false;
                pthread_mutex_unlock(&q->m_mutex);
                break;
            }
        }
        if (q->m_size < q->m_capacity)
        {
            write(q, a_event);
            pthread_cond_broadcast(&q->m_cond);  // Notify blocked readers.
            pthread_mutex_unlock(&q->m_mutex);
            result = true;
            break;
        }
        else
        {
            pthread_mutex_unlock(&q->m_mutex);
        }
    }

    return result;
}
