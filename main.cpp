#include "position.hpp"
#include "dir4pattern.hpp"

extern void gtp();

int main(int argc, char* argv[]){
	init_bb_table();
	Position::init_hash_seed();
	init_dir4_table();
	gtp();
	return 0;
}