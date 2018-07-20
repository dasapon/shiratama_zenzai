#include "policy.hpp"
#include "dir4pattern.hpp"
#include "position.hpp"

static sheena::Array<std::bitset<BoardDim>, MaxBoardSize + 1> is_edge;
//relation
//25
//146
//0345
//X012

static sheena::Array2d<int, BoardDim, BoardDim> relation_table;

struct PolicyWeight{
	sheena::Array3d<float, diag4dim, dir4dim, 2> pattern;
	sheena::Array<float, 8> relation;
};

static PolicyWeight policy_weight;
static void clear_policy(){

	for(int i=0;i < diag4dim ;i++){
		for(int j=0;j< dir4dim;j++){
			policy_weight.pattern[i][j][0] = 1;
			policy_weight.pattern[i][j][1] = 1;
		}
	}
	for(int i=0;i<8;i++)policy_weight.relation[i] = 1;
}
void init_policy(){
	//盤端からの距離の初期化
	for(int board_size = 1;board_size <= MaxBoardSize; board_size++){
		for(int y=1;y<=board_size;y++){
			for(int x=1;x<=board_size;x++){
				int distance = std::min(std::min(y - 1, board_size - y), std::min(x - 1, board_size - x));
				is_edge[board_size][intersection(x, y)] = distance < 2;
			}
		}
	}
	//2点間の関係
	for(Intersection i = 0; i < BoardDim; i++){
		for(Intersection j = 0; j < BoardDim; j++){
			int dx = std::abs(i % BoardWidth - j % BoardWidth);
			int dy = std::abs(i / BoardWidth - j / BoardWidth);
			int min = std::min(dx, dy);
			int max = std::max(dx, dy);
			if(max > 3 || min >= 3)relation_table[i][j] = 7;
			else if(min == 0)relation_table[i][j] = max - 1;
			else if(min == 1)relation_table[i][j] = max + 2;
			else if(min == 2)relation_table[i][j] = max + 4;
		}
	}
	//policyの重みの読み込み
	FILE* fp = fopen("policy.bin", "rb");
	if(fp == nullptr){
		std::cerr << "policy param can't be opened" << std::endl;
		clear_policy();
		return;
	}
	bool ok = fread(&policy_weight, sizeof(PolicyWeight), 1, fp) == 1;
	if(!ok){
		std::cerr << "policy param can't be loaded" << std::endl;
	}
}

float pattern_policy(int board_size, Intersection i, int diag4pattern, int dir4pattern){
	return policy_weight.pattern[diag4pattern][dir4pattern][is_edge[board_size][i]];
}

float relation_policy(Intersection lastmove, Intersection move){
	return policy_weight.relation[relation_table[lastmove][move]];
}


void learn_policy(const std::vector<Record>& records){
	clear_policy();
	PolicyWeight denom;
	PolicyWeight numerator;
	for(int epoch = 0; epoch < 16; epoch++){
		for(int i=0;i<diag4dim;i++){
			for(int j=0;j<dir4dim;j++){
				for(int k=0;k < 2;k++){
					numerator.pattern[i][j][k] = 1;
					denom.pattern[i][j][k] = 2.0 / (1 + policy_weight.pattern[i][j][k]);
				}
			}
		}
		for(int i=0;i<8;i++){
			numerator.relation[i] = 1;
				denom.relation[i] = 2.0 / (1 + policy_weight.relation[i]);
		}
		double loss = 0;
		int cnt = 0;
		for(const Record& record : records){
			//9路限定
			Position pos(9);
			const std::bitset<BoardDim>& is_edge9 = is_edge[9];
			for(int ply = 0;ply<record.size();ply++){
				if(record[ply] != pass){
					//合法手生成
					MoveArray moves;
					sheena::Array<float, 362> policy_score;
					int n_moves = pos.generate_moves(moves, policy_score);
					float sum = 0;
					float best_score = 0;
					Intersection lastmove = ply == 0? pass : record[ply - 1];
					for(int i=1;i<n_moves;i++){
						if(lastmove != pass)policy_score[i] *= relation_policy(lastmove, moves[i]);
						sum += policy_score[i];
						if(moves[i] == record[ply])best_score = policy_score[i];
					}
					loss += -std::log(best_score / sum);
					cnt++;
					//分子の更新
					numerator.pattern[pos.diag4index(record[ply])][pos.dir4index(record[ply])][is_edge9[record[ply]]] += 1;
					if(lastmove != pass)numerator.relation[relation_table[lastmove][record[ply]]] += 1;
					//分母の更新
					for(int i=1;i<n_moves;i++){
						denom.pattern[pos.diag4index(moves[i])][pos.dir4index(moves[i])][is_edge9[moves[i]]] += 1.0 / sum;
						if(lastmove != pass)denom.relation[relation_table[lastmove][moves[i]]] += 1.0 / sum;
					}
				}
				//1手進める
				pos.make_move(record[ply]);
			}
		}
		//パラメータの更新
		if(epoch % 2 == 1){
			for(int i=0;i<diag4dim;i++){
				for(int j=0;j<dir4dim;j++){
					for(int k=0;k<2;k++)policy_weight.pattern[i][j][k] = numerator.pattern[i][j][k] / denom.pattern[i][j][k];
				}
			}
		}
		else{
			for(int i=0;i<8;i++){
				policy_weight.relation[i] = numerator.relation[i] / denom.relation[i];
			}
		}
		//損失の表示
		std::cout << "loss " << loss / cnt << std::endl;
	}
	//パラメータ保存
	FILE* fp = fopen("policy.bin", "wb");
	fwrite(&policy_weight, sizeof(PolicyWeight), 1, fp);
}
