#pragma once
#include "shiratama_zenzai.hpp"
#include "position.hpp"

class LGRF : private sheena::Array2d<Intersection, BoardDim, 2>{
public:
	void clear(){
		for(int i=0;i<BoardDim;i++){
			(*this)[i][0] = pass;
			(*this)[i][1] = pass;
		}
	}
	void update(Stone start_turn, int result, const sheena::Array<Intersection, BoardDim>& moves, int ply){
		if(result == 0)return;
		int start = start_turn == Black ? 0 : 1;
		//黒勝ち
		if(result > 0){
			for(int i=0;i < ply; i+=2){
				(*this)[moves[i]][1 - start] = pass;
			}
			for(int i=1;i < ply;  i+=2){
				(*this)[moves[i]][start] = moves[i + 1];
			}
		}
		//白勝ち
		else{
			for(int i=0;i < ply; i+=2){
				(*this)[moves[i]][1 - start] = moves[i + 1];
			}
			for(int i=1;i < ply;  i+=2){
				(*this)[moves[i]][start] = pass;
			}
		}
	}
	Intersection probe(Stone turn, Intersection last)const{
		if(turn == Black)return (*this)[last][0];
		else return (*this)[last][1];
	}
};