#pragma once

#include "shiratama_zenzai.hpp"

class MonteCarloOwner : public sheena::Array<float, BoardDim>{
public:
	void init(){
		for(int i=0;i<BoardDim;i++)operator[](i) = 0.5;
	}
	void update(Intersection i, int sign){
		operator[](i) = operator[](i) * 0.95 + sign * 0.05;
	}
};