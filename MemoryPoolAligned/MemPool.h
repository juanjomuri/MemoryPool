#pragma once

#include <vector>
#include <mutex>
#include <map>

/**
 * This class implements a subpool of chunks. With the following characteristics:
 *  - All chunks are of the same size: chunk_size.
 *  - There is a finite number of chunks: chunk_count.
 *  - The pool memory is not allocated in this class, so there is no need to free the memory in the destructor.
 *  - The access to the chunks is thread safe. Use of mutex.
 *  - The subpool is not intented to reallocate memory if there are no chunks available. This will be treated as an error.
 */
class SubPool {

    friend class MemPool;

private:
    // Size of the chunks of the pool
    uint32_t chunk_size;

    // Total number of chunks in the pool
    uint32_t chunk_count;

    // Mapping between the chunk pointer and its state. True if its free: available to allocate.
    std::map<void*, bool> chunks;

    // Mutex to avoid race conditions
    std::mutex mutex;

public:

    /**
     * Class constructor
     * @param p_chunk_size. Size of the chunks contained in the pool.
     */
    SubPool(uint32_t p_chunk_size);

    /**
     * Allocate a chunk from the pool.
     * @return the pointer to the beginning of the chunk allocated. Returns 'nullptr' if there are no chunks available.
     */
    void* Alloc();

    /**
     * Free a chunk of the pool.
     * @param ptr. Pointer to the chunk to be freed.
     * @return true if the operation is successful.
     */
    bool Free(void* ptr);

    /**
     * Add a new chunk to the pool.
     * @param ptr. Pointer to the chunk to be added.
     * @return true if the operation is successful.
     */
    bool AddNewChunk(void* ptr);

    /**
     * @return total number of chunks of the pool.
     */
    inline uint32_t getNumChunks() { return chunk_count; }

    /**
     * @return number of chunks that have not been freed.
     */
    uint32_t getNumUnfreedChunks();
};

/**
 * This class implements a memory pool composed by subpools of fixed size chunks. With the following characteristics:
 *  - The maximum memory allocated is an input parameter.
 *  - The chunk sizes of the subpools are input parameter.
 *  - The chunk sizes of the subpool must be aligned to the alignment specified in the constructor.
 *  - All chunks allocated must be tracked to return the chunk to the correct pool.
 */
class MemPool {

private:
    // Maximum memory allowed to be allocated
    uint32_t max_memory;

    // Memory alignment
    uint32_t alignment;

    // Vector containing all the chunk sizes available in the allocator
    std::vector<uint32_t> chunk_sizes;

    // Mapping between pool size and its SubPool instance
    std::map<uint32_t, SubPool*> sub_pools;

    // Pointer to the beginning of the memory
    uint8_t* buffer;

    // Pointer to the beginning of the memory pending to be allocated
    void* read_buffer;

    uint32_t available_size;

    // Mapping between an allocated chunk and its corresponding SubPool
    std::map<void*, SubPool*> subpool_register;

    // Mutex to avoid race conditions
    std::mutex mutex;

public:

    /**
     * Class constructor
     * @param p_max_memory. Maximum memory available to allocate (in bytes).
     * @param p_alignment. Alignment of the memory (in bytes).
     */
    MemPool(uint32_t p_max_memory, std::size_t p_alignment);

    /**
     * Class destructor. 
     * Important note: All memory allocated in the constructor must be released.
     */
    ~MemPool();

    /**
     * Initializes the pool and all the subpools managed.
     * @param p_chunk_sizes. Vector containing the chunk sizes to be managed by the subpools.
     * @return true if the operation is successful.
     */
    bool Initialize(const std::vector<uint32_t>& p_chunk_sizes);

    /**
     * Allocate a chunk from the corresponding subpool.
     * Important note: subpools don't have a limited number of chunks available. The chunk creation 
     * is on demand of the external client. The number of chunks limit is based on the remaining
     * buffer memory available.
     * @return the pointer to the beginning of the chunk allocated. Returns 'nullptr' if it's not possible
     * to allocate the requested size.
     */
    void* Alloc(uint32_t size);

    /**
     * Free a chunk from the corresponding subpool.
     * @param ptr. Pointer to the chunk to be freed.
     * @return true if the operation is successful.
     */
    bool Free(void* ptr);

    /**
     * Dump the following statistics of each subpool:
     *  - Maximum number of chunks allocated.
     *  - Number of unfreed chunks.
     */
    void DumpStats();
};
