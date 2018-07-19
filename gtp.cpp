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
			if(color != state.turn())state.act(pass);
			Intersection i = string2intersection(args[2]);
			bool legal = state.is_move_legal(color, i);
			if(legal){
				state.act(i);
				send("");
			}
			else{
				error("illegal move'");
			}
		}
	};
	responses["show"] = [&](const std::vector<std::string>& args){
		MoveArray moves;
		sheena::Array<float, 362> probabilities;
		int n_action;
		state.get_actions(n_action, moves, probabilities, 0);
		std::cerr << "legal_moves";
		for(int i=0;i<n_action;i++){
			std::cerr << " " << intersection2string(moves[i]);
		}
		std::cerr << std::endl;
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
		if(color != state.turn())state.act(pass);
		sheena::Stopwatch stopwatch;
		std::cerr << "search start" << std::endl;
		//探索
		searcher.search(state, 10000000, 500);
		std::cerr << "time " << stopwatch.msec() <<"[msec]" << std::endl;
		Intersection bestmove = searcher.bestmove(state);
		if(bestmove != resign){
			state.act(bestmove);
		}
		send(intersection2string(bestmove));
	};
}
void gtp(){
	Searcher searcher;
	searcher.resize_tt(256);
	searcher.set_expansion_threshold(10);
	searcher.set_threads(4);
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