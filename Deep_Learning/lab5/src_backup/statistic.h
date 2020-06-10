#pragma once

#include <cstdint>
#include "state.h"

#define MAX 16
class Statistic {
private:
	std::string path;
	int scores;
	int score_mx;
	int cnt_pass[MAX];
	int cnt_max[MAX];

	int log_size;

	int get_max_tile(State &s);

public:
	Statistic(std::string path);
	void reset();
	void print();
	void log(Move &last_move, int score);
	void save(int i);
};
