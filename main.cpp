#include "position.hpp"

extern void gtp();

int main(void){
	init_bb_table();
	Position::init_hash_seed();
	gtp();
	return 0;
}