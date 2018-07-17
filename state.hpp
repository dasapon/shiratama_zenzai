#pragma once

#include "position.hpp"

class Searcher;

class State{
	Position pos;
	sheena::Array<uint64_t, 16> key_history;
	sheena::Array<Intersection, 2> move_history;
	Searcher* searcher;
	int game_ply;
	bool super_kou;
	Intersection random_move(std::mt19937& mt)const;
public:
	State(Searcher& searcher, int board_size):pos(board_size), searcher(&searcher), game_ply(0), super_kou(false){
		for(int i=0;i<16;i++){
			key_history[i] = 0;
		}
		move_history[0] = move_history[1] = -1;
	}
	State(const State& state):pos(state.pos), key_history(state.key_history), move_history(state.move_history), searcher(state.searcher), game_ply(state.game_ply), super_kou(state.super_kou){}
	void operator=(const State& rhs){
		pos = rhs.pos;
		key_history = rhs.key_history;
		move_history = rhs.move_history;
		searcher = rhs.searcher;
		game_ply = rhs.game_ply;
		super_kou = rhs.super_kou;
	}
	void clear(){
		pos.clear();
		game_ply = 0;
		super_kou = false;
		for(int i=0;i<16;i++){
			key_history[i] = 0;
		}
		move_history[0] = move_history[1] = -1;
	}
	void act(Intersection i){
		pos.make_move(i);
		game_ply++;
		//super kou判定
		for(int i=0;i<key_history.size();i++){
			if(key() == key_history[i]){
				super_kou = true;
				break;
			}
		}
		key_history[game_ply % 16] = key();
		move_history[game_ply & 1] = i;
	}
	uint64_t key()const{
		return pos.get_key();
	}
	Stone turn()const{
		return pos.turn_player();
	}
	void playout(sheena::Array<double, 2>&, size_t thread_id);
	int get_actions(int&, sheena::Array<Intersection, 362>&, sheena::Array<float, 362>&, size_t thread_id)const;
	bool is_move_legal(Stone turn, Intersection i){
		if(turn != pos.turn_player())return false;
		MoveArray moves;
		int n = pos.generate_moves(moves);
		for(int i=0;i<n;i++){
			if(moves[i] == i)return true;
		}
		return false;
	}
};
constexpr size_t max_threads = 16;
class Searcher : public sheena::mcts::Searcher<sheena::mcts::UCB1, State, Intersection, 2, 362>{
	friend class State;
	int komix2;
	sheena::Array<std::mt19937, max_threads> mt;
public:
	Searcher():komix2(0){
		int i = 0;
		for(auto& rand : mt){
			rand.seed(i++);
		}
	}
	void set_komi(double komi){
		komix2 = komi * 2;
	}
	Intersection bestmove(const State& state){
		std::cerr <<"select" << std::endl;std::cerr.flush();
		MoveArray moves;
		sheena::Array<double, 362>rewards;
		sheena::Array<int, 362> counts;
		int n_moves = search_result(state, moves, rewards, counts);
		Intersection ret = resign;
		int max_count = 0;
		for(int i=0;i<n_moves;i++){
			std::cerr << intersection2string(moves[i]) << "," << rewards[i] << "," << counts[i] << std::endl;
			if(rewards[i] > -0.8){
				if(max_count < counts[i]){
					ret = moves[i];
					max_count = counts[i];
				}
			}
		}
		return ret;
	}
};