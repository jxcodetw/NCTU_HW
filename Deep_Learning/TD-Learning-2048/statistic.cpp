#include "statistic.h"
#include <fstream>

Statistic::Statistic(std::string path) {
	this->path = path;
	reset();
}

int Statistic::get_max_tile(State &s) {
	int mx = 0;
	for(int i=0;i<16;++i) {
		mx = std::max(mx, (int)(s.flatten[i]));
	}
	return mx;
}

void Statistic::reset() {
	log_size = 0;
	scores = 0;
	score_mx = 0;
	for(int i=0;i<MAX;++i) {
		cnt_pass[i] = 0;
		cnt_max[i] = 0;
	}
}

void Statistic::log(Move &last_move, int score) {
	int mx_tile = get_max_tile(last_move.afterState);
	for(int i=0;i<=mx_tile;++i) {
		cnt_pass[i] += 1;
	}
	cnt_max[mx_tile] += 1;

	scores += score;
	if (score > score_mx) {
		score_mx = score;
	}
	log_size += 1;
}

void Statistic::print() {
	// start
	int j=0;
	while(cnt_pass[j] == log_size) {
		j++;
	}
	j -= 1;

	// end
	int k;
	for(int t=0;t<16;++t) {
		if (cnt_pass[t] != 0) {
			k = t;
		}
	}

	printf("Mean Score: %.2f, Max Score: %d\n", 
		(float)scores / (float)log_size, score_mx);
	for(int i=j;i<=k;++i) {
		float p1 = (float)cnt_pass[i] / log_size * 100.0;
		float p2 = (float)cnt_max[i] / log_size * 100.0;
		
		printf("%5d:\t%.1f%%\t(%.1f%%)\n", (1<<i), p1, p2);
	}
}

void Statistic::save(int step) {
	std::ofstream fout;
	fout.open(path, std::ios::out | std::ios::ate | std::ios::app);
	if (fout.is_open()) {
		fout << step << ',' << (float)scores / (float)log_size << ',' << score_mx << std::endl;
		fout.close();
	} else {
		std::cout << "error opening log file: " << path << std::endl;
	}
}
