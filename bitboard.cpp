#include "bitboard.hpp"

sheena::Array<BitBoard, BoardDim> intersection_bb;

void init_bb_table(){
	for(Intersection i=0;i<BoardDim;i++){
		intersection_bb[i].clear();
		intersection_bb[i] |= i;
	}
}