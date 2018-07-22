#include "position.hpp"
#include "dir4pattern.hpp"

static const sheena::Array<Intersection, 4> dir4({East, West, North, South});

sheena::Array2d<uint64_t, BoardWidth * BoardWidth, 4> Position::hash_seed;

void Position::init_hash_seed(){
	std::mt19937_64 mt(0);
	for(int i=0;i<hash_seed.size();i++){
		hash_seed[i][Empty] = mt() & ~1ULL;//コウ
		hash_seed[i][Black] = mt() & ~1ULL;
		hash_seed[i][White] = mt() & ~1ULL;
	}
}

Position::Position(int board_size): stones(Sentinel), key(0), board_size(board_size), kou(0), turn(Black), n_stone(0){
	occupied[Empty].clear();
	occupied[Black].clear();
	occupied[White].clear();
	liverty_cache.clear();
	for(int y = 1; y<=board_size;y++){
		for(int x = 1; x<= board_size;x++){
			Intersection i = intersection(x, y);
			stones[i] = Empty;
			occupied[Empty] ^= i;
		}
	}
}

Position::Position(const Position& pos){
	operator=(pos);
}

void Position::operator=(const Position& rhs){
	stones = rhs.stones;
	occupied = rhs.occupied;
	key = rhs.key;
	board_size = rhs.board_size;
	kou = rhs.kou;
	turn = rhs.turn;
	n_stone = rhs.n_stone;
	liverty_cache = rhs.liverty_cache;
}

void Position::put(Stone color, Intersection i){
	stones[i] = color;
	key ^= hash_seed[i][color];
	occupied[Empty] ^= i;
	occupied[color] ^= i;
	n_stone++;
}
void Position::remove(Stone color, Intersection i){
	stones[i] = Empty;
	key ^= hash_seed[i][color];
	occupied[Empty] ^= i;
	occupied[color] ^= i;
	n_stone--;
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

//空点がどちらの地かを判定
int Position::result_sub(Intersection i, BitBoard& done, Stone& color)const {
	assert(stones[i] == Empty);
	done |= i;
	int ret = 1;
	for(Intersection dir : dir4){
		if(stones[i + dir] == Sentinel)continue;
		color = static_cast<Stone>(color | stones[i + dir]);
		if(stones[i + dir] == Empty){
			if(!done[i + dir]){
				ret += result_sub(i + dir, done, color);
			}
		}
	}
	return ret;
}

int Position::result(int komix2, MonteCarloOwner& mc_owner)const{
	int territory_diff = 0;
	BitBoard counted_empties;
	counted_empties.clear();
	for(int y = 1; y<=board_size;y++){
		for(int x = 1; x<= board_size;x++){
			Intersection i = intersection(x, y);
			Stone stone = (*this)[i];
			switch(stone){
			case Empty:
				if(!counted_empties[i]){
					Stone surround = Empty;
					BitBoard bb(counted_empties);
					int n = result_sub(i, counted_empties, surround);
					int sign = 0;
					switch(surround){
					case Black:
						territory_diff += n;
						sign = 1;
						break;
					case White:
						territory_diff -= n;
						sign = -1;
						break;
					default:
						break;
					}
					if(sign){
						bb = counted_empties & ~bb;
						bb.for_each([&](Intersection i){
							mc_owner.update(i, sign);
						});
					}
				}
				break;
			case Black:
				territory_diff++;
				mc_owner.update(i, 1);
				break;
			case White:
				territory_diff--;
				mc_owner.update(i, -1);
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
		else if(stones[neighbor] == Empty){
			not_enemy++;
		}
		else if(stones[neighbor] == turn){
			liverty_changed |= neighbor;
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
	int ret = 0;
	occupied[Empty].for_each([&](Intersection i){
		//序盤であれば、1線、2線に打たない
		if(board_size == 19 && n_stone < 20){
			int x = i % BoardWidth;
			int y = i / BoardWidth;
			if(x <= 2 || x >= 18 || y <= 2 || y >= 18)return;
		}
		bool invalid = false;
		int dir4pattern = 0;
		//合法手である条件
		//コウを即座に取り返す手でない
		//打った石の呼吸点が0に成らない
		//自分の目を潰す手で無い(呼吸点2以上の味方の石)でないものが周辺に存在する
		for(Intersection dir : dir4){
			Intersection neighbor = i + dir;
			//コウを即座に取り返す手
			if(neighbor == kou){
				invalid = true;
				break;
			}
			dir4pattern = update_pattern(turn, dir4pattern, static_cast<Stone>(stones[neighbor]), liverty_cache[neighbor]);
		}
		if(!invalid && is_legal.test(dir4pattern)){
			moves[ret++] = i;
		}
	});
	return ret;
}

int Position::generate_moves_fast(MoveArray& moves)const{
	int ret = 0;
	BitBoard kou_inv = intersection_bb[kou];
	occupied[Empty].for_each([&](Intersection i){
		moves[ret++] = i;
	});
	return ret;
}

//合法手判定
//ただし、iは空であることは保証されているとする
bool Position::is_move_legal(Intersection i)const{
	int dir4index = 0;
	for(Intersection dir : dir4){
		Intersection neighbor = i + dir;
		if(neighbor == kou)return false;
		dir4index = update_pattern(turn, dir4index, static_cast<Stone>(stones[neighbor]), liverty_cache[neighbor]);
	}
	return is_legal.test(dir4index);
}
