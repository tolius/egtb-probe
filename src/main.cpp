#include "egtb.h"
#include "tb/egtb/tb_reader.h"
#include "tb/misc.h"

#include <algorithm> 
#include <tuple> 
#include <cctype>
#include <locale>
#include <iostream>
#include <iomanip>
#include <charconv>


using namespace std;


void test()
{
	constexpr int16_t DRAW = egtb::DRAW;
	using Test = tuple<string, int16_t, uint8_t, bool>;
	auto tests = {
		Test("7k/8/5K2/8/8/1R6/P7/8 w - - 99 1",   50,   2, false),
		Test("7k/8/5K2/8/8/1R6/P7/8 w - - 98 1",   50,   2, true ),
		Test("8/8/8/8/8/3k4/8/1R6 b - - 0 1",     -35,  35, true ),
		Test("8/8/8/8/8/P7/8/7k b - - 0 1",       -45,   2, true ),
		Test("8/8/K7/8/8/6b1/8/3b4 w - - 0 1",    -47,  45, true ),
		Test("8/4p3/8/8/8/P7/P7/8 w - - 99 105",   68,   1, true ),
		Test("2Q2R2/8/8/8/8/8/8/1K5k w - - 0 1",  110,  76, true ),
		Test("8/1P6/8/8/8/K2P4/8/2k5 w - - 0 1",  142,   7, true ),
		Test("8/5N2/5B2/8/k7/8/8/2K5 w - - 0 1",  148, 101, false),
		Test("8/8/8/8/8/5P2/1B6/2N3k1 w - - 0 1", 174, 101, false),
		Test("8/8/8/8/8/5Pp1/3B4/6N1 w - - 0 1",  174, 101, false),
		Test("8/8/8/5N2/8/B4P2/8/6k1 w - - 0 1",  162, 101, false),
		Test("8/8/8/8/3N4/B4P2/8/6k1 b - - 0 1", -161, 101, false),
		Test("8/5P2/8/8/3N4/B7/8/7k w - - 0 1",    72,   1, true ),
		Test("8/2NK4/8/8/8/B2k4/8/8 b - - 0 1",  -105, 101, false),
		Test("8/2NK4/8/8/8/B7/2k5/8 w - - 1 2",   104, 100, false),
		Test("8/2NK4/8/8/8/B7/2k5/8 w - - 0 1",   104, 100, true ),
		Test("8/8/8/8/k7/8/8/K3K1N1 w - - 0 1",   104, 100, true ),
		Test("8/8/8/8/k7/8/8/K3K1N1 w - - 1 2",   104, 100, false),
		Test("8/8/8/8/k7/8/4N3/K3K3 b - - 1 1",  DRAW,   0, true ),
		Test("8/8/8/8/1k6/8/4N3/K3K3 w - - 0 1", DRAW,   0, true ),
		Test("8/p7/8/1P6/7p/7R/8/8 b - - 0 25",     4,   1, true ),
		Test("8/8/8/pP6/7p/7R/8/8 w - a6 0 26",    -3,   1, true ),
	};
	for (auto& test : tests)
	{
		auto& fen = get<0>(test);
		auto& dtw = get<1>(test);
		auto& dtz = get<2>(test);
		auto& is_dtz_ok = get<3>(test);
		cout << "Test " << setw(40) << left << fen << ": ";
		auto data = probe_fen(fen);
		if (get<0>(data) != dtw)
			cout << "DTW " << get<0>(data) << " != " << dtw << " --> ERROR";
		else if (get<1>(data) != dtz)
			cout << "DTZ " << (int)get<1>(data) << " != " << (int)dtz << " --> ERROR";
		else if (get<2>(data) != is_dtz_ok)
			cout << "50-move rule flag: " << get<2>(data) << " != " << is_dtz_ok << " --> ERROR";
		else
			cout << "PASSED";
		cout << endl;
	}
}


void trim(string& s)
{
	s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) { return !isspace(ch); }));
	s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !isspace(ch); }).base(), s.end());
}


int main(int argc, char* argv[])
{
	init(argc, argv);
	string egtb_path = CommandLine::binaryDirectory + "EGTB";
	bool is_init = init_EGTB(egtb_path);
	if (!is_init)
	{
		cout << "\nError: Not all tablebase files have been loaded." << endl;
		close_TB();
		return -1;
	}

	test();
	cout << "\nEnter your FEN, or a number to select a move, or 'q' to exit:\n";

	string fen;
	vector<MoveInfo> moves;
	for (;;)
	{
		string s;
		getline(cin, s);
		trim(s);
		if (s == "quit" || s == "q")
			break;
		if (s == "test" || s == "t")
		{
			test();
			continue;
		}

		if (!fen.empty() && s.length() >= 1 && s.length() <= 2)
		{
			size_t val;
			auto res = from_chars(s.data(), s.data() + s.size(), val).ec;
			if (res != std::errc{} || val == 0 || val > moves.size())
				continue;
			fen = next_fen(fen, moves[val - 1].move);
			if (fen.empty())
				continue;
			cout << fen << "\n";
		}
		else
		{
			fen = s;
		}

		moves = get_endgame_moves(fen);
		size_t i = 0;
		for (auto& move : moves)
		{
			if (i != 0)
				cout << (i % 3 == 0 ? "\n" : "    ");
			i++;
			int16_t num_full_moves = (move.dtw < 0) ? (-move.dtw + 1) / 2 : ((-move.dtw - 1) / 2);
			string dtw = (abs(move.dtw) >= egtb::DRAW)    ? "draw"
			           : (move.dtw == 0 && move.dtz == 0) ? "loss"
			           : move.is_dtz_ok                   ? ("#" + to_string(num_full_moves))
			                                              : ("#" + to_string(num_full_moves) + "*"); // cursed-win or blessed-loss
			cout << setw(2) << right << i << ": " << setw(7) << left << move.san;
			cout << setw(5) << left << dtw << " DTZ=" << setw(3) << left << (int)move.dtz;
		}
		cout << endl;
	}

	close_TB();
	return 0;
}
