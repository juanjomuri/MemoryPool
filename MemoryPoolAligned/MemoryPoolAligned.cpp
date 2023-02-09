// MemoryPool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "MemPool.h"

int main()
{
    std::cout << "Hello World!\n";
    std::vector<uint32_t> chunk_sizes;
    chunk_sizes.push_back(64);
    chunk_sizes.push_back(128);
    chunk_sizes.push_back(256);

    MemPool* mem_pool = new MemPool(512, 8);

    if (mem_pool->Initialize(chunk_sizes))
    {
        void* buf1 = mem_pool->Alloc(50);
        void* buf2 = mem_pool->Alloc(250);
        void* buf3 = mem_pool->Alloc(250);
        void* buf4 = mem_pool->Alloc(250);

        mem_pool->Free(buf1);
        mem_pool->Free(buf2);
        mem_pool->Free(buf3);
        mem_pool->Free(buf4);
    }

    delete(mem_pool);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
