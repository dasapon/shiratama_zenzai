#include "state.hpp"

static int binary_search(int start, int end, float f, sheena::Array<float, 362>& policy){
	if(end - start == 1)return start;
	int mid = (start + end) / 2;
	if(f < policy[mid])return binary_search(start, mid, f, policy);
	else return binary_search(mid, end, f, policy);
}

Intersection State::random_move(std::mt19937& mt)const{
	MoveArray moves;
	sheena::Array<float, 362> policy;
	//合法手生成
	int n_moves = pos.generate_moves(moves, policy);
	//他の合法手があればパスは選択しない
	//パス以外の合法手しか無いならばパスをする
	if(n_moves == 1)return pass;
	for(int i=2;i<n_moves;i++){
		policy[i] += policy[i - 1];
	}
	std::uniform_real_distribution<float> dist(0, policy[n_moves - 1]);
	float p = dist(mt);
	//2分探索
	return moves[binary_search(1, n_moves, p, policy)];
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

int State::get_actions(int& n_moves, MoveArray& moves, sheena::Array<float, 362>& probabilities, size_t thread_id)const{
	//連続パス,または同型反復による終局判定
	if((move_history[0] == pass
	&& move_history[1] == pass)
	|| super_kou != Empty){
		n_moves = 0;
		return pos.turn_player() - 1;
	}
	//非合法手
	n_moves = pos.generate_moves(moves, probabilities);
	float sum = 0;
	for(int i=0;i<n_moves;i++)sum += probabilities[i];
	for(int i=0;i<n_moves;i++)probabilities[i] /= sum;
	return pos.turn_player() - 1;
}
void Searcher::set_random(){
	std::random_device rd;
	for(auto& rand : mt){
		rand.seed(rd());
	}
}