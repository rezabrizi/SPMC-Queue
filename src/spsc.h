/**
 * Author: Reza A Tabrizi
 * Email: Rtabrizi03@gmail.com
 */


#pragma once

#include <cstdint>
#include <functional>
#include <atomic>

using BlockVersion = uint32_t;
using MessageSize = uint32_t;
using WriteCallback = std::function<void(uint8_t* data)>;

struct Block{
    std::atomic<BlockVersion> mVersion{0};
    std::atomic<bool> unread{true};
    std::atomic<MessageSize> mSize{0};
    // 512 Bytes per block
    alignas(64) uint8_t data[64];
};

struct Header{
    alignas(64) std::atomic<uint64_t> mBlockCounter {0};
};

class SPSC_Q {
    SPSC_Q(size_t sz): mSz(sz){
        mBlocks = new Block[mSz];
    }

    ~SPSC_Q(){
        delete[] mBlocks;
    }

    void Write(MessageSize size, WriteCallback c){
        // This ensures that only one writer can write by having an atomic blockIndex and using fetch_add
        uint64_t blockIndex = mHeader.mBlockCounter.fetch_add(1, std::memory_order_acquire) % mSz;
        Block &currBlock = mBlocks[blockIndex];

        BlockVersion expectedVersion = currBlock.mVersion.load(std::memory_order_acquire);

        // if the version is odd make it even
        if (expectedVersion % 2 == 1){
            currBlock.mVersion.store(expectedVersion+1, std::memory_order_release);
        }

        currBlock.mSize.store(size, std::memory_order_release);

        c(currBlock.data);
        // set it to unread so that the reader can read it
        currBlock.unread.store(true, std::memory_order_release);

        // at this point the version will be even because we made sure in the if statement
        expectedVersion = currBlock.mVersion.load(std::memory_order_acquire);
        // make the version odd to indicate ready for a read
        currBlock.mVersion.compare_exchange_strong(expectedVersion, expectedVersion + 1,
                                                   std::memory_order_release);
    }

    bool read(int index, uint8_t* data, MessageSize& size){
        Block &block = mBlocks[index];
        BlockVersion version = block.mVersion.load(std::memory_order_acquire);
        // Can only read if the version is odd
        if (version % 2 == 1){
            bool expected = true;
            // change the unread to false
            if (block.unread.compare_exchange_strong(expected, false, std::memory_order_acquire)){
                // ... do the copy now; FINALLY
                size = block.mSize.load(std::memory_order_acquire);
                std::memcpy(data, block.data, size);
                return true;
            }
        }
        return false;
    }

private:
    size_t mSz;
    Header mHeader;
    Block* mBlocks;
};
