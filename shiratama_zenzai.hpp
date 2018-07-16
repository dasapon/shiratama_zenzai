#pragma once

#include <random>
#include <functional>
#include <cstdint>
#include <map>
#include <iostream>

#include "sheena/sheena.hpp"

enum Stone{
	Empty,
	Black,
	White,
	Sentinel,
	StoneDim,
};

inline Stone opponent(Stone stone){
	assert(stone == Black || stone == White);
	return static_cast<Stone>(3 - stone);
}

constexpr int BoardWidth = 20;
constexpr int MaxBoardSize = 19;
constexpr int BoardDim = 64 * 8;
using Intersection = int;

enum{
	South = BoardWidth,
	North = -BoardWidth,
	West = -1,
	East = 1,
};

inline Intersection intersection(int x, int y){
	return x + y * BoardWidth;
}
extern std::string intersection2string(Intersection i);