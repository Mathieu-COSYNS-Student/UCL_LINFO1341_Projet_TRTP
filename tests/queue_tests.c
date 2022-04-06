#include "queue_tests.h"

#include <stdbool.h>
#include <stdlib.h>

#include "../src/log.h"
#include "../src/queue.h"

bool test_queue_1()
{
    queue_t* queue = queue_new(1, 5);
    ASSERT(queue);
    bool is_empty = queue_is_empty(queue);
    bool is_full = queue_is_full(queue);
    int size = queue_get_size(queue);
    int capacity = queue_get_capacity(queue);
    void* peek = queue_peek(queue);

    ASSERT(is_empty);
    ASSERT(!is_full);
    ASSERT(size == 0);
    ASSERT(capacity == 5);
    ASSERT(!peek);

    queue_del(queue);

    return is_empty && !is_full && size == 0 && capacity == 5 && !peek;
}

bool test_queue_2()
{
    size_t item_count = 0;
    queue_t* queue = queue_new(sizeof(item_count), 5);
    ASSERT(queue);

    bool is_empty;
    bool is_full;
    size_t size;
    size_t capacity;
    size_t* peek;
    size_t* element;

    do {
        item_count++;
        ASSERT(queue_enqueue(queue, &item_count));

        is_empty = queue_is_empty(queue);
        is_full = queue_is_full(queue);
        size = queue_get_size(queue);
        capacity = queue_get_capacity(queue);
        peek = queue_peek(queue);
        element = queue_get_item(queue, item_count - 1);

        ASSERT(!is_empty);
        if (item_count != 5) {
            ASSERT(!is_full);
        } else {
            ASSERT(is_full);
        }
        ASSERT(size == item_count);
        ASSERT(capacity == 5);
        ASSERT(peek);
        ASSERT(*peek == 1);
        ASSERT(*element == item_count);

    } while (!is_empty && !is_full && size == item_count && capacity == 5 && peek);

    ASSERT(!queue_enqueue(queue, &item_count));
    ASSERT(!queue_resize(queue, 4));

    ASSERT(queue_resize(queue, 5));
    ASSERT(queue_get_capacity(queue) == 5);
    ASSERT(queue_is_full(queue));

    ASSERT(queue_resize(queue, 8));
    ASSERT(!queue_is_full(queue));
    ASSERT(queue_get_capacity(queue) == 8);
    ASSERT(*((size_t*)queue_get_item(queue, 5)) == 0);
    ASSERT(*((size_t*)queue_get_item(queue, 6)) == 0);
    ASSERT(*((size_t*)queue_get_item(queue, 7)) == 0);

    size_t item_count_full = item_count;
    element = NULL;

    do {
        item_count--;
        element = queue_dequeue(queue);

        is_empty = queue_is_empty(queue);
        is_full = queue_is_full(queue);
        size = queue_get_size(queue);
        capacity = queue_get_capacity(queue);
        peek = queue_peek(queue);

        if (item_count != 0) {
            ASSERT(!is_empty);
            ASSERT(peek);
            ASSERT(*peek == 6 - item_count);
        } else {
            ASSERT(is_empty);
            ASSERT(!peek);
        }
        ASSERT(!is_full);
        ASSERT(size == item_count);
        ASSERT(capacity == 8);
        ASSERT(*element == 5 - item_count);
    } while (!is_empty && !is_full && size == item_count && capacity == 8 && peek);

    queue_del(queue);

    return item_count_full == 5 && item_count == 0;
}

bool test_queue_3()
{
    size_t item_count = 0;
    queue_t* queue = queue_new(sizeof(item_count), 5);
    ASSERT(queue);

    bool is_empty;
    bool is_full;
    size_t size;
    size_t capacity;
    size_t* peek;
    size_t* element;

    do {
        item_count++;
        ASSERT(queue_enqueue(queue, &item_count));

        is_empty = queue_is_empty(queue);
        is_full = queue_is_full(queue);
        size = queue_get_size(queue);
        capacity = queue_get_capacity(queue);
        peek = queue_peek(queue);
        element = queue_get_item(queue, item_count - 1);

        ASSERT(!is_empty);
        if (item_count != 5) {
            ASSERT(!is_full);
        } else {
            ASSERT(is_full);
        }
        ASSERT(size == item_count);
        ASSERT(capacity == 5);
        ASSERT(peek);
        ASSERT(*peek == 1);
        ASSERT(*element == item_count);

    } while (!is_empty && !is_full && size == item_count && capacity == 5 && peek);

    ASSERT(!queue_enqueue(queue, &item_count));
    ASSERT(!queue_resize(queue, 4));

    ASSERT(queue_resize(queue, 5));
    ASSERT(queue_get_capacity(queue) == 5);
    ASSERT(queue_is_full(queue));

    ASSERT(queue_resize(queue, 8));
    ASSERT(!queue_is_full(queue));
    ASSERT(queue_get_capacity(queue) == 8);
    ASSERT(*((size_t*)queue_get_item(queue, 5)) == 0);
    ASSERT(*((size_t*)queue_get_item(queue, 6)) == 0);
    ASSERT(*((size_t*)queue_get_item(queue, 7)) == 0);

    size_t item_count_full = item_count;
    element = NULL;

    queue_dequeue_from_start_to_index(queue, 2);

    ASSERT(queue_get_size(queue) == 2);

    for (size_t i = 0; i < 2; i++) {
        ASSERT(*((size_t*)queue_get_item(queue, i)) == i + 4);
        if (*((size_t*)queue_get_item(queue, i)) != i + 4) {
            item_count = -1;
        }
    }

    for (size_t i = 2; i < 8; i++) {
        ASSERT(*((size_t*)queue_get_item(queue, i)) == 0);
        if (*((size_t*)queue_get_item(queue, i)) != 0) {
            item_count = -1;
        }
    }

    queue_del(queue);

    return item_count_full == 5 && item_count == 5;
}

typedef bool (*TestCallback)();
TestCallback queue_tests[] = {
    &test_queue_1,
    &test_queue_2,
    &test_queue_3,
    NULL
};

int run_queue_tests()
{
    size_t number_of_tests_failed = 0;
    size_t i = 0;

    while (queue_tests[i]) {
        bool test_result = queue_tests[i]();
        if (!test_result)
            number_of_tests_failed++;
        i++;
    }

    char* test_name = "Queue tests";

    if (number_of_tests_failed) {
        TEST_FAILED(test_name, i, number_of_tests_failed);
        return EXIT_FAILURE;
    }

    TEST_SUCCESS(test_name, i);

    return EXIT_SUCCESS;
}