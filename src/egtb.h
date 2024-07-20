#ifndef _EGTB_H_
#define _EGTB_H_

#include "tb/types.h"

#include <tuple>
#include <vector>
#include <memory>
#include <string>


class Position;


struct MoveInfo
{
	std::string san;
	int16_t dtw;
	uint8_t dtz;
	bool is_dtz_ok;
	Move move;

	MoveInfo(const std::string& san, Move move, int16_t dtw, uint8_t dtz, bool is_dtz_ok = true);
};

std::string move2san(Position& pos, const Move& move);

std::tuple<int16_t, uint8_t, bool> probe_fen(const std::string& fen);
std::vector<MoveInfo> get_endgame_moves(const std::string& fen);
std::string next_fen(const std::string& fen, Move move);

void init(int argc, char* argv[]);
bool init_EGTB(const std::string& egtb_path);
void close_TB();

#endif
