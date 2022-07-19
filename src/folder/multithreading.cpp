#include <iostream>
#include <thread>
#include <ctime>

// calculates sum of numbers in range [start, end)
void print(std::string string, int times) {
    for(int i = 0; i < times; i++) {
        std::cout << string;
    }
}

int main() {
    time_t start_time = clock();

    std::thread t0(print, "A", 1000);
    std::thread t1(print, "B", 1000);

    t0.join();
    t1.join();

    time_t elapsed_time = clock() - start_time;
    std::cout << "elapsed time: " << elapsed_time << "\n";
    return 0;
}