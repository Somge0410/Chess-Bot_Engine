#include <iostream>
#include "board.h"
#include "utils.h"    
#include "constants.h" 
#include "zobrist.h"
#include "attack_rays.h"
#include "bitboard_masks.h"
#include "MoveGenerator.h"
#include "notation_utils.h"
#include "evaluation.h"
#include "engine.h"
#include "rook_tables.h"
#include "bishop_tables.h"
#include <chrono>
#include "prepare_data.h"
#include <vector>
#include "uci.h"
#include "see.h"
#include "Squares.h"

#include <iostream>
void print_2d_array(const std::string& name, const std::vector<std::vector<uint64_t>>& arr) {
    if (arr.empty() || arr[0].empty())
    {
        std::cout << "const uint64_t " << name << "[0][0] = {};" << std::endl;
        return;
    }
    size_t rows = arr.size();
    size_t cols = arr[0].size();

    std::cout << "const uint64_t " << name << "[" << rows << "][" << cols << "] = {" << std::endl;
    for (size_t i = 0; i < rows; ++i)
    {
        std::cout << "    { ";
        for (size_t j = 0; j < cols; ++j)
        {
            std::cout << "0x" << std::hex << arr[i][j] << "ULL," << (j % 8 == 7 ? "\n      " : " ");
        }
        std::cout << "}," << std::endl;
    }
    std::cout << "};" << std::endl << std::endl;

}
int main(int argc, char* argv[]) {
	/*std::vector<std::vector<uint64_t>> reduction_depth(64, std::vector<uint64_t>(218, 0));
    for (int depth = 0; depth < 64; ++depth) {
        for (int move_count = 0; move_count < 218; ++move_count) {
            reduction_depth[depth][move_count] = std::round(0.2 + std::log(depth + 1) * std::log(move_count + 1) / 3.35);
        }
    }

    print_2d_array("Q-REDUCTION_AMOUNT", reduction_depth);*/
    Zobrist::initialize_keys();
        std::ios::sync_with_stdio(false);
        std::cin.tie(nullptr);
        uci_loop();
        return 0;
}