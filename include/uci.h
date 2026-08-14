#pragma once

#include <string>
#include <utility>
#include <vector>

void uci_loop();
std::vector<std::pair<std::string, std::string>> get_default_positions();
std::vector<std::pair<std::string, std::string>> get_bench_positions();
std::vector<std::pair<std::string, std::string>> get_full_bench_positions();
