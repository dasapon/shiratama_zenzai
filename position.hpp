#pragma once

#include "shiratama_zenzai.hpp"
#include "bitboard.hpp"

using MoveArray = sheena::Array<Intersection, 362>;

constexpr Intersection pass = 0;
constexpr Intersection resign = 65536;

class Position{
	static sheena::Array2d<uint64_t, 441, 4> hash_seed;
	sheena::VInt<BoardDim> stones, liverty_cache;
	uint64_t key;
	int board_size;
	Intersection kou;
	Stone turn;
	void put(Stone color, Intersection i);
	void remove(Stone color, Intersection i);
	int remove_string(Stone color, Intersection i, BitBoard& liverty_changed);
	int update_liverty_sub(Stone color, Intersection i, BitBoard& done, BitBoard& string)const;
	BitBoard update_liverty(Intersection i);
public:
	Position(int board_size);
	Position(const Position& pos);
	void operator=(const Position& rhs);
	uint64_t get_key()const{return key;}
	int get_board_size()const{return board_size;}
	Stone turn_player()const{return turn;}
	Stone operator[](Intersection i)const{
		return static_cast<Stone>(stones[i]);
	}
	//void operator=(const Board& rhs);
	static void init_hash_seed();
	//黒勝ちなら1, 白勝ちなら-1, 引き分けなら0を返す関数(簡易実装)
	int result(int komix2)const;
	void make_move(Intersection i);
	void clear();
	int generate_moves(MoveArray& moves)const;
};