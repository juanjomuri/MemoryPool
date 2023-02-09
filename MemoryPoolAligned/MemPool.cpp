#include "MemPool.h"
#include <algorithm>
#include <cassert>
#include <memory>


SubPool::SubPool(uint32_t p_chunk_size)
    : chunk_size(p_chunk_size), chunk_count(0) {}

void* SubPool::Alloc()
{
    mutex.lock();

    // Search for the first free chunk
    for (std::map<void*, bool>::iterator it = chunks.begin(); it != chunks.end(); it++)
    {
        if (it->second)
        {
            // Mark chunk as used
            it->second = false;

            mutex.unlock();

            return it->first;
        }
    }

    mutex.unlock();
    return nullptr;
}

bool SubPool::Free(void* ptr)
{
    bool ret_val = false;

    mutex.lock();

    std::map<void*, bool>::iterator it = chunks.find(ptr);

    if (it != chunks.end())
    {
        if (it->second)
        {
            printf("WARNING: %s -> Chunk %p was already released\n", __func__, ptr);
            ret_val = false;
        }
        else
        {
            // Mark chunk as free
            it->second = true;
            ret_val = true;
        }
    }
    else
    {
        printf("ERROR: %s -> Chunk %p not found in the subpool\n", __func__, ptr);
        ret_val = false;
    }

    mutex.unlock();

    return ret_val;
}

bool SubPool::AddNewChunk(void* ptr)
{
    mutex.lock();
    chunks.insert(std::pair<void*, bool>(ptr, true));
    mutex.unlock();

    chunk_count++;
    return true;
}

uint32_t SubPool::getNumUnfreedChunks()
{
    uint32_t ret_val = 0;

    for (std::map<void*, bool>::iterator it = chunks.begin(); it != chunks.end(); it++)
    {
        ret_val += (it->second ? 0 : 1);
    }

    return ret_val;
}


MemPool::MemPool(uint32_t p_max_memory, std::size_t p_alignment) :
    max_memory(p_max_memory),
    alignment(p_alignment),
    read_buffer(nullptr),
    available_size(p_max_memory)
{
    // Allocate the memory that will be used by the subpools
    buffer = new uint8_t[max_memory];
    read_buffer = buffer;
}

MemPool::~MemPool()
{
    DumpStats();

    // Delete all the SubPools
    std::map<uint32_t, SubPool*>::iterator it = sub_pools.begin();

    for (std::map<uint32_t, SubPool*>::iterator it = sub_pools.begin(); it != sub_pools.end(); ++it)
    {
        delete(it->second);
    }

    // Free the allocated buffer
    delete(buffer);
}

bool MemPool::Initialize(const std::vector<uint32_t>& p_chunk_sizes)
{
    chunk_sizes = p_chunk_sizes;

    // Sort the chunk sizes in ascending order
    std::sort(chunk_sizes.begin(), chunk_sizes.end());

    // Eliminate duplicate elements
    chunk_sizes.erase(std::unique(chunk_sizes.begin(), chunk_sizes.end()), chunk_sizes.end());

    uint32_t sum_chunk_size = 0;

    // Check that all chunk sizes are multiple of the alignment
    for (uint32_t chunk_size : chunk_sizes)
    {
        if (chunk_size % alignment != 0)
        {
            printf("ERROR: %s -> chunk size %d is not multiple of %d bytes alignment\n", __func__, chunk_size, alignment);
            return false;
        }

        sum_chunk_size += chunk_size;
    }

    // Check that the sum of all chunk sizes is smaller than the available memory size
    if (sum_chunk_size > max_memory)
    {
        printf("ERROR: %s -> the sum of all chunk sizes (%d bytes) is bigger than allocator memory size (%d bytes)\n", __func__, sum_chunk_size, max_memory);
        return false;
    }

    // --------------------
    // SubPool construction
    // --------------------
    // Create all the necessary SubPool instances
    for (uint32_t chunk_size : chunk_sizes)
    {
        SubPool* sub_pool = new SubPool(chunk_size);
        sub_pools.insert(std::pair<uint32_t, SubPool*>(chunk_size, sub_pool));
    }

    return true;
}

void MemPool::DumpStats()
{
    std::map<uint32_t, SubPool*>::iterator it = sub_pools.begin();

    for (std::map<uint32_t, SubPool*>::iterator it = sub_pools.begin(); it != sub_pools.end(); ++it)
    {
        printf("STATS: SubPool with size %d -> max used chunks: %d, unfreed chunks %d\n",
                it->first,
                it->second->chunk_count,
                it->second->getNumUnfreedChunks());
    }

}

void* MemPool::Alloc(uint32_t size)
{
    void* chunk_ptr = nullptr;

    // Search for the subpool which chunks fit to the requested size
    for (uint32_t chunk_size : chunk_sizes)
    {
        if (size <= chunk_size)
        {
            mutex.lock();
            chunk_ptr = sub_pools[chunk_size]->Alloc();
            mutex.unlock();

            if (chunk_ptr == nullptr)
            {
                // If there is no chunk available in this subpool, try to allocate memory from the buffer
                bool chunk_assigned = false;

                mutex.lock();

                if (available_size > 0)
                {
                        void* res = std::align(alignment, chunk_size, read_buffer, available_size);

                        if (res != nullptr)
                        {
                            // Add the new chunk to the subpool
                            sub_pools[chunk_size]->AddNewChunk(read_buffer);

                            // Update memory buffer parameters
                            read_buffer = static_cast<std::uint8_t*>(res) + chunk_size;
                            available_size -= chunk_size;

                            // Allocate the new chunk created
                            chunk_ptr = sub_pools[chunk_size]->Alloc();

                            if (chunk_ptr)
                            {
                                chunk_assigned = true;
                                // Save the subpool where this chunk was retrieved from
                                subpool_register[chunk_ptr] = sub_pools[chunk_size];
                            }

                            mutex.unlock();
                            break;
                        }
                        else
                        {
                            mutex.unlock();
                            printf("ERROR: %s -> not possible to find aligned memory in the buffer to allocate %d bytes (available memory: %d bytes)\n", __func__, chunk_size, available_size);
                            return nullptr;
                        }
                }
                else
                {
                    mutex.unlock();
                    printf("ERROR: %s -> not enough memory left in the buffer to allocate %d bytes (available memory: %d)\n", __func__, chunk_size, available_size);
                    return nullptr;
                }

                mutex.unlock();

                if (!chunk_assigned)
                {
                    printf("ERROR: %s -> Not possible to allocate %d bytes\n", __func__, chunk_size);
                    return nullptr;
                }
            }

        }
    }

    return chunk_ptr;
}

bool MemPool::Free(void* ptr)
{
    if (ptr == nullptr)
    {
        return false;
    }

    std::map<void*, SubPool*>::iterator it;

    mutex.lock();
    it = subpool_register.find(ptr);
    mutex.unlock();

    if (it == subpool_register.end())
    {
        printf("ERROR: %s -> no subpool found for %p\n", __func__, ptr);
        return false;
    }

    return it->second->Free(ptr);
}