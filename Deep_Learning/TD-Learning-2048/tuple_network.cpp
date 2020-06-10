#include "tuple_network.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cerrno>

static bool yes_no(std::string question) {
	std::string answer;
	while(true) {
		std::cout << question << " (y/n)? ";
		std::cin >> answer;
		std::transform(answer.begin(), answer.end(), answer.begin(), ::tolower);
		if (answer == "y" || answer == "yes") {
			return true;
		} else if (answer == "n" || answer == "no") {
			return false;
		} else {
			std::cout << "invalid choice" << std::endl;
		}
	}
}

NTuple::NTuple(std::vector<std::pair<int, int>> feature, std::vector<int> _c, std::array<Position, 8> &isomap) {
	c = _c;
	#ifdef AI_JS_OUTPUT
		std::cout << "  [ " << std::endl;
	#endif
	for(auto m : isomap) {
		Tuple tuple;

		#ifdef AI_JS_OUTPUT
			std::cout << "    [ ";
		#endif
		for(auto f : feature) {
			auto pos = m[f.first][f.second];
			#ifdef AI_JS_OUTPUT
				std::cout << '[' << pos.first << ',' << pos.second << "], ";
			#endif
			tuple.push_back(pos.first * 4 + pos.second);
		}
		#ifdef AI_JS_OUTPUT
			std::cout << "]," << std::endl;
		#endif
		isomorphic.push_back(tuple);
	}
	#ifdef AI_JS_OUTPUT
		std::cout << "  ], " << std::endl;
	#endif

	size = (1<<(feature.size() * 4));
	w = new float[this->size]();

	#ifndef AI_JS_OUTPUT
		for(auto f : feature) {
			std::cout << std::hex << (f.first * 4 + f.second);
		}
		std::cout << ' ' << std::dec;

		size_t usage = size * sizeof(float);
		if (usage >= (1 << 30)) {
			std::cout << " (" << (usage >> 30) << "GB)";
		} else if (usage >= (1 << 20)) {
			std::cout << " (" << (usage >> 20) << "MB)";
		} else if (usage >= (1 << 10)) {
			std::cout << " (" << (usage >> 10) << "KB)";
		}
		std::cout << std::endl;

	#endif
}

float NTuple::V(State &s) {
	float res = 0;
	for(size_t i=0;i<this->isomorphic.size();++i) {
		Tuple &feature = this->isomorphic[i];
		int idx = 0;
		for(size_t j=0;j<feature.size();++j) {
			idx += s.flatten[feature[j]] * this->c[j];
		}
		res += this->w[idx];
	}
	return res;
}

float NTuple::update_V(State &s, float val) {
	val /= this->isomorphic.size();
	float res = 0;
	for(size_t i=0;i<this->isomorphic.size();++i) {
		Tuple &feature = this->isomorphic[i];
		int idx = 0;
		for(size_t j=0;j<feature.size();++j) {
			idx += s.flatten[feature[j]] * this->c[j];
		}
		this->w[idx] += val;
		res += this->w[idx];
	}
	return res;
}

TupleNetwork::TupleNetwork() {
	for(int i=0;i<7;++i) {
		this->c.push_back(0x1<<(i*4));
	}

	Position map;
	for(int i=0;i<4;++i) {
		for(int j=0;j<4;++j) {
			map[i][j] = std::make_pair(i, j);
		}
	}
	for(size_t i=0;i<isomap.size();++i) {
		if (i == 4) { // mirror left right
			Position tmp;
			for(int r=0;r<4;++r) {
				for(int c=0;c<4;++c) {
					tmp[r][3-c] = map[r][c];
				}
			}
			map = tmp;
		}
		this->isomap[i] = map;
		// rotate right
		Position tmp;
		for(int r=0;r<4;++r) {
			for(int c=0;c<4;++c) {
				tmp[c][3-r] = map[r][c];
			}
		}
		map = tmp;
	}
}

void TupleNetwork::add(std::vector<int> f) {
	std::vector<std::pair<int, int>> pos;
	for(auto v : f) {
		pos.push_back(std::make_pair(v/4, v%4));
	}
	this->tuples.push_back(new NTuple(pos, this->c, this->isomap));
}

float TupleNetwork::V(State &s) {
	float res = 0;
	for(size_t i=0;i<tuples.size();++i) {
		res += tuples[i]->V(s);
	}
	return res;
}

float TupleNetwork::update_V(State &s, float val) {
	float res = 0;
	val /= tuples.size();
	for(size_t i=0;i<tuples.size();++i) {
		res += tuples[i]->update_V(s, val);
	}
	return res;
}

// save & load

void TupleNetwork::save(std::string path) {
	std::ofstream fout;
	fout.open(path, std::ios::out | std::ios::binary | std::ios::trunc);

	if (!fout.is_open()) {
		std::cout << "save error" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	#ifdef AI_JS_OUTPUT
		std::ofstream ai_fout;
		ai_fout.open("ai.w", std::ios::out | std::ios::trunc);
		ai_fout << "this.w = [" << std::endl;
	#endif

	size_t size = tuples.size();
	fout.write((char*)(&size), sizeof(size_t));
	for(size_t i=0;i<tuples.size();++i) {
		fout.write((char*)(&(tuples[i]->size)), sizeof(size_t));
		fout.write((char*)(tuples[i]->w), sizeof(float) * tuples[i]->size);

		#ifdef AI_JS_OUTPUT
			ai_fout << "  [ ";
			for(size_t j=0;j<tuples[i]->size;++j) {
				ai_fout << tuples[i]->w[j] << ", ";
			}
			ai_fout << "]," << std::endl;
		#endif
	}

	#ifdef AI_JS_OUTPUT
		ai_fout << "];" << std::endl;
		ai_fout.close();
	#endif

	fout.close();
	std::cout << "weights saved: " << path << std::endl;
}

static void check_size(size_t expected, size_t got) {
	if (expected != got) {
		std::cout << "size mismatch: expected " << expected << " but got " << got << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void TupleNetwork::load(std::string path) {
	std::ifstream fin;
	fin.open(path, std::ios::in | std::ios::binary);
	if (!fin.is_open()) {
		if (errno == ENOENT) {
			std::cout << "not such file: " << path << std::endl;
			std::cout << "training from scratch." << std::endl;
		} else {
			std::cout << "load error" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	} else {
		std::cout << "found: " << path << std::endl;
		std::cout << "training with saved weight." << std::endl;
	}
	if (yes_no("continue")) {
		if (fin.good()) {
			size_t size;
			fin.read((char*)(&size), sizeof(size_t));
			check_size(tuples.size(), size);
			for(size_t i=0;i<tuples.size();++i) {
				fin.read((char*)(&size), sizeof(size_t));
				check_size(tuples[i]->size, size);
				fin.read((char*)(tuples[i]->w), sizeof(float) * size);
			}
			std::cout << "weights loaded: " << path << std::endl;
		} else {
			return;
		}
	} else {
		std::exit(EXIT_SUCCESS);
	}
}



