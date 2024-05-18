#pragma once
#include <stdint.h>
#include <functional>
#include <atomic>

using BlockVersion = uint32_t;
using MessageSize = uint32_t;
using WriteCallback = std::function<void(uint8_t* data)>;

struct Block
{
    std::atomic<BlockVersion> mVersion;
    std::atomic<MessageSize> mSize;
    alignas(64) uint8_t mData[64];
};

struct Header
{
    alignas(64) std::atomic<uint64_t> mBlockCounter {0};
    alignas(64) Block mBlocks[0];
};

constexpr size_t NUM_BLOCKS = 1024;

struct Q {
    Header mHeader;
    Block mBlocks[NUM_BLOCKS];

    Q()
    {
        for (size_t i = 0; i < NUM_BLOCKS; i++)
        {
            mBlocks[i].mVersion.store(0, std::memory_order_relaxed);
            mBlocks[i].mSize.store(0, std::memory_order_relaxed);
        }
    }

    void Write(MessageSize size, WriteCallback writeCallback){
        uint64_t blockIndex = mHeader.mBlockCounter.fetch_add(1, std::memory_order_relaxed) % NUM_BLOCKS;
        Block &block = mBlocks[blockIndex];

        BlockVersion newVersion = block.mVersion.load(std::memory_order_acquire) + 1;
        block.mSize.store(size, std::memory_order_release);

        writeCallback(block.mData);

        block.mVersion.store(newVersion, std::memory_order_release);

    }

    bool Read (uint64_t blockIndex, uint8_t* data, MessageSize& size){
        Block &block = mBlocks[blockIndex];

        BlockVersion version = block.mVersion.load(std::memory_order_acquire);

        if(version % 2 == 1){

            size = block.mSize.load(std::memory_order_acquire);

            std::memcpy(data, block.mData, size);

            block.mVersion.store(version + 1, std::memory_order_release);

            return true;
        }

        return false;
    }
};