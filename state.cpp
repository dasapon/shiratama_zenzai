#include "state.hpp"
#include "dir4pattern.hpp"

Intersection State::random_move(std::mt19937& mt)const{
	MoveArray moves;
	//合法手生成
	int n_moves = pos.generate_moves_fast(moves);
	//他の合法手があればパスは選択しない
	//パス以外の合法手しか無いならばパスをする
	while(n_moves){
		int idx = mt() % n_moves;
		Intersection move = moves[idx];
		if(pos.is_move_legal(move))return move;
		else moves[idx] = moves[--n_moves];
	}
	return pass;
}

bool State::terminate(sheena::Array<double, 2>& reward)const{
	if(super_kou != Empty){
		int r = super_kou == Black ? -1 : 1;
		reward[0] = r;
		reward[1] = -r;
		return true;
	}
	if(move_history[0] == pass
	&& move_history[1] == pass){
		int r = pos.result(searcher->komix2);
		reward[0] = r;
		reward[1] = -r;
		return true;
	}
	return false;
}
void State::playout(sheena::Array<double, 2>& result, size_t thread_id){
	int limit = pos.get_board_size() * pos.get_board_size() * 2;
	for(int ply = 0; ply < limit; ply++){
		if(terminate(result))return;
		Intersection i = random_move(searcher->mt[thread_id]);
		act(i);
	}
	//終局しない場合は引き分けとする
	result[0] = result[1] = 0;
}

int State::get_actions(int& n_moves, MoveArray& moves, sheena::Array<float, MaxLegalMove>& probabilities, size_t thread_id)const{
	//合法手生成
	n_moves = pos.generate_moves(moves);
	//進行度が0.5以上ならパスを追加
	if(pos.progress() >= 0.5){
		moves[n_moves++] = pass;
	}
	float p = 1.0f / n_moves;
	for(int i=0;i<n_moves;i++)probabilities[i] = p;
	return pos.turn_player() - 1;
}
void Searcher::set_random(){
	std::random_device rd;
	for(auto& rand : mt){
		rand.seed(rd());
	}
}