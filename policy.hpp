#pragma once

#include "shiratama_zenzai.hpp"

extern void init_policy();

extern float policy(int board_size, Intersection i, int dir4pattern);

extern void learn_policy(const std::vector<Record>& records);