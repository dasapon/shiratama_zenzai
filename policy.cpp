#include "policy.hpp"
#include "dir4pattern.hpp"
#include "position.hpp"

static sheena::Array2d<int, MaxBoardSize + 1, BoardDim> edge_distance_max3;

using PolicyWeight = sheena::Array2d<float, 4, dir4dim>;
static PolicyWeight policy_weight;
static void clear_policy(){
	for(int i=0;i<policy_weight.size();i++){
		for(int j=0;j<policy_weight[i].size();j++)policy_weight[i][j] = 1;
	}
}
void init_policy(){
	//盤端からの距離の初期化
	for(int board_size = 1;board_size <= MaxBoardSize; board_size++){
		for(int y=1;y<=board_size;y++){
			for(int x=1;x<=board_size;x++){
				int distance = std::min(std::min(y - 1, board_size - y), std::min(x - 1, board_size - x));
				edge_distance_max3[board_size][intersection(x, y)] = std::min(3, distance);
			}
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

float policy(int board_size, Intersection i, int dir4pattern){
	return policy_weight[edge_distance_max3[board_size][i]][dir4pattern];
}

void learn_policy(const std::vector<Record>& records){
	clear_policy();
	PolicyWeight denom;
	PolicyWeight numerator;
	for(int epoch = 0; epoch < 4; epoch++){
		for(int i=0;i<denom.size();i++){
			for(int j=0;j<denom[i].size();j++){
				numerator[i][j] = 1;
				denom[i][j] = 2.0 / (1 + policy_weight[i][j]);
			}
		}
		double loss = 0;
		int cnt = 0;
		for(const Record& record : records){
			//9路限定
			Position pos(9);
			const sheena::Array<int, BoardDim>& edge_dist = edge_distance_max3[9];
			for(int ply = 0;ply<record.size();ply++){
				if(record[ply] != pass){
					//合法手生成
					MoveArray moves;
					sheena::Array<float, 362> policy_score;
					int n_moves = pos.generate_moves(moves, policy_score);
					float sum = 0;
					float best_score = 0;
					for(int i=1;i<n_moves;i++){
						sum += policy_score[i];
						if(moves[i] == record[ply])best_score = policy_score[i];
					}
					loss += -std::log(best_score / sum);
					cnt++;
					//分子の更新
					numerator[edge_dist[record[ply]]][pos.dir4index(record[ply])] += 1;
					//分母の更新
					for(int i=1;i<n_moves;i++){
						denom[edge_dist[moves[i]]][pos.dir4index(moves[i])] += sum;
					}
				}
				//1手進める
				pos.make_move(record[ply]);
			}
		}
		//パラメータの更新
		for(int i=0;i<denom.size();i++){
			for(int j=0;j<denom[i].size();j++){
				policy_weight[i][j] = numerator[i][j] / denom[i][j];
			}
		}
		//損失の表示
		std::cout << "loss " << loss / cnt << std::endl;
	}
}
