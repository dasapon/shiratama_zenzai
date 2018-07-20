#pragma once

#include <random>
#include <functional>
#include <cstdint>
#include <map>
#include <iostream>
#include <bitset>

#include <omp.h>

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

enum{
	SW = South + West,
	SE = South + East,
	NW = North + West,
	NE = North + East,
};
constexpr int diag4dim = 3 * 3 * 3 * 3;

inline Intersection intersection(int x, int y){
	return x + y * BoardWidth;
}
extern std::string intersection2string(Intersection i);
extern Intersection string2intersection(std::string str);

struct Record : public std::vector<Intersection>{
	int result;
};

extern std::vector<Record> load_records(std::string filename);

extern void store_records(std::string filename, const std::vector<Record>& records);

extern void generate_records();