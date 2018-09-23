#include "shiratama_zenzai.hpp"

std::vector<Record> load_records(std::string filename){
	std::vector<Record> ret;
	std::vector<std::vector<std::string>> lines;
	sheena::read_separated_values<' '>(filename, lines);
	for(const auto& line : lines){
		Record record;
		record.result = std::stoi(line[0]);
		for(int i=1;i<line.size();i++){
			record.push_back(string2intersection(line[i]));
		}
		ret.push_back(record);
	}
	return ret;
}

void store_records(std::string filename, const std::vector<Record>& records){
	std::ofstream out(filename);
	for(const auto& record : records){
		out << record.result;
		for(Intersection i : record){
			out << " " << intersection2string(i);
		}
		out << std::endl;
	}
	out.close();
}