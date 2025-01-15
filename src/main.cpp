#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <chrono>
#include <atomic>
#include <boost/lockfree/queue.hpp>
#include "spmc_q.h"
#include "benchmark.h"
#include "BlockingQueue.h"


void smpcProducer(SPMC_Q& queue, std::atomic<bool>& running) {
    int id = 0;
    while (running) {
        std::string message = "Message " + std::to_string(id);
        MessageSize size = message.size() + 1;
        auto writeCallback = [message](uint8_t* data) {
            std::memcpy(data, message.c_str(), message.size() + 1);
        };

        queue.Write(size, writeCallback);
        id++;
    }
}

void smpcConsumer(SPMC_Q& queue, std::atomic<bool>& running, std::atomic<int>& messageCount) {
    uint64_t blockIndex = 0;
    while (running) {
        uint8_t data[64];
        MessageSize size;
        if (queue.Read(blockIndex, data, size)) {
            messageCount++;
            blockIndex++;
            if (blockIndex >= queue.size()) blockIndex = 0;
        } else {
            std::this_thread::yield();
        }
    }
}

void boostQueueProducer(boost::lockfree::queue<int>& queue, std::atomic<bool>& running) {
    int id = 0;
    while (running) {
        while (!queue.push(id)) {
            std::this_thread::yield();
        }
        id++;
    }
}

void boostQueueConsumer(boost::lockfree::queue<int>& queue, std::atomic<bool>& running, std::atomic<int>& messageCount) {
    int data;
    while (running) {
        if (queue.pop(data)) {
            messageCount++;
        } else {
            std::this_thread::yield();
        }
    }
}

void blockingQueueProducer(BlockingQueue<int>& queue, std::atomic<bool>& running) {
    int id = 0;
    while (running) {
        queue.push(id);
        id++;
    }
}

void blockingQueueConsumer(BlockingQueue<int>& queue, std::atomic<bool>& running, std::atomic<int>& messageCount) {
    while (running) {
        int data = queue.pop();
        messageCount++;
    }
}

template <typename QueueType, typename ProducerFunc, typename ConsumerFunc>
void startBenchmark(const std::string& queueName, QueueType& queue, ProducerFunc producerFunc, ConsumerFunc consumerFunc, int numProducers, int numConsumers, int duration, std::atomic<int>& messageCount) {
    auto producerWrapper = [&queue, &producerFunc](std::atomic<bool>& running) {
        producerFunc(queue, running);
    };

    auto consumerWrapper = [&queue, &consumerFunc, &messageCount](std::atomic<bool>& running) {
        consumerFunc(queue, running, messageCount);
    };

    runBenchmark(queueName, producerWrapper, consumerWrapper, numProducers, numConsumers, duration);

    std::cout << queueName << ": " << " blocks " << std::to_string(numProducers) << " producer  " << std::to_string(numConsumers) << " consumers  " << std::to_string(duration) << " seconds" << std::endl;
    std::cout << "Total messages processed: " << messageCount.load() << "\n" << std::endl;
}

void test_spmc (){
    std::atomic<int> messageCount(0);

    SPMC_Q spmcQueue(1024);
    startBenchmark("SMPC Queue", spmcQueue, smpcProducer, smpcConsumer, 1, 1, 5, messageCount);
    messageCount = 0;
    SPMC_Q spmcQueue1(1024);
    startBenchmark("SMPC Queue", spmcQueue1, smpcProducer, smpcConsumer, 1, 3, 5, messageCount);
    messageCount = 0;
    SPMC_Q spmcQueue2(1024);
    startBenchmark("SMPC Queue", spmcQueue2, smpcProducer, smpcConsumer, 1, 10, 5, messageCount);
}

void test_blocking(){
    std::atomic<int> messageCount(0);
    BlockingQueue<int> blockingQueue1(1024);
    startBenchmark("BlockingQueue", blockingQueue1, blockingQueueProducer, blockingQueueConsumer, 1, 1, 5, messageCount);
    messageCount = 0;
    BlockingQueue<int> blockingQueue2(1024);
    startBenchmark("BlockingQueue", blockingQueue2, blockingQueueProducer, blockingQueueConsumer, 1, 3, 5, messageCount);
    messageCount = 0;
    BlockingQueue<int> blockingQueue3(1024);
    startBenchmark("BlockingQueue", blockingQueue3, blockingQueueProducer, blockingQueueConsumer, 1, 10, 5, messageCount);
}

void test_lock_free_boost(){
    std::atomic<int> messageCount(0);

    boost::lockfree::queue<int> boostQueue(1024);
    startBenchmark("Boost Lockfree Queue", boostQueue, boostQueueProducer, boostQueueConsumer, 1, 1, 5, messageCount);
    messageCount = 0;
    boost::lockfree::queue<int> boostQueue1(1024);
    startBenchmark("Boost Lockfree Queue", boostQueue1, boostQueueProducer, boostQueueConsumer, 1, 3, 5, messageCount);
    messageCount = 0;
    boost::lockfree::queue<int> boostQueue2(1024);
    startBenchmark("Boost Lockfree Queue", boostQueue2, boostQueueProducer, boostQueueConsumer, 1, 10, 5, messageCount);
}

int main() {
    test_spmc();
    std::cout << "\n\n\n";
    test_lock_free_boost();
    return 0;
}