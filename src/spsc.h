/**
 * Author: Reza A Tabrizi
 * Email: Rtabrizi03@gmail.com
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <cstring>
#include <atomic>
#include <new>


using BlockVersion = uint32_t;
using MessageSize = uint32_t;
using WriteCallback = std::function<void(uint8_t* data)>;

enum class Result {
    SUCCESS,
    ERROR
};

struct Block{
    // 512 Bytes per block
    alignas(std::hardware_destructive_interference_size) uint8_t data[64]{};
    std::atomic<BlockVersion> mVersion{0};
    std::atomic<MessageSize> mSize{0};
    std::atomic<bool> unread{false};
};

struct Header{
    alignas(std::hardware_destructive_interference_size) std::atomic<uint64_t> mBlockCounter {0};
};

class SPSC_Q {
public:

    SPSC_Q(size_t sz): mSz(sz), mBlocks(std::make_unique<Block[]>(sz)){}
    ~SPSC_Q() = default;

    Result Write(MessageSize size, WriteCallback c){
        // This ensures that only one writer can write by having an atomic blockIndex and using fetch_add
        uint64_t blockIndex = mHeader.mBlockCounter.fetch_add(1, std::memory_order_acq_rel) % mSz;
        // Get the current Block
        Block &currBlock = mBlocks[blockIndex];

        // We want to set `unread` to false
        // If we succeed then we can write to the block safely
        bool expectedUnread = true;
        if (!currBlock.unread.compare_exchange_strong(expectedUnread, false, std::memory_order_acquire)){
            // if we didn't succeed and `unread` was already false then the version should be even
            // because either the version was just 0 meaning no writes or reads
            // or a read happened and the version became even because the reader does odd_version + 1 = even_version
            // in the case of the version being even after we didn't succeed we can still proceed with the writing

            // HOWEVER, if we failed to set unread to false because the reader thread set `unread` to false first
            // AND the read is still happening then version is still odd so we CANNOT proceed with writing
            BlockVersion expectedVersion = currBlock.mVersion.load(std::memory_order_acquire);
            if (expectedVersion % 2 == 1){
                return Result::ERROR;
            }
        }

        // Copying the data and setting the size and marking the block as ready to read
        currBlock.mSize.store(size, std::memory_order_release);
        try {
            c(currBlock.data);
        } catch(...) {
            currBlock.unread.store(false, std::memory_order_release);
            throw;
        }

        currBlock.unread.store(true, std::memory_order_release);

        // at this point the version will be even because we made sure in the if statement
        currBlock.mVersion += 1;
        return Result::SUCCESS;
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
                try{
                    std::memcpy(data, block.data, size);
                }catch (...){
                    block.unread.store(true, std::memory_order_release);
                    throw;
                }
                block.mVersion.store(version+1, std::memory_order_release);
                return true;
            }
        }
        return false;
    }

private:
    size_t mSz;
    Header mHeader;
    std::unique_ptr<Block[]> mBlocks;
};
