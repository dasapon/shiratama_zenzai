#pragma once

#include "shiratama_zenzai.hpp"

constexpr int dir4dim = 4096;
extern std::bitset<dir4dim> is_legal;

extern void init_dir4_table();

extern int update_pattern(Stone turn, int old, Stone stone, int liverty);