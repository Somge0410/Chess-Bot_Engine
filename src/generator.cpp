#include <iostream>
#include <cstdint>
#include <vector>
#include "constants.h"
#include <array>
void print_array(const std::string& name, const std:: vector<uint64_t>& arr){
    size_t size=arr.size();
    std:: cout <<"const uint64_t "<<name << "["<<size<<"] = {" << std::endl;
    for (int i = 0; i < size; ++i)
    {
        if (i % 8 == 0) std::cout << "    ";
        std::cout << "0x" << std::hex << arr[i] << "ULL, ";
        if (i % 8 == 7) std::cout<< std::endl;
    }
    if (size % 8 !=0)
    {
        std::cout << std::endl;
    }
    
    std::cout << "};"<< std::endl << std::endl;
    
}

void print_2d_array(const std::string& name, const std:: vector<std::vector<uint64_t>>& arr){
    if (arr.empty()|| arr[0].empty())
    {
        std::cout << "const uint64_t " << name << "[0][0] = {};" << std::endl;
        return;
    }
    size_t rows=arr.size();
    size_t cols=arr[0].size();
    
    std::cout <<"const uint64_t " <<name << "[" <<rows<<"]["<< cols <<"] = {"<< std::endl;
    for (size_t i = 0; i < rows; ++i)
    {
        std::cout << "    { ";
        for (size_t j = 0; j < cols; ++j)
        {
            std::cout << "0x"<<std::hex <<arr[i][j] << "ULL,"<< (j % 8 ==7 ? "\n      " : " ");
        }
        std::cout << "},"<< std::endl;
    }
    std::cout <<"};" << std::endl << std::endl;
    
}
void print_2d_int_array(const std::string& name, const std::vector<std::vector<int>>& arr) {
    if (arr.empty() || arr[0].empty())
    {
        std::cout << "const uint64_t " << name << "[0][0] = {};" << std::endl;
        return;
    }
    size_t rows = arr.size();
    size_t cols = arr[0].size();

    std::cout << "const int " << name << "[" << rows << "][" << cols << "] = {" << std::endl;
    for (size_t i = 0; i < rows; ++i)
    {
        std::cout << "    { ";
        for (size_t j = 0; j < cols; ++j)
        {
            std::cout <<arr[i][j] <<", "<< (j % 8 == 7 ? "\n      " : " ");
        }
        std::cout << "}," << std::endl;
    }
    std::cout << "};" << std::endl << std::endl;

}
std::array<std::vector<uint64_t>,64> rook_blocker_combinations;
generate_blocker_subsets(int square,uint64_t blocker_mask) {
	rook_blocker_combinations[square].push_back(0);
	uint64_t subset = blocker_mask;
    while (subset > 0) {
                rook_blocker_combinations[square].push_back(subset);
				subset = (subset - 1) & blocker_mask;
    }
}
uint64_t calculate_rook_blocker_mask(int square) {
    uint64_t mask = 0;
    int rank = square / 8;
    int file = square % 8;
    // Horizontal (rank)
    for (int f = file + 1; f <= 6; ++f) mask |= (1ULL << (rank * 8 + f));
    for (int f = file - 1; f >= 1; --f) mask |= (1ULL << (rank * 8 + f));
    // Vertical (file)
    for (int r = rank + 1; r <= 6; ++r) mask |= (1ULL << (r * 8 + file));
    for (int r = rank - 1; r >= 1; --r) mask |= (1ULL << (r * 8 + file));
    return mask;
}
void find_all_combinations() {
    for (int square = 0; square < 64; ++square) {
        uint64_t blocker_mask = calculate_rook_blocker_mask(square);
        generate_blocker_subsets(square, blocker_mask);
    }
}
bool is_magic_number(uint64_t magic,int square) {
	uint64_t calculate_rook_blocker_mask(int square);
    int relevant_bits = popcount(calculate_rook_blocker_mask(square));
    int combinations = 1 << relevant_bits;
    std::vector<uint64_t> used_attacks(combinations, 0);
    for (uint64_t blockers : rook_blocker_combinations[square]) {
        uint64_t index = (blockers * magic) >> (64 - relevant_bits);
        if (used_attacks[index] == 0) {
            used_attacks[index] = 1;
        } else if (used_attacks[index] != 0) {
            return false; // Collision detected
        }
    }
	return true; // No collisions, valid magic number
}
std::array<uint64_t> magic_rook_number;
for(int square=0;square<64;++square){
    
}

