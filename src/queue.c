#include "queue.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct queue {
    void* items;
    size_t nmemb;
    size_t capacity;
    size_t size;
};

queue_t* queue_new(size_t nmemb, size_t initial_capacity)
{
    queue_t* queue = (queue_t*)malloc(sizeof(queue_t));
    if (queue) {
        queue->items = NULL;
        queue->size = 0;
        queue->capacity = 0;
        queue->nmemb = nmemb;
        if (!queue_resize(queue, initial_capacity)) {
            free(queue);
            return NULL;
        }
    }
    return queue;
}

void queue_del(queue_t* queue)
{
    if (!queue)
        return;

    if (queue->items) {
        free(queue->items);
    }
    free(queue);
}

bool queue_resize(queue_t* queue, size_t capacity)
{
    if (!queue)
        return false;
    if (capacity == queue->capacity)
        return true;
    if (capacity < queue->size)
        return false;

    if (queue->items == NULL) {
        queue->items = calloc(queue->nmemb, capacity);
        if (queue->items == NULL)
            return false;
    } else {
        void* items = realloc(queue->items, queue->nmemb * capacity);
        if (items == NULL)
            return false;
        queue->items = items;
        if (capacity > queue->capacity)
            memset((char*)queue->items + (queue->nmemb * queue->capacity), 0, queue->nmemb * (capacity - queue->capacity));
    }
    queue->capacity = capacity;
    return true;
}

bool queue_clear(queue_t* queue)
{
    queue->size = 0;
    memset((char*)queue->items, 0, queue->nmemb * queue->capacity);

    return true;
}

size_t queue_get_capacity(queue_t* queue)
{
    if (!queue)
        return 0;
    return queue->capacity;
}

size_t queue_get_size(queue_t* queue)
{
    if (!queue)
        return 0;
    return queue->size;
}

bool queue_is_full(queue_t* queue)
{
    if (!queue)
        return true;
    return queue->size >= queue->capacity;
}

bool queue_is_empty(queue_t* queue)
{
    if (!queue)
        return true;
    return queue->size == 0;
}

bool queue_enqueue(queue_t* queue, void* item)
{
    return queue_enqueue_at_index(queue, item, queue->size);
}

bool queue_enqueue_at_index(queue_t* queue, void* item, size_t index)
{
    if (!queue)
        return false;
    if (queue_get_item(queue, index) == NULL)
        return false;
    memcpy(queue_get_item(queue, index), item, queue->nmemb);

    queue->size = fmaxl(index + 1, queue->size);

    return true;
}

void* queue_dequeue(queue_t* queue)
{
    if (!queue)
        return NULL;
    return queue_dequeue_from_start_to_index(queue, 0);
}

void* queue_dequeue_from_start_to_index(queue_t* queue, size_t index)
{
    if (!queue)
        return NULL;
    if (queue_is_empty(queue))
        return NULL;

    void* item = calloc(queue->nmemb, 1);
    if (item == NULL)
        return false;

    memcpy(item, queue_get_item(queue, index), queue->nmemb);

    for (size_t i = 0; i < queue->size - index - 1; i++) {
        memcpy((char*)queue->items + (i * queue->nmemb),
            (char*)queue->items + ((i + index + 1) * queue->nmemb), queue->nmemb);
    }

    memset((char*)queue->items + ((queue->size - index - 1) * queue->nmemb), 0, queue->nmemb * (index + 1));

    queue->size -= index + 1;
    return item;
}

void* queue_peek(queue_t* queue)
{
    if (!queue)
        return NULL;
    if (queue_is_empty(queue))
        return NULL;
    return queue->items;
}

void* queue_get_item(queue_t* queue, size_t index)
{
    if (index >= queue->capacity)
        return NULL;
    return (char*)queue->items + (index * queue->nmemb);
}