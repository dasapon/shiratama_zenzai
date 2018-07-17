#include "state.hpp"

Intersection State::random_move(std::mt19937& mt)const{
	MoveArray moves;
	//合法手生成
	int n_moves = pos.generate_moves(moves);
	//他の合法手があればパスは選択しない
	//パス以外の合法手しか無いならばパスをする
	if(n_moves == 1)return pass;
	std::uniform_int_distribution<int> dist(1, n_moves - 1);
	return moves[dist(mt)];
}

void State::playout(sheena::Array<double, 2>& result, size_t thread_id){
	int limit = pos.get_board_size() * pos.get_board_size() * 2;
	for(int ply = 0; ply < limit; ply++){
		if(super_kou){
			int r = turn() == Black ? -1 : 1;
			result[0] = r;
			result[1] = -r;
			return;
		}
		Intersection i = random_move(searcher->mt[thread_id]);
		act(i);
		if(move_history[0] == pass
		&& move_history[1] == pass){
			int r = pos.result(searcher->komix2);
			result[0] = r;
			result[1] = -r;
			return;
		}
	}
	//終局しない場合は引き分けとする
	result[0] = result[1] = 0;
}

int State::get_actions(int& n_moves, MoveArray& moves, sheena::Array<float, 362>& probabilities, size_t thread_id)const{
	//連続パス,または同型反復による終局判定
	if((move_history[0] == pass
	&& move_history[1] == pass)
	|| super_kou){
		n_moves = 0;
		return pos.turn_player() - 1;
	}
	//非合法手
	n_moves = pos.generate_moves(moves);
	for(int i=0;i<n_moves;i++)probabilities[i] = 1.0 / n_moves;
	return pos.turn_player() - 1;
}