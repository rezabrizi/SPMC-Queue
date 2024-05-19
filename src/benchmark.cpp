#include "benchmark.h"

void runBenchmark(const std::string& name, BenchmarkFunction producer, BenchmarkFunction consumer, int numProducers, int numConsumers, int durationSeconds) {
    std::atomic<bool> running(true);
    std::vector<std::thread> producerThreads;
    std::vector<std::thread> consumerThreads;

    for (int i = 0; i < numProducers; ++i) {
        producerThreads.emplace_back(producer, std::ref(running));
    }

    for (int i = 0; i < numConsumers; ++i) {
        consumerThreads.emplace_back(consumer, std::ref(running));
    }

    std::this_thread::sleep_for(std::chrono::seconds(durationSeconds));
    running = false;

    for (auto& t : producerThreads) {
        t.join();
    }
    for (auto& t : consumerThreads) {
        t.join();
    }

    std::cout << name << " benchmark completed." << std::endl;
}
