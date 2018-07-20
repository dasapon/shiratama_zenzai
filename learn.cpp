#include "state.hpp"
#include "policy.hpp"

static constexpr int selfplay_threads = 12;
void generate_records(){
	sheena::ArrayAlloc<Searcher> searcher(selfplay_threads);
	std::vector<Record> records;
	std::mutex mtx;
	for(int i=0;i<selfplay_threads;i++){
		searcher[i].set_random();
		searcher[i].set_expansion_threshold(20);
		searcher[i].set_komi(7);
	}
	Searcher book;
	book.set_expansion_threshold(20);
	{
		State book_state(book, 9);
		book.resize_tt(1024 * 12);
		book.set_virtual_loss(5, -1);
		book.set_komi(7);
		book.set_threads(selfplay_threads);
		book.search(book_state, 1800 * 1000, 7200 * 1000 * 3);
		book.bestmove(book_state);
	}
	
	//棋譜生成
	omp_set_num_threads(selfplay_threads);
	sheena::Stopwatch stopwatch;
#pragma omp parallel for schedule(dynamic)
	for(int i=0;i<7200;i++){
		size_t thread_id = omp_get_thread_num();
		Record record;
		record.result = 0;
		State state(searcher[thread_id], 9);
		sheena::Array<double, 2> result;
		for(int ply = 0; ply < 81; ply++){
			if(state.terminate(result)){
				if(result[0] > 0)record.result = 1;
				else if(result[0] < 0)record.result = -1;
				break;
			}
			//1手打つ
			Intersection move = book.select(state, 1000);
			if(move == resign){
				searcher[thread_id].search(state, 1000, 1000);
				move = searcher[thread_id].select(state, 0);
			}
			state.act(move);
			record.push_back(move);
		}
		std::lock_guard<std::mutex> lk(mtx);
		records.push_back(record);
		if(thread_id == 0)std::cout << records.size() << ", " << stopwatch.sec() << "[sec]" << std::endl;
	}
	store_records("selfplay.txt",records);
}