#include "position.hpp"
#include "policy.hpp"
#include "dir4pattern.hpp"

extern void gtp();

int main(int argc, char* argv[]){
	init_bb_table();
	init_policy();
	Position::init_hash_seed();
	init_dir4_table();
	if(argc >= 2){
		if(std::string(argv[1]) == "policy"){
			std::vector<Record> records = load_records("selfplay.go");
			learn_policy(records);
		}
		else if(std::string(argv[1]) == "selfplay"){
			generate_records();
		}
	}
	gtp();
	return 0;
}