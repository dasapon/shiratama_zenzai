#include "state.hpp"
#include "state.hpp"

static void send(std::string result){
	std::cout << "= " << result << "\n" << std::endl;
}
static void error(std::string str){
	std::cout << "? " << str << "\n" << std::endl;
}

static const std::string x_char(" ABCDEFGHJKLMNOPQRST");
static const std::string x_char_small(" abcdefghjklmnopqrst");
static const sheena::Array<std::string, 4> black_str({
	"b", "B", "black", "BLACK"
});
static const sheena::Array<std::string, 4> white_str({
	"w", "W", "white", "WHITE"
});
std::string intersection2string(Intersection i){
	if(i == pass){
		return "pass";
	}
	else if(i == resign){
		return "resign";
	}
	return x_char[i % BoardWidth] + std::to_string(i / BoardWidth);
}
Intersection string2intersection(std::string str){
	if(str == "pass" || str == "PASS")return pass;
	int x;
	for(x=0;x<x_char.size();x++){
		if(str[0] == x_char[x] || str[0] == x_char_small[x]){
			break;
		}
	}
	int y = std::stoi(str.substr(1, str.size()));
	return intersection(x, y);
}

static void init_responses(std::map<std::string, std::function<void(const std::vector<std::string>& args)>>& responses, Searcher& searcher, State& state){
	responses["protocol_version"] = [](const std::vector<std::string>& args){
		send("2");
	};
	responses["name"] = [](const std::vector<std::string>& args){
		send("ShiratamaZenzai");
	};
	responses["version"] = [](const std::vector<std::string>& args){
		send("dev");
	};
	responses["known_command"] = [&](const std::vector<std::string>& args){
		if(args.size() < 2)error("few args");
		else{
			for(const auto& res : responses){
				if(res.first == args[1]){
					send("true");
					return;
				}
			}
			send("false");
		}
	};
	responses["list_commands"] = [&](const std::vector<std::string>& args){
		std::cout << "=";
		for(const auto& res : responses){
			std::cout << " " << res.first;
		}
		std::cout << "\n" << std::endl;
	};
	responses["quit"] = [](const std::vector<std::string>& args){
		std::exit(0);
	};
	responses["boardsize"] = [&](const std::vector<std::string>& args){
		if(args.size() < 2)error("few args");
		else{
			searcher.clear_tt();
			int board_size = std::stoi(args[1]);
			if(board_size >= 9 && board_size<=19){
				state = State(searcher, board_size);
				send("");
			}
			else{
				error("nacceptable size");
			}
		}
	};
	responses["clear_board"] = [&](const std::vector<std::string>& args){
		state.clear();
		searcher.clear_tt();
		send("");
	};
	responses["komi"] = [&](const std::vector<std::string>& args){
		if(args.size() < 2)error("few args");
		else {
			searcher.set_komi(std::stod(args[1]));
			send("");
		}
	};
	responses["play"] = [&](const std::vector<std::string>& args){
		if(args.size() < 3)error("few args");
		else{
			Stone color = Black;
			for(const auto& str : white_str){
				if(args[1] == str){
					color = White;
					break;
				}
			}
			if(color != state.turn())state.act(pass, 0);
			Intersection i = string2intersection(args[2]);
			bool legal = state.is_move_legal(i);
			if(legal){
				state.act(i, 0);
				send("");
			}
			else{
				error("illegal move'");
			}
		}
	};
	responses["show"] = [&](const std::vector<std::string>& args){
		MoveArray moves;
		sheena::Array<float, MaxLegalMove> probabilities;
		int n_action;
		state.get_actions(n_action, moves, probabilities, 0);
		std::cerr << "legal_moves";
		for(int i=0;i<n_action;i++){
			std::cerr << " " << intersection2string(moves[i]);
		}
		std::cerr << std::endl;
		std::cerr << "super_kou " << state.super_kou << std::endl;;
	};
	responses["genmove"] = [&](const std::vector<std::string>& args){
		//手番
		Stone color = Black;
		for(const auto& str : white_str){
			if(args[1] == str){
				color = White;
				break;
			}
		}
		if(color != state.turn())state.act(pass, 0);
		sheena::Stopwatch stopwatch;
		bool sended = false;
		//日本ルール対応(一応)
		if(state.progress() >= 0.5 && state.lastmove() == pass){
			send(intersection2string(pass));
			state.act(pass, 0);
			sended = true;
		}
		auto book = [&](std::string move){
			if(sended)return;
			Intersection i = string2intersection(move);
			if(state.is_move_legal(i)){
				state.act(i, 0);
				send(move);
				sended = true;
			}
			return;
		};

		book("G7");
		book("G13");
		book("N7");
		book("N13");
		if(state.is_empty(string2intersection("D3"))
		&& state.is_empty(string2intersection("C4")))book("C3");
		if(state.is_empty(string2intersection("Q17"))
		&& state.is_empty(string2intersection("R16")))book("R17");
		if(sended)return;
		std::cerr << "search start " << std::endl;
		int sec = 20;
		//探索
		searcher.search(state, sec * 1000, 200000);
		std::cerr << "time " << stopwatch.msec() <<"[msec]" << std::endl;
		Intersection bestmove = searcher.bestmove<true>(state);
		if(bestmove != resign){
			state.act(bestmove, 0);
		}
		send(intersection2string(bestmove));
	};
}
void gtp(){
	Searcher searcher;
	searcher.set_random();
	searcher.resize_tt(256);
	searcher.set_expansion_threshold(0);
	searcher.set_threads(8);
	searcher.set_virtual_loss(5, -1);
	State state(searcher, 19);
	std::map<std::string, std::function<void(const std::vector<std::string>& args)>> responses;
	init_responses(responses, searcher, state);
	std::string line;
	while(std::getline(std::cin, line)){
		std::vector<std::string> command = sheena::split_string(line, ' ');
		if(command.size() > 0){
			if(responses.find(command[0]) != responses.end()){
				responses[command[0]](command);
			}
			else{
				error("unknown command");
			}
		}
	}
}