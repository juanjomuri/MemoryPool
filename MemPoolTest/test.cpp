#include "pch.h"
#include <vector>
#include <iostream>
#include "..\MemoryPoolAligned\MemPool.h"

namespace mytest {

    class MemPoolInitialize : public ::testing::Test {
    protected:
        void SetUp() override {
            mem_pool = new MemPool(1024, 8);
        }

        void TearDown() override {
            delete(mem_pool);
        }

        std::vector<uint32_t> chunk_sizes;
        MemPool* mem_pool;
    };

    TEST_F(MemPoolInitialize, ChunkSizesAreMultiple) {
        chunk_sizes.push_back(64);
        chunk_sizes.push_back(128);
        chunk_sizes.push_back(256);

        EXPECT_TRUE(mem_pool->Initialize(chunk_sizes));

    }

    TEST_F(MemPoolInitialize, ChunkSizesAreNotMultiple) {
        chunk_sizes.push_back(64);
        chunk_sizes.push_back(126);
        chunk_sizes.push_back(256);

        EXPECT_FALSE(mem_pool->Initialize(chunk_sizes));
    }

    TEST_F(MemPoolInitialize, ChunkSizesBiggerThanMemory) {
        chunk_sizes.push_back(64);
        chunk_sizes.push_back(128);
        chunk_sizes.push_back(256);
        chunk_sizes.push_back(1024);

        EXPECT_FALSE(mem_pool->Initialize(chunk_sizes));
    }

    class MemPoolManagement : public ::testing::Test {
    protected:
        void SetUp() override {
            mem_pool = new MemPool(2048, 8);
        }

        void TearDown() override {
            delete(mem_pool);
        }

        std::vector<uint32_t> chunk_sizes;
        MemPool* mem_pool;
    };

    TEST_F(MemPoolManagement, BigAllocation) {
        chunk_sizes.push_back(64);
        chunk_sizes.push_back(128);
        chunk_sizes.push_back(256);

        ASSERT_TRUE(mem_pool->Initialize(chunk_sizes));

        EXPECT_EQ(mem_pool->Alloc(512), nullptr);
    }

    TEST_F(MemPoolManagement, AllocFree) {
        chunk_sizes.push_back(64);
        chunk_sizes.push_back(128);
        chunk_sizes.push_back(256);

        ASSERT_TRUE(mem_pool->Initialize(chunk_sizes));

        void* chunk[4];
        for (int i = 0; i < 4; i++)
        {
            chunk[i] = mem_pool->Alloc(256);
            EXPECT_NE(chunk[i], nullptr);
        }

        for (int i = 0; i < 4; i++)
        {
            EXPECT_TRUE(mem_pool->Free(chunk[i]));
        }
    }

    TEST_F(MemPoolManagement, AllocOverload) {
        chunk_sizes.push_back(64);
        chunk_sizes.push_back(128);
        chunk_sizes.push_back(256);

        ASSERT_TRUE(mem_pool->Initialize(chunk_sizes));

        void* chunk[8];
        for (int i = 0; i < 8; i++)
        {
            chunk[i] = mem_pool->Alloc(256);
            EXPECT_NE(chunk[i], nullptr);
        }

        EXPECT_EQ(mem_pool->Alloc(20), nullptr);

        for (int i = 0; i < 8; i++)
        {
            EXPECT_TRUE(mem_pool->Free(chunk[i]));
        }
    }

    TEST_F(MemPoolManagement, FreeWrongPointer) {
        chunk_sizes.push_back(64);
        chunk_sizes.push_back(128);
        chunk_sizes.push_back(256);

        ASSERT_TRUE(mem_pool->Initialize(chunk_sizes));

        void* chunk1 = mem_pool->Alloc(50);
        ASSERT_NE(chunk1, nullptr);

        void* chunk2 = mem_pool->Alloc(100);
        ASSERT_NE(chunk2, nullptr);

        void* chunk3 = mem_pool->Alloc(200);
        ASSERT_NE(chunk3, nullptr);

        ASSERT_TRUE(mem_pool->Free(chunk1));
        ASSERT_TRUE(mem_pool->Free(chunk2));

        void* temp;
        chunk3 = temp;
        ASSERT_FALSE(mem_pool->Free(chunk3));
    }

    TEST_F(MemPoolManagement, DoubleFree) {
        chunk_sizes.push_back(64);
        chunk_sizes.push_back(128);
        chunk_sizes.push_back(256);

        ASSERT_TRUE(mem_pool->Initialize(chunk_sizes));

        void* chunk1 = mem_pool->Alloc(50);
        ASSERT_NE(chunk1, nullptr);

        void* chunk2 = mem_pool->Alloc(100);
        ASSERT_NE(chunk2, nullptr);

        ASSERT_TRUE(mem_pool->Free(chunk1));
        ASSERT_TRUE(mem_pool->Free(chunk2));

        ASSERT_FALSE(mem_pool->Free(chunk2));
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}