#include "profiler.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

int main() {
    std::cout << "[profiler] opening \"uart1.log\"..." << std::endl;
    FILE* file = std::fopen("../uart1.log", "r");

    entry_t buffer[ITEMS_PER_READ];

    std::vector<uint64_t> call_stack[TRACE_CORE_COUNT];
    std::unordered_map<uint64_t, function_trace> func_map;

    size_t total;
    size_t objects_read;
    while ((objects_read = std::fread(buffer, sizeof(entry_t), ITEMS_PER_READ, file)) != 0) {
        for (size_t i = 0; i < objects_read; i++) {
            entry_t* current = buffer + i;
            uint8_t core_id = (current->flags >> 8);

            if(core_id == 0)
                std::cout << "!" << std::endl;

            if (current->flags & 0x1) {
                call_stack[core_id].push_back(current->function);
                call_stack[core_id].push_back(current->time);

                auto found = func_map.find(current->function);

                if (found == func_map.end()) {
                    func_map.insert(std::make_pair(current->function, function_trace(1, 0)));
                } else {
                    found->second.num_executions += 1;
                }

            } else if (call_stack[core_id].size() > 0) {
                auto start_time = call_stack[core_id].back();
                call_stack[core_id].pop_back();
                auto function = call_stack[core_id].back();
                call_stack[core_id].pop_back();

                if (function != current->function) {
                    // std::cout << "[profiler] received non-matching function from call_stack..."
                    //   << std::endl;
                    //               << std::endl;
                    //     std::cout << "=> extended: " << function << ", received: " <<
                    //     current->function
                    //               << std::endl;
                    //     exit(1);
                    continue;
                }

                func_map.at(function).total_time += (current->time - start_time);
            }
        }
        total += objects_read;
    }

    for (auto ptr = func_map.begin(); ptr != func_map.end(); ptr++) {
        std::cout << ptr->first << "'s total_time: " << ptr->second.total_time
                  << ", executions=" << ptr->second.num_executions << std::endl;
    }

    for (int i = 0; i < TRACE_CORE_COUNT; i++) {
        std::cout << i << ")call stack size (unmatched): " << call_stack[i].size() << std::endl;
    }

    return 0;
}