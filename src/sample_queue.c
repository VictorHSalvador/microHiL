/* Implementação do módulo sample queue. */
#include "sample_queue.h"

void sample_queue_init(SampleQueue *queue) {
    atomic_init(&queue->head, 0);
    atomic_init(&queue->tail, 0);
    atomic_init(&queue->dropped, 0);
}

bool sample_queue_push(SampleQueue *queue, const SimulationSample *sample) {
    size_t head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    size_t next = (head + 1U) % SAMPLE_QUEUE_CAPACITY;
    size_t tail = atomic_load_explicit(&queue->tail, memory_order_acquire);

    if (next == tail) {
        atomic_fetch_add_explicit(&queue->dropped, 1, memory_order_relaxed);
        return false;
    }

    queue->slots[head] = *sample;
    atomic_store_explicit(&queue->head, next, memory_order_release);
    return true;
}

bool sample_queue_pop(SampleQueue *queue, SimulationSample *sample) {
    size_t tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&queue->head, memory_order_acquire);

    if (tail == head) return false;

    *sample = queue->slots[tail];
    atomic_store_explicit(&queue->tail, (tail + 1U) % SAMPLE_QUEUE_CAPACITY, memory_order_release);
    return true;
}

uint64_t sample_queue_dropped(const SampleQueue *queue) {
    return atomic_load_explicit(&queue->dropped, memory_order_relaxed);
}
