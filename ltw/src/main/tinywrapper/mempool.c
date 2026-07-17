#include "mempool.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef __ANDROID__
#include <pthread.h>
#endif

static inline size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

#ifdef __ANDROID__
#define LOCK(pool) do { if((pool)->thread_safe) pthread_mutex_lock(&(pool)->mutex); } while(0)
#define UNLOCK(pool) do { if((pool)->thread_safe) pthread_mutex_unlock(&(pool)->mutex); } while(0)
#else
#define LOCK(pool)
#define UNLOCK(pool)
#endif

static mempool_t* mempool_create_internal(size_t object_size, size_t chunk_size, size_t max_memory, bool thread_safe) {
    mempool_t* pool = calloc(1, sizeof(mempool_t));
    if (!pool) {
        LTW_ERROR_PRINTF("mempool: failed to allocate pool structure");
        return NULL;
    }

    pool->object_size = align_size(object_size, MEMPOOL_ALIGNMENT);
    if (pool->object_size < sizeof(void*)) {
        pool->object_size = align_size(sizeof(void*), MEMPOOL_ALIGNMENT);
    }

    pool->chunk_size = chunk_size > 0 ? chunk_size : MEMPOOL_CHUNK_SIZE;
    pool->free_list = NULL;
    pool->chunks = NULL;
    pool->total_chunks = 0;
    pool->used_count = 0;
    pool->free_count = 0;
    pool->max_memory = max_memory > 0 ? max_memory : MEMPOOL_DEFAULT_MAX_MEMORY;
    pool->current_memory = 0;
    pool->thread_safe = thread_safe;

#ifdef __ANDROID__
    if (thread_safe) {
        pthread_mutex_init(&pool->mutex, NULL);
    }
#endif

    return pool;
}

mempool_t* mempool_create(size_t object_size, size_t chunk_size) {
    return mempool_create_internal(object_size, chunk_size, MEMPOOL_DEFAULT_MAX_MEMORY, false);
}

mempool_t* mempool_create_ex(size_t object_size, size_t chunk_size, size_t max_memory, bool thread_safe) {
    return mempool_create_internal(object_size, chunk_size, max_memory, thread_safe);
}

void mempool_destroy(mempool_t* pool) {
    if (!pool) return;

    LOCK(pool);

    mempool_chunk_t* chunk = pool->chunks;
    while (chunk) {
        mempool_chunk_t* next = chunk->next;
        free(chunk);
        chunk = next;
    }

    UNLOCK(pool);

#ifdef __ANDROID__
    if (pool->thread_safe) {
        pthread_mutex_destroy(&pool->mutex);
    }
#endif

    free(pool);
}

static bool mempool_expand(mempool_t* pool) {
    if(pool->chunk_size > SIZE_MAX / pool->object_size) {
        LTW_ERROR_PRINTF("mempool: integer overflow in chunk size calculation (multiplication)");
        return false;
    }
    size_t objects_size = pool->object_size * pool->chunk_size;
    if(objects_size > SIZE_MAX - sizeof(mempool_chunk_t)) {
        LTW_ERROR_PRINTF("mempool: integer overflow in chunk size calculation (addition)");
        return false;
    }
    size_t chunk_size = objects_size + sizeof(mempool_chunk_t);

    if (pool->current_memory + chunk_size > pool->max_memory) {
        LTW_ERROR_PRINTF("mempool: memory limit exceeded (current=%zu, max=%zu)", 
                         pool->current_memory, pool->max_memory);
        return false;
    }

    mempool_chunk_t* chunk = malloc(chunk_size);
    if (!chunk) {
        LTW_ERROR_PRINTF("mempool: failed to expand pool");
        return false;
    }

    chunk->next = pool->chunks;
    chunk->used = 0;
    pool->chunks = chunk;

    char* data = (char*)chunk + sizeof(mempool_chunk_t);
    for (size_t i = 0; i < pool->chunk_size; i++) {
        mempool_block_t* block = (mempool_block_t*)(data + i * pool->object_size);
        block->next = pool->free_list;
        pool->free_list = block;
    }

    pool->total_chunks++;
    pool->free_count += pool->chunk_size;
    pool->current_memory += chunk_size;

    return true;
}

void* mempool_alloc(mempool_t* pool) {
    if (!pool) return NULL;

    LOCK(pool);

    if (!pool->free_list) {
        if (!mempool_expand(pool)) {
            UNLOCK(pool);
            return NULL;
        }
    }

    mempool_block_t* block = pool->free_list;
    pool->free_list = block->next;
    pool->used_count++;
    pool->free_count--;

    UNLOCK(pool);

    return block;
}

void mempool_free(mempool_t* pool, void* ptr) {
    if (!pool || !ptr) return;

    LOCK(pool);

    mempool_block_t* block = (mempool_block_t*)ptr;
    block->next = pool->free_list;
    pool->free_list = block;
    pool->used_count--;
    pool->free_count++;

    UNLOCK(pool);
}

void mempool_get_stats(mempool_t* pool, size_t* total, size_t* used, size_t* free, size_t* memory_used) {
    if (!pool) return;

    LOCK(pool);

    if (total) *total = pool->total_chunks * pool->chunk_size;
    if (used) *used = pool->used_count;
    if (free) *free = pool->free_count;
    if (memory_used) *memory_used = pool->current_memory;

    UNLOCK(pool);
}

void mempool_reset(mempool_t* pool) {
    if (!pool) return;

    LOCK(pool);

    pool->free_list = NULL;
    pool->used_count = 0;
    pool->free_count = 0;

    mempool_chunk_t* chunk = pool->chunks;
    while (chunk) {
        char* data = (char*)chunk + sizeof(mempool_chunk_t);
        for (size_t i = 0; i < pool->chunk_size; i++) {
            mempool_block_t* block = (mempool_block_t*)(data + i * pool->object_size);
            block->next = pool->free_list;
            pool->free_list = block;
        }
        chunk = chunk->next;
    }

    pool->free_count = pool->total_chunks * pool->chunk_size;

    UNLOCK(pool);
}

void mempool_set_max_memory(mempool_t* pool, size_t max_memory) {
    if (!pool) return;
    LOCK(pool);
    pool->max_memory = max_memory;
    UNLOCK(pool);
}

size_t mempool_get_total_allocated(mempool_t* pool) {
    if (!pool) return 0;
    LOCK(pool);
    size_t result = pool->current_memory;
    UNLOCK(pool);
    return result;
}
