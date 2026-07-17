#ifndef POJAVLAUNCHER_MEMPOOL_H
#define POJAVLAUNCHER_MEMPOOL_H

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

#define MEMPOOL_MAX_POOLS 8
#define MEMPOOL_CHUNK_SIZE 64
#define MEMPOOL_ALIGNMENT 16
#define MEMPOOL_DEFAULT_MAX_MEMORY (2048 * 1024 * 1024)

typedef struct mempool_block {
    struct mempool_block* next;
} mempool_block_t;

typedef struct mempool_chunk {
    struct mempool_chunk* next;
    size_t used;
} mempool_chunk_t;

typedef struct mempool {
    size_t object_size;
    size_t chunk_size;
    mempool_block_t* free_list;
    mempool_chunk_t* chunks;
    size_t total_chunks;
    size_t used_count;
    size_t free_count;
    size_t max_memory;
    size_t current_memory;
    bool thread_safe;
#ifdef __ANDROID__
    pthread_mutex_t mutex;
#endif
} mempool_t;

mempool_t* mempool_create(size_t object_size, size_t chunk_size);
mempool_t* mempool_create_ex(size_t object_size, size_t chunk_size, size_t max_memory, bool thread_safe);
void mempool_destroy(mempool_t* pool);
void* mempool_alloc(mempool_t* pool);
void mempool_free(mempool_t* pool, void* ptr);
void mempool_get_stats(mempool_t* pool, size_t* total, size_t* used, size_t* free, size_t* memory_used);
void mempool_reset(mempool_t* pool);
void mempool_set_max_memory(mempool_t* pool, size_t max_memory);
size_t mempool_get_total_allocated(mempool_t* pool);

#endif //POJAVLAUNCHER_MEMPOOL_H