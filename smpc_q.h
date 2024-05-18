
#pragma once

#include <cstdint>
#include <functional>
#include <atomic>

using BlockVersion = uint32_t;
using MessageSize = uint32_t;
using WriteCallback = std::function<void(uint8_t* data)>;

struct Block
{
    // Local block versions reduce contention for the queue
    std::atomic<BlockVersion> mVersion;
    // Size of the data
    std::atomic<MessageSize> mSize;
    // 64 byte buffer
    alignas(64) uint8_t mData[64];
};

struct Header
{
    // Block count
    alignas(64) std::atomic<uint64_t> mBlockCounter {0};
};


struct Q {
    Header mHeader;
    Block* mBlocks;
    size_t mSz;

    Q(size_t sz): mSz(sz)
    {
        mBlocks = new Block[mSz];
        for (size_t i = 0; i < mSz; i++)
        {
            mBlocks[i].mVersion.store(0, std::memory_order_relaxed);
            mBlocks[i].mSize.store(0, std::memory_order_relaxed);
        }
    }
    ~Q()
    {
        delete[] mBlocks;
    }

    void Write(MessageSize size, WriteCallback c){
        // the next block index to write to
        uint64_t blockIndex = mHeader.mBlockCounter.fetch_add(1, std::memory_order_relaxed) % mSz;
        Block &block = mBlocks[blockIndex];

        BlockVersion currentVersion = block.mVersion.load(std::memory_order_acquire) + 1;

        // the block has been written to before so it has an odd version
        // we need to make its version even before writing begins to indicate that writing is in progress
        if (block.mVersion % 2 == 1){
            // make the version even
            block.mVersion.store(currentVersion, std::memory_order_release);
            // store the newVersion used for after the writing is done
            currentVersion++;
        }
        // store the size
        block.mSize.store(size, std::memory_order_release);
        // perform write using the callback function
        c(block.mData);
        // store the new odd version
        block.mVersion.store(currentVersion, std::memory_order_release);
    }

    bool Read (uint64_t blockIndex, uint8_t* data, MessageSize& size) const {
        // Block
        Block &block = mBlocks[blockIndex];
        // Block version
        BlockVersion version = block.mVersion.load(std::memory_order_acquire);
        // Read when version is odd
        if(version % 2 == 1){
            // Size of the data
            size = block.mSize.load(std::memory_order_acquire);
            // Perform the read
            std::memcpy(data, block.mData, size);
            // Indicate that a read has occurred by adding a 2 to the version
            // However do not block subsequent reads
            block.mVersion.store(version + 2, std::memory_order_release);
            return true;
        }
        return false;
    }
};