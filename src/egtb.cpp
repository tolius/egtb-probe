#include "egtb.h"

#include "tb/egtb/tb_reader.h"
#include "tb/egtb/elements.h"
#include "tb/bitboard.h"
#include "tb/endgame.h"
#include "tb/position.h"
#include "tb/search.h"
#include "tb/thread.h"
#include "tb/tt.h"
#include "tb/uci.h"
#include "tb/syzygy/tbprobe.h"

#include <vector>
#include <array>
#include <iostream>
#include <assert.h>


using namespace std;
using namespace egtb;


namespace PSQT {
	void init();
}

namespace
{
	static constexpr array<char, 8> FILE_NAMES = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };
	static constexpr array<char, 8> RANK_NAMES = { '1', '2', '3', '4', '5', '6', '7', '8' };

#ifdef USE_FAIRY_SF
	static Bitboard FileBB[FILE_NB] = { FileABB, FileABB << 1, FileABB << 2, FileABB << 3, FileABB << 4, FileABB << 5, FileABB << 6, FileABB << 7 };
	static Bitboard RankBB[RANK_NB] = { Rank1BB, Rank1BB << 8, Rank1BB << 16, Rank1BB << 24, Rank1BB << 32, Rank1BB << 40, Rank1BB << 48, Rank1BB << 56 };
#endif
}


MoveInfo::MoveInfo(const std::string& san, Move move, int16_t dtw, uint8_t dtz, bool is_dtz_ok)
	: san(san), move(move), dtw(dtw), dtz(dtz), is_dtz_ok(is_dtz_ok)
{}


string move2san(Position& pos, const Move& move)
{
	stringstream san;
	if (move == MOVE_NULL)
		return "--";

	/// Look ahead for checkmate.
	StateInfo st;
	pos.do_move(move, st);
	bool is_mate = is_anti_win(pos);
	pos.undo_move(move);

	Square from = from_sq(move);
	Square to = to_sq(move);
	auto piece = pos.piece_on(from);
	bool capture = pos.capture(move);

	if (type_of(piece) != PAWN)
		san << " PNBRQK"[type_of(piece)];

	if (piece != PAWN)
	{
		/// Get ambiguous move candidates.
		/// Relevant candidates: not exactly the current move, but to the same square.
		Bitboard others = 0;
		Bitboard from_mask = pos.pieces(pos.side_to_move(), static_cast<PieceType>(type_of(piece)));
		from_mask &= ~SquareBB[from];
		auto to_mask = SquareBB[to];
		MoveList<LEGAL> moves(pos);
		for (auto& candidate : moves)
		{
			auto candidate_from = from_sq(candidate);
			if ((SquareBB[candidate_from] & from_mask) && (SquareBB[to_sq(candidate)] & to_mask))
				others |= SquareBB[candidate_from];
		}

		/// Disambiguate.
		if (others)
		{
			bool row = false;
			bool column = false;

			if (others & RankBB[rank_of(from)])
				column = true;

			if (others & FileBB[file_of(from)])
				row = true;
			else
				column = true;

			if (column)
				san << FILE_NAMES[file_of(from)];
			if (row)
				san << RANK_NAMES[rank_of(from)];
		}
	}
	else if (capture)
		san << FILE_NAMES[file_of(from)];

	/// Captures.
	if (capture)
		san << "x";

	/// Destination square.
	san << FILE_NAMES[file_of(to)] << RANK_NAMES[rank_of(to)];

	/// Promotion.
	if (pos.capture_or_promotion(move) && !pos.capture(move))
#ifdef USE_FAIRY_SF
		san << "=" << pos.variant()->pieceToChar[promotion_type(move)];
#else
		san << "=" << PIECE_SYMBOLS[promotion_type(move)];
#endif // USE_FAIRY_SF

	/// Add check or checkmate suffix.
	if (is_mate)
		san << "#";

	return san.str();
}


bool comp_moves(const MoveInfo& a, const MoveInfo& b)
{
	if ((a.dtw >= 0) != (b.dtw >= 0))
		return a.dtw < 0;

	if (a.is_dtz_ok == b.is_dtz_ok)
		return a.dtw > b.dtw;

	bool a_draw = (abs(a.dtw) >= egtb::DRAW);
	bool b_draw = (abs(b.dtw) >= egtb::DRAW);
	if (a_draw != b_draw)
		return a_draw;

	return a.dtw >= 0 ? b.is_dtz_ok : a.is_dtz_ok;
}


std::tuple<int16_t, uint8_t, bool> probe_fen(const std::string& fen)
{
	try
	{
		StateInfo st;
		Position pos;
		pos.set(fen, false, ANTI_VARIANT, &st, Threads.main());
		int16_t tb_val;
		uint8_t tb_dtz;
		bool is_error = TB_Reader::probe_EGTB(pos, tb_val, tb_dtz);
		if (!is_error)
		{
			int dtz_max = 100 - pos.rule50_count();
			return { tb_val, tb_dtz, tb_dtz <= dtz_max };
		}
	}
	catch (exception e)
	{
		cout << "ERROR " << fen << ": " << e.what() << endl;
	}
	catch (...)
	{
		cout << "ERROR " << fen << ": error" << endl;
	}
	return { 0, 0, false };
}


std::vector<MoveInfo> get_endgame_moves(const std::string& fen)
{
	vector<MoveInfo> tb_moves;
	
	try
	{
		StateInfo st;
		Position pos;
		pos.set(fen, false, ANTI_VARIANT, &st, Threads.main());

		for (const auto& move_i : MoveList<LEGAL>(pos))
		{
			string san = move2san(pos, move_i.move);
			StateInfo st_i;
			pos.do_move(move_i.move, st_i);
			if (is_anti_win(pos))
			{
				tb_moves.emplace_back(san, move_i.move, 0, 0);
			}
			else
			{
				assert(!is_anti_loss(pos));
				int16_t tb_val;
				uint8_t tb_dtz;
				bool is_error = TB_Reader::probe_EGTB(pos, tb_val, tb_dtz);
				if (is_error)
					return vector<MoveInfo>();
				int dtz_max = 100 - pos.rule50_count();
				tb_moves.emplace_back(san, move_i.move, tb_val, tb_dtz, tb_dtz <= dtz_max);
			}
			pos.undo_move(move_i.move);
		}
	}
	catch (exception e)
	{
		cout << "ERROR " << fen << ": " << e.what() << endl;
		return vector<MoveInfo>();
	}
	catch (...)
	{
		cout << "ERROR " << fen << ": error" << endl;
		return vector<MoveInfo>();
	}
	sort(tb_moves.begin(), tb_moves.end(), comp_moves);
	return tb_moves;
}

std::string next_fen(const std::string& fen, Move move)
{
	StateInfo st;
	Position pos;
	pos.set(fen, false, ANTI_VARIANT, &st, Threads.main());
	StateInfo st_next;
	pos.do_move(move, st_next);
	return pos.fen();
}

bool init_EGTB(const std::string& egtb_path)
{
	Tablebases::init(ANTI_VARIANT, egtb_path);
	TB_Reader::init(egtb_path, true);
	return (egtb::TB_Reader::tb_cache.size() >= 714);
}

void init(int argc, char* argv[])
{
	CommandLine::init(argc, argv);
	UCI::init(Options);
	Tune::init();
	PSQT::init();
	Bitboards::init();
	Position::init();
	Bitbases::init();
	Endgames::init();
	Threads.set(size_t(Options["Threads"]));
	Search::clear(); // After threads are up
}

void close_TB()
{
	Threads.set(0);
}
