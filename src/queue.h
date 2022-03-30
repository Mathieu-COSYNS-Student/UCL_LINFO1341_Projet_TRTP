#ifndef __QUEUE_H_
#define __QUEUE_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct queue queue_t; // Opaque type

queue_t* queue_new(size_t nmemb, size_t initial_capacity);

void queue_del(queue_t* queue);

bool queue_resize(queue_t* queue, size_t capacity);

size_t queue_get_capacity(queue_t* queue);

size_t queue_get_size(queue_t* queue);

bool queue_is_full(queue_t* queue);

bool queue_is_empty(queue_t* queue);

bool queue_enqueue(queue_t* queue, void* item);

bool queue_enqueue_at_index(queue_t* queue, void* item, size_t index);

void* queue_dequeue(queue_t* queue);

void* queue_dequeue_from_start_to_index(queue_t* queue, size_t index);

void* queue_peek(queue_t* queue);

void* queue_get_item(queue_t* queue, size_t index);

#endif /* __QUEUE_H_ */
