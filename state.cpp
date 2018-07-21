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

bool State::terminate(sheena::Array<double, 2>& reward, size_t thread_id)const{
	if(super_kou != Empty){
		int r = super_kou == Black ? -1 : 1;
		reward[0] = r;
		reward[1] = -r;
		return true;
	}
	if(move_history[0] == pass
	&& move_history[1] == pass){
		int r = pos.result(searcher->komix2, searcher->mc_owner[thread_id]);
		reward[0] = r;
		reward[1] = -r;
		return true;
	}
	return false;
}
void State::playout(sheena::Array<double, 2>& result, size_t thread_id){
	int limit = std::min(BoardDim, pos.get_board_size() * pos.get_board_size() * 2);
	sheena::Array<Intersection, BoardDim> history;
	Intersection last = move_history[game_ply % 2];
	if(last < 0)last = pass;
	history[0] = last;
	Stone start_player = turn();
	for(int ply = 0; ply < limit; ply++){
		if(terminate(result, thread_id)){
			int r = result[0] > 0? 1 : (result[0] == 0? 0 : -1);
			searcher->lgrf[thread_id].update(start_player, r, history, ply);
			return;
		}
		//LGRF
		Intersection i = searcher->lgrf[thread_id].probe(turn(), last);
		if(i == pass || !pos.is_empty(i) || !is_move_legal(turn(), i)){
			i = random_move(searcher->mt[thread_id]);
		} 
		history[ply + 1] = i;
		last = i;
		act(i, thread_id);
	}
	//終局しない場合は引き分けとする
	result[0] = result[1] = 0;
}

int State::get_actions(int& n_moves, MoveArray& moves, sheena::Array<float, MaxLegalMove>& scores, size_t thread_id)const{
	//合法手生成
	n_moves = pos.generate_moves(moves);
	//着手のスコア付け
	if(turn() == Black)for(int i=0;i<n_moves;i++)scores[i] = 0.6 - std::abs(searcher->mc_owner[thread_id][moves[i]] - 0.5);
	//進行度が0.5以上ならパスを追加
	if(pos.progress() >= 0.5){
		moves[n_moves] = pass;
		scores[n_moves++] = 0.01;
	}
	return pos.turn_player() - 1;
}
void Searcher::set_random(){
	std::random_device rd;
	for(auto& rand : mt){
		rand.seed(rd());
	}
}