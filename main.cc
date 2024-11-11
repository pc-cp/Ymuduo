#include <atomic>
#include <thread>
#include <iostream>
#include <thread>
#include <memory>
std::atomic_bool quit_(false);

void worker() {
    while(!quit_.load()) {
        std::this_thread::sleep_for(std::chrono::microseconds(500000)); // 0.5 second
        std::cout << "Working..." << std::endl;
    }
    std::cout << "Thread exiting..." << std::endl;
}

int main() {
    std::shared_ptr<std::thread> t;
    t = std::make_shared<std::thread>([]()->void{ std::cout << "......" << std::endl; });

    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // quit_.store(true);

    t->join();
    return 0;
}