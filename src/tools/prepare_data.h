#include <vector>
#include "position.h"
#include "utils.h"
#include "constants.h"
const int HISTORY_FRAMES = 8;
const int PIECE_CHANNELS_PER_FRAME = 12;
const int STATE_CHANNELS = 9; // Example number of state channels
const int TOTAL_CHANNELS = (HISTORY_FRAMES * PIECE_CHANNELS_PER_FRAME) + STATE_CHANNELS;
const int TENSOR_SIZE = TOTAL_CHANNELS * 64;

std::vector<float> pos_to_tensor_with_history(const Position& pos) {
    std::vector<float> tensor(TENSOR_SIZE, 0.0f);

    const auto& history = pos.get_history(); 
    int history_size = history.size();

    // Loop through the number of frames we want to include (up to 8)
    for (int i = 0; i < HISTORY_FRAMES; ++i) {
        int history_index = history_size - 1 - i;

        // If we've run out of past moves, this frame will remain all zeros
        if (history_index < 0) continue;

        const PositionState& state = history[history_index];
        int frame_offset = i * PIECE_CHANNELS_PER_FRAME*64;
        // Create plaen fille function
        auto fill_plane = [&](int channel_index, uint64_t bitpos) {
            int offset = channel_index * 64;
            while(bitpos) {
				int square = get_lsb(bitpos);
				tensor[frame_offset + offset + square] = 1.0f;
				bitpos &= bitpos - 1; // Clear the least significant bit
            }
			};
        // Function to fill piece planes for a given state
        auto fill_piece_planes_for_state = [&](const auto& pieces) {
            for (int piece_type = 0; piece_type < 6; ++piece_type) {
                uint64_t bitpos_white = pieces[0][piece_type];
                uint64_t bitpos_black = pieces[1][piece_type];

				fill_plane(piece_type, bitpos_white); // White pieces
				fill_plane(piece_type + 6, bitpos_black); // Black pieces
            }
            };

        fill_piece_planes_for_state(state.pieces);
    }

    // --- Now, add the state planes for the CURRENT position only ---
    int state_offset = HISTORY_FRAMES * PIECE_CHANNELS_PER_FRAME*64;

	float turn_val = (pos.get_turn() == Color::White) ? 1.0f : 0.0f;  

    std::fill(tensor.begin() + state_offset, tensor.begin() + state_offset + 64, turn_val);
	state_offset += 64;
    uint8_t castle_rights = pos.get_castle_rights();
    if (castle_rights & WHITE_KING_CASTLE)  std::fill(tensor.begin() + state_offset, tensor.begin() + state_offset + 64, 1.0f);
	state_offset += 64;
    if (castle_rights & WHITE_QUEEN_CASTLE) std::fill(tensor.begin() + state_offset, tensor.begin() + state_offset + 64, 1.0f);
	state_offset += 64;
    if (castle_rights & BLACK_KING_CASTLE)  std::fill(tensor.begin() + state_offset, tensor.begin() + state_offset + 64, 1.0f);
    state_offset += 64;
    if (castle_rights & BLACK_QUEEN_CASTLE) std::fill(tensor.begin() + state_offset, tensor.begin() + state_offset + 64, 1.0f);
	state_offset += 64;

	float moves_val = static_cast<float>(pos.get_move_count())/200.0f;
	std::fill(tensor.begin() + state_offset, tensor.begin() + state_offset + 64, moves_val);
    state_offset+=64;

    // Plane for Half Move Clock (1 channel)
    float half_move_val = static_cast<float>(pos.get_half_moves()) / 100.0f;
    // CORRECTED: Use the current_state_channel to get the correct position
    std::fill(tensor.begin() + (state_offset), tensor.begin() + state_offset + 64, half_move_val);
	state_offset += 64;
	float rep_val = static_cast<float>(pos.get_position_repeat_count());
    if(rep_val>0) std::fill(tensor.begin() + (state_offset), tensor.begin() + state_offset + 64, 1.0f);
	state_offset += 64;
	if (rep_val > 1) std::fill(tensor.begin() + (state_offset), tensor.begin() + state_offset + 64, 1.0f);

    return tensor;
}