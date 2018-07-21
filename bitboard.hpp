#pragma once

#include "shiratama_zenzai.hpp"

/*
 * 盤面レイアウト
 * 横21 縦最大21の441
 * BitBoard 64 * 8 = 512
 */
class BitBoard{
	sheena::Array<uint64_t, 8> bb;
public:
	BitBoard(){}
	BitBoard(const BitBoard& rhs){
		*this = rhs;
	}
	bool operator[](Intersection i)const{
		return (bb[i / 64] & (1ULL << (i % 64))) != 0;
	}
	void operator^=(Intersection i){
		bb[i / 64] ^= 1ULL << (i % 64);
	}
	void operator|=(Intersection i){
		bb[i / 64] |= 1ULL << (i % 64);
	}
	void operator|=(const BitBoard& rhs){
		for(int i=0;i<bb.size();i++)bb[i] |= rhs.bb[i];
	}
	void operator&=(const BitBoard& rhs){
		for(int i=0;i<bb.size();i++)bb[i] &= rhs.bb[i];
	}
	BitBoard operator&(const BitBoard rhs){
		BitBoard ret;
		for(int i=0;i<bb.size();i++)ret.bb[i] = bb[i] & rhs.bb[i];
		return ret;
	}
	void operator=(const BitBoard& rhs){
		for(int i=0;i<bb.size();i++)bb[i] = rhs.bb[i];
	}
	bool operator==(const BitBoard rhs)const{
		for(int i=0;i<bb.size();i++)if(bb[i] != rhs.bb[i])return false;
		return true;
	}
	BitBoard operator~()const{
		BitBoard ret;
		for(int i=0;i<bb.size();i++)ret.bb[i] = ~bb[i];
		return ret;
	}

	void clear(){
		for(int i=0;i<bb.size();i++)bb[i] = 0;
	}
	void for_each(std::function<void(Intersection i)> proce)const{
		for(int x=0;x<bb.size();x++){
			uint64_t mask = bb[x];
			while(mask){
				proce(x * 64 + __builtin_ctzll(mask));
				mask &= mask - 1;
			}
		}
	}

};

extern sheena::Array<BitBoard, BoardDim> intersection_bb;

extern void init_bb_table();