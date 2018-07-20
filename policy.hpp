#pragma once

#include "shiratama_zenzai.hpp"

extern void init_policy();

extern float pattern_policy(int board_size, Intersection i, int diag4pattern, int dir4pattern);

extern float relation_policy(Intersection last, Intersection i);

extern void learn_policy(const std::vector<Record>& records);