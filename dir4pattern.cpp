#include "dir4pattern.hpp"

std::bitset<dir4dim> is_legal;

void init_dir4_table(){
	for(int pattern = 0;pattern < dir4dim;pattern++){
		bool suicide = true;
		bool point_break = true;
		for(int i=0;i<4;i++){
			int pat = (pattern >> (i * 3)) & 7;
			switch(pat){
			case 0://空
				suicide = false;
				point_break = false;
				break;
			case 1://壁
				break;
			case 2://呼吸点1の味方
				point_break = false;
				break;
			case 3:
			case 4://呼吸点2以上の味方
				suicide = false;
				break;
			case 5://取れる石
				suicide = false;
				point_break = false;
				break;
			case 6:
			case 7://取れない相手の石
				point_break = false;
				break;
			}
		}
		is_legal[pattern] = !suicide && !point_break;
	}
}

int update_pattern(Stone turn, int old, Stone stone, int liverty){
	old *= 8;
	switch(stone){
	case Empty:
		break;
	case Black:
	case White:
		if(stone == turn){
			old += 2;
		}
		else{
			old += 5;
		}
		old += std::min(liverty - 1, 2);
		break;
	case Sentinel:
		old += 1;
		break;
	default:
		assert(false);
	}
	return old;
}