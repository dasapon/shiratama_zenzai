#pragma once

#include "position.hpp"

class Searcher;

class State{
public:
	Position pos;
	sheena::Array<uint64_t, 16> key_history;
	sheena::Array<Intersection, 2> move_history;
	Searcher* searcher;
	int game_ply;
	Stone super_kou;
	Intersection random_move(std::mt19937& mt)const;
	State(Searcher& searcher, int board_size):pos(board_size), searcher(&searcher), game_ply(0), super_kou(Empty){
		for(int i=0;i<16;i++){
			key_history[i] = 0;
		}
		move_history[0] = move_history[1] = -1;
	}
	int board_size()const{
		return pos.get_board_size();
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
	bool terminate(sheena::Array<double, 2>& reward, size_t thread_id)const;
	void clear(){
		pos.clear();
		game_ply = 0;
		super_kou = Empty;
		for(int i=0;i<16;i++){
			key_history[i] = 0;
		}
		move_history[0] = move_history[1] = -1;
	}
	void act(Intersection i, size_t thread_id){
		pos.make_move(i);
		game_ply++;
		move_history[game_ply % 2] = i;
		//super kou判定
		if(super_kou == Empty && i != pass){
			for(int i=game_ply % 2;i<key_history.size();i+=2){
				if(key() == key_history[i]){
					super_kou = opponent(turn());
					break;
				}
			}
		}
		key_history[game_ply % 16] = key();
	}
	uint64_t key()const{
		return pos.get_key();
	}
	Stone turn()const{
		return pos.turn_player();
	}
	Intersection lastmove()const{
		return move_history[game_ply % 2];
	}
	void playout(sheena::Array<double, 2>&, size_t thread_id);
	int get_actions(int&, MoveArray&, sheena::Array<float, MaxLegalMove>&, size_t thread_id)const;
	float progress()const{
		return pos.progress();
	}
};
constexpr size_t max_threads = 16;
class Searcher : public sheena::mcts::Searcher<sheena::mcts::UCB1, State, Intersection, 2, MaxLegalMove>{
	friend class State;
	int komix2;
	sheena::Array<std::mt19937, max_threads> mt;
	sheena::Array<MonteCarloOwner, max_threads> mc_owner;
public:
	
	Searcher():komix2(0){
		int i = 0;
		for(auto& rand : mt){
			mc_owner[i].init();
			rand.seed(i++);
		}
	}
	void set_komi(double komi){
		komix2 = komi * 2;
	}
	void set_random();
	template<bool print>
	Intersection bestmove(const State& state){
		MoveArray moves;
		sheena::Array<double, MaxLegalMove>rewards;
		sheena::Array<int, MaxLegalMove> counts;
		int n_moves = search_result(state, moves, rewards, counts);
		Intersection ret = pass;
		int max_count = 0;
		double max_reward = -2;
		for(int i=0;i<n_moves;i++){
			if(print && counts[i] >= 500)std::cerr << intersection2string(moves[i]) << "," << rewards[i] << "," << counts[i] << std::endl;
			max_reward = std::max(max_reward, rewards[i]);
			if(max_count < counts[i]){
				ret = moves[i];
				max_count = counts[i];
			}
		}
		if(max_reward <= -0.8)ret = resign;
		return ret;
	}
	Intersection select(const State& state, int threshold){
		MoveArray moves;
		sheena::Array<double, MaxLegalMove>rewards;
		sheena::Array<int, MaxLegalMove> counts;
		int n_moves = search_result(state, moves, rewards, counts);
		int count_sum = 0;
		for(int i=0;i<n_moves;i++){
			count_sum += counts[i];
		}
		if(count_sum <= threshold)return resign;
		std::uniform_int_distribution<int> dist(0, count_sum - 1);
		int r = dist(mt[0]);
		for(int i=0;i<n_moves;i++){
			r -= counts[i];
			if(r < 0)return moves[i];
		}
		return pass;
	}
};