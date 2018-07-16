#include "position.hpp"

static const sheena::Array<Intersection, 4> dir4({East, West, North, South});

sheena::Array2d<uint64_t, 441, 4> Position::hash_seed;

void Position::init_hash_seed(){
	std::mt19937_64 mt(0);
	for(int i=0;i<hash_seed.size();i++){
		hash_seed[i][Black] = mt() & ~1ULL;
		hash_seed[i][White] = mt() & ~1ULL;
	}
}

Position::Position(int board_size): stones(Sentinel), key(0), board_size(board_size), kou(0), turn(Black){
	liverty_cache.clear();
	for(int y = 1; y<=board_size;y++){
		for(int x = 1; x<= board_size;x++){
			stones[intersection(x, y)] = Empty;
		}
	}
}

Position::Position(const Position& pos){
	operator=(pos);
}

void Position::operator=(const Position& rhs){
	stones = rhs.stones;
	key = rhs.key;
	board_size = rhs.board_size;
	kou = rhs.kou;
	turn = rhs.turn;
	liverty_cache = rhs.liverty_cache;
}

void Position::put(Stone color, Intersection i){
	stones[i] = color;
	key ^= hash_seed[i][color];
}
void Position::remove(Stone color, Intersection i){
	stones[i] = Empty;
	key ^= hash_seed[i][color];
}
int Position::remove_string(Stone color, Intersection i, BitBoard& liverty_changed){
	assert(stones[i] == color);
	int ret = 1;
	remove(color, i);
	for(Intersection dir : dir4){
		Stone st = (*this)[i + dir];
		if(st == color){
			ret += remove_string(color, i + dir, liverty_changed);
		}
		else if(st == opponent(color)){
			liverty_changed |= i + dir;
		}
	}
	return ret;
}

int Position::update_liverty_sub(Stone color, Intersection i, BitBoard& done, BitBoard& bb)const{
	assert(stones[i] == color);
	bb |= i;
	int ret = 0;
	for(Intersection dir : dir4){
		Intersection neighbor = i + dir;
		if(done[neighbor])continue;
		done |= neighbor;
		Stone stone = (*this)[neighbor];
		if(stone == Empty){
			ret++;
		}
		else if(stone == color){
			ret += update_liverty_sub(color, neighbor, done, bb);
		}
	}
	return ret;
}
BitBoard Position::update_liverty(Intersection i){
	assert(stones[i] == Black || stones[i] == White);
	BitBoard done(intersection_bb[i]), bb;
	bb.clear();
	int liverty = update_liverty_sub((*this)[i], i, done, bb);
	//liverty_cacheの更新
	bb.for_each([&](Intersection i){
		liverty_cache[i] = liverty;
	});
	return bb;
}

int Position::result(int komix2)const{
	int territory_diff = 0;
	for(int y = 1; y<=board_size;y++){
		for(int x = 1; x<= board_size;x++){
			Intersection i = intersection(x, y);
			Stone stone = (*this)[i];
			switch(stone){
			case Empty:
				{
					sheena::VInt<StoneDim> surround;
					surround.clear();
					for(Intersection dir : dir4){
						surround[(*this)[i + dir]] += 1;
					}
					if(surround[Black]){
						if(surround[White] == 0)territory_diff++;
					}
					else if(surround[White]){
						territory_diff--;
					}
				}
				break;
			case Black:
				territory_diff++;
				break;
			case White:
				territory_diff--;
				break;
			default:
				assert(false);
			}
		}
	}
	territory_diff *= 2;
	territory_diff -= komix2;
	if(territory_diff > 0)return 1;
	else if(territory_diff == 0)return 0;
	else return -1;
}

void Position::clear(){
	liverty_cache.clear();
	stones = Sentinel;
	key = 0;
	kou = -1;
	turn = Black;
	for(int y = 1; y<=board_size;y++){
		for(int x = 1; x<= board_size;x++){
			stones[intersection(x, y)] = Empty;
		}
	}
}

void Position::make_move(Intersection i){
	if(i == pass){
		//手番入れ替え
		turn = opponent(turn);
		key ^= 1;
		return;
	}
	assert(stones[i] == Empty);
	Stone enemy = opponent(turn);
	BitBoard liverty_changed(intersection_bb[i]);
	int captured = 0;
	int not_enemy = 0;
	//周辺の石についての処理
	for(Intersection dir : dir4){
		Intersection neighbor = i + dir;
		if(stones[neighbor] == enemy){
			if(liverty_cache[neighbor] == 1){
				//石を取る
				captured += remove_string(enemy, neighbor, liverty_changed);
			}
			else{
				liverty_changed |= neighbor;
			}
		}
		else{
			not_enemy++;
			if(stones[neighbor] == turn){
				liverty_changed |= neighbor;
			}
		}
	}
	//石を置く
	put(turn, i);
	//コウの更新
	if(captured == 1 && not_enemy == 0){
		kou = i;
	}
	else{
		kou = 0;
	}
	//呼吸点の更新
	BitBoard updated;
	updated.clear();
	liverty_changed.for_each([&](Intersection i){
		if(!updated[i]){
			updated |= update_liverty(i);
		}
	});
	//手番入れ替え
	turn = opponent(turn);
	key ^= 1;
}

int Position::generate_moves(MoveArray& moves)const{
	const Stone enemy = opponent(turn);
	int ret = 0;
	moves[ret++] = pass;
	for(int y = 1; y <= board_size; y++){ for(int x = 1; x <= board_size; x++){
		Intersection i = intersection(x, y);
		if(stones[i] == Empty){
			bool valid = false, point_break = true;
			//合法手である条件
			//コウを即座に取り返す手でない
			//打った石の呼吸点が0に成らない
			//自分の目を潰す手で無い(呼吸点2以上の味方の石)でないものが周辺に存在する
			for(Intersection dir : dir4){
				Intersection neighbor = i + dir;
				//コウを即座に取り返す手
				if(neighbor == kou){
					valid = false;
					break;
				}
				if(stones[neighbor] == turn){
					if(liverty_cache[neighbor] != 1){
						valid = true;
					}
					else{
						point_break = false;
					}
				}
				else{
					if(stones[neighbor] != Sentinel)point_break = false;
					if(stones[neighbor] == enemy){
						//石を取る手
						if(liverty_cache[neighbor] < 2){
							valid = true;
						}
					}
					else{
						//空点が存在
						valid = true;
					}
				}
			}
			if(valid && !point_break){
				moves[ret++] = i;
			}
		}
	}}
	return ret;
}