#include "state.hpp"
#include "policy.hpp"

static constexpr int selfplay_threads = 12;
void generate_records(){
	sheena::ArrayAlloc<Searcher> searcher(selfplay_threads);
	std::vector<Record> records;
	std::mutex mtx;
	for(int i=0;i<selfplay_threads;i++){
		searcher[i].set_random();
	}
	//棋譜生成
	omp_set_num_threads(selfplay_threads);
#pragma omp parallel for schedule(dynamic)
	for(int i=0;i<4800;i++){
		size_t thread_id = omp_get_thread_num();
		Record record;
		record.result = 0;
		State state(searcher[thread_id], 9);
		sheena::Array<double, 2> result;
		for(int ply = 0; ply < 81 * 2; ply++){
			if(state.terminate(result)){
				if(result[0] > 0)record.result = 1;
				else if(result[0] < 0)record.result = -1;
				break;
			}
			//1手打つ
			searcher[thread_id].search(state, 1000, 500);
			Intersection move = searcher[thread_id].select(state);
			state.act(move);
			record.push_back(move);
		}
		std::lock_guard<std::mutex> lk(mtx);
		records.push_back(record);
	}
	store_records("selfplay.txt",records);
}