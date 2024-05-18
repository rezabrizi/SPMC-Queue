#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <chrono>
#include "smpc_q.h"


void producer(Q& queue, std::atomic<bool>& running){
    int id = 0;
    while (running){
        MessageSize size = 32;
        auto writeCallback = [id](uint8_t* data){
            std::string message = "Message " + std::to_string(id);
            std::memcpy(data, message.c_str(), message.size() +1);
        };

        queue.Write(size, writeCallback);
        id++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void consumer (Q& queue, int consumer_id, std::atomic<bool>& running){
    uint64_t blockIndex = 0;
    while (running) {
        uint8_t data[64];
        MessageSize size;
        if(queue.Read(blockIndex, data, size)){
            std::cout << "Consumer " << consumer_id << " received: " << data << std::endl;
            blockIndex ++;
            if (blockIndex >= NUM_BLOCKS)
                blockIndex = 0;
        } else {
            std::this_thread::yield();
        }
    }
}

int main (){
    Q queue;
    std::atomic<bool> running(true);

    std::thread producer_thread(producer, std::ref(queue), std::ref(running));

    const int num_consumers = 3;
    std::vector<std::thread> consumer_threads;

    for(int i = 0; i < num_consumers; i++){
        consumer_threads.emplace_back(consumer, std::ref(queue), i, std::ref(running));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    running = false;

    producer_thread.join();
    for (auto& t: consumer_threads)
    {
        t.join();
    }

    return 0;
}