int main(){
    // Knight Attacks
    const int KNIGHT_DIRECTIONS[]={6,15,17,10,-6,-17,-15,-10};
    std::vector<uint64_t> knight_attacks(64,0);
    for (int square = 0; square < 64; ++square)
    {
        for (int direction: KNIGHT_DIRECTIONS)
        {
            int target_square=square+direction;
            if (target_square>=0 && target_square<64)
            {
                if (std::abs((target_square % 8)-(square % 8)) <= 2)
                {
                    knight_attacks[square] |= (1ULL << target_square);
                }
                
            }
            
        }
        
    }
    
    // King ATTACKS
    const int KING_DIRECTIONS[]={7,8,9,1,-7,-8,-9,-1};
    std::vector<uint64_t> king_attacks(64,0);
    for (int square = 0; square < 64; ++square)
    {
        for (int  direction:KING_DIRECTIONS)
        {
            int target_square=square+direction;
            if (target_square>=0 && target_square<64)
            {
                if (std::abs((target_square % 8)-(square % 8))<=1)
                {
                    king_attacks[square] |= (1ULL << target_square);
                }
                
            }
            
        }
        
    }

    //Bishop Attacks

    const int BISHOP_DIRECTIONS[]={7,9,-7,-9};
    std::vector<uint64_t> bishop_attacks(64,0);
    for (int square = 0; square < 64; ++square)
    {
        for (int direction : BISHOP_DIRECTIONS)
        {
            int target_square=square+direction;

            while (target_square>=0 && target_square<64)
            {
                if (std::abs((target_square % 8)-((target_square-direction) % 8))<=1)
                {
                    bishop_attacks[square]|=(1ULL << target_square);
                    target_square+=direction;
                }else
                {
                    break;
                }
                
                
            }
            
        }
        
    }

    // Rook attacks:

    const int ROOK_DIRECTIONS[]={8,1,-8,-1};
    std::vector<uint64_t> rook_attacks(64,0);
    for (int square = 0; square < 64; ++square)
    {
        for (int direction : ROOK_DIRECTIONS)
        {
            int target_square=square+direction;

            while (target_square>=0 && target_square<64)
            {
                if (std::abs((target_square % 8)-((target_square-direction) % 8))<=1)
                {
                    rook_attacks[square]|=(1ULL << target_square);
                    target_square+=direction;
                }else
                {
                    break;
                }
                
                
            }
            
        }
        
    }
    // QUEEN attacks:

    const int QUEEN_DIRECTIONS[]={7,8,9,1,-7,-8,-9,-1};
    std::vector<uint64_t> queen_attacks(64,0);
    for (int square = 0; square < 64; ++square)
    {
        for (int direction : QUEEN_DIRECTIONS)
        {
            int target_square=square+direction;

            while (target_square>=0 && target_square<64)
            {
                if (std::abs((target_square % 8)-((target_square-direction) % 8))<=1)
                {
                    queen_attacks[square]|=(1ULL << target_square);
                    target_square+=direction;
                }else
                {
                    break;
                }
                
                
            }
            
        }
        
    }
    // Pawn Attacks
    std::vector<std::vector<uint64_t>> pawn_attacks(2,std::vector<uint64_t>(64,0));
    for (int square = 0; square < 64; ++square)
    {
        uint64_t white_attacks=0;

        if (square % 8 >0)
        {
            if (square+7<64)
            {
                white_attacks|= (1ULL << (square+7));
            }
            
        }
        if ((square % 8)<7)
        {
            if (square+9<64)
            {
                white_attacks|= (1ULL <<(square+9));
            }
            
        }
        pawn_attacks[0][square]=white_attacks;

        uint64_t black_attacks=0;

        if (square % 8 >0)
        {
            if (square-9>=0)
            {
                black_attacks|= (1ULL << (square-9));
            }
            
        }
        if ((square % 8)<7)
        {
            if (square-7>0)
            {
                black_attacks|= (1ULL <<(square-7));
            }
            
        }
        pawn_attacks[0][square]=white_attacks;
        pawn_attacks[1][square]=black_attacks;
        
    }
    

    // Line Between Mask (includes endpoints)

    std::vector<std::vector<uint64_t>> line_between(64,std::vector<uint64_t>(64,0));

    for (int from_sq = 0; from_sq < 64; ++from_sq)
    {
        for (int  to_square = 0; to_square < 64; ++to_square)
        {
            int r1=from_sq/8, f1=from_sq %8;
            int r2=to_square/8, f2=to_square %8;

            uint64_t line_bb=0;

            if (r1 == r2|| f1 == f2|| std::abs(r1-r2) == std::abs(f1-f2))
            {
                int rank_step=(r2>r1)-(r2<r1);
                int file_step=(f2>f1)-(f2<f1);
                int step = rank_step*8+file_step;

                int current_sq = from_sq;

                while (current_sq!=to_square)
                {
                    line_bb|=(1ULL << current_sq);
                    current_sq+=step;
                }
                line_bb |=(1ULL <<to_square);
            }
            line_between[from_sq][to_square]=line_bb;
        }
    }
    // King Shield_Masks

    std::vector<std::vector<uint64_t>> king_shield_mask(2,std::vector<uint64_t>(64,0));

    for (int square = 0; square < 64; ++square)
    {
        int rank=square/8, file=square%8;
        uint64_t shield=0;

        if (file==0)
        {
            if (rank<7)
            {
                king_shield_mask[0][square]=(1ULL << square+8)|(1ULL << square+9);
            }
            if (rank>0)
            {
                king_shield_mask[1][square]=(1ULL << square-8)|(1ULL << square-7);
            }
        }else if (file==7)
        {
            if (rank<7)
            {
                king_shield_mask[0][square]=(1ULL<<square+8)|(1ULL << square+7);
            }
            if (rank>0)
            {
                king_shield_mask[1][square]=(1ULL << square-8)|(1ULL << square-9);
            }
        }else
        {
            if (rank<7)
            {
                king_shield_mask[0][square]=(1ULL <<square+8)|(1ULL<< square+9)|(1ULL<<square+7);
            }
            if (rank>0)
            {
                king_shield_mask[1][square]=(1ULL<<square-8)|(1ULL<< square-7)|(1ULL <<square-9);
            }
        }
    }
    
    // king zone mask
    std::vector<uint64_t> king_zone(64,0);
    int direction[]={-18,-17,-16,-15,-14,-10,-9,-8,-7,-6,-5,-2,-1,1,2,6,7,8,9,10,14,15,16,17,18};
    for (int square = 0; square < 64; ++square)
    {
        uint64_t zone=0;
        for (int d:direction)
        {
            if (square+d>0 && square+d<64)
            {
                if (std::abs(square %8-(square+d) %8)<=2)
                {
                    zone|=(1ULL << square+d);
                }
            }
            
        }
    king_zone[square]=zone;
    }
    

    std::vector<std::vector<uint64_t>> ray_mask(8,std::vector<uint64_t>(64,0));

    for (int square = 0; square < 64; ++square)
    {
        for (int dir_index=0; dir_index<8; ++dir_index)
        {   
            int dir=DIRECTIONS[dir_index];
            uint64_t ray=0;
            int target_square=square+dir;

            while (target_square>=0 && target_square<64)
            {
                if (std::abs((target_square%8)-(target_square-dir) %8)>1)
                {
                    break;
                }
                ray|=1ULL << target_square;
                target_square+=dir;
                
            }
            ray_mask[dir_index][square]=ray;
        }
        
    }

    std::vector<uint64_t> file_mask(8,0);
    for (size_t i = 0; i < 8; ++i)
    {
        file_mask[i]=0x0101010101010101ULL<<i;
    }
    
    std::vector<uint64_t> rank_mask(8,0);
    for (size_t i = 0; i < 8; ++i)
    {
        rank_mask[i]=0xffULL<< 8*i;
    }

    std::vector<uint64_t> adjacend_file_mask(8,0);
    for (size_t i = 0; i < 8; ++i)
    {
        if (i==0)
        {
            adjacend_file_mask[0]=file_mask[1];
        }else if (i==7)
        {
            adjacend_file_mask[7]=file_mask[6];
        }else
        {
            adjacend_file_mask[i]=file_mask[i-1] | file_mask[i+1];
        }
        
        
        
    }
    
    
    std::vector<std::vector<uint64_t>> passed_pawn_mask(2,std::vector<uint64_t>(64,0));

    for (size_t square = 0; square < 64; ++square)
    {
        uint64_t files=adjacend_file_mask[square %8] | file_mask[square % 8];
        uint64_t forward_ranks=0;
        uint64_t backward_ranks=0;

        for (size_t row = square/8+1; row < 8; ++row)
        {
            forward_ranks|=rank_mask[row];
        }
        for (size_t row = 0; row < square/8; ++row)
        {
            backward_ranks|=rank_mask[row];
        }
        passed_pawn_mask[0][square]=forward_ranks & files;
        passed_pawn_mask[1][square]=backward_ranks & files;
        
    }
	std::vector<std::vector<int>> distance_bonus(64, std::vector<int>(64, 0));
    for (int king_square = 0; king_square < 64; ++king_square) {
        for (int att_sq = 0; att_sq < 64; ++att_sq) {
            int rank_diff = std::abs((king_square / 8) - (att_sq / 8));
            int file_diff = std::abs((king_square % 8) - (att_sq % 8));
            int king_distance = std::max(rank_diff,file_diff);
            distance_bonus[king_square][att_sq] = std::max(0, 3 - king_distance); 
		}
    }
    
    


    // print_array("KNIGHT_ATTACKS", knight_attacks);
    // print_array("KING_ATTACKS",king_attacks);
    // print_array("BISHOP_ATTACKS",bishop_attacks);
    // print_array("ROOK_ATTACKS",rook_attacks);
    // print_array("QUEEN_ATTACKS",rook_attacks);
    // print_array("WHITE_PAWN_ATTACKS",pawn_attacks[0]);
    //print_2d_array("PAWN_ATTACKS",pawn_attacks);
    //print_2d_array("LINE_BETWEEN",line_between);
    //print_2d_array("KING_SHIELD",king_shield_mask);
    //print_array("KING_ZONE",king_zone);
    //print_2d_array("RAY_MASK",ray_mask);
    //print_array("FILE_MASK", file_mask);
    //print_array("RANK_MASK",rank_mask);
    //print_array("ADJACENT_FILE_MASK",adjacend_file_mask);
    //print_2d_array("PASSED_PAWN_MASK",passed_pawn_mask);
	print_2d_int_array("DISTANCE_BONUS", distance_bonus);
    return 0;
    
}