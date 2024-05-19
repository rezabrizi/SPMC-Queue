#pragma once
#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>

using BenchmarkFunction = std::function<void(std::atomic<bool>&)>;

void runBenchmark(const std::string& name, BenchmarkFunction producer, BenchmarkFunction consumer, int numProducers, int numConsumers, int durationSeconds);
