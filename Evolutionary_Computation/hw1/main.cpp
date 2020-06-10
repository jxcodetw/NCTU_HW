#include <iostream>
#include <fstream>
#include <vector>
#include <array>

const int BIT_LEN = 50;
const int POP_SIZE = 200;
const int ITERATION = 100;
typedef std::array<bool, BIT_LEN> Gene;

std::array<Gene, POP_SIZE> creatures, mating_pool;
std::array<double, POP_SIZE> fit;
double best_fit, sum_fit;

std::vector<int> max_fitness_log;

double base_fitness = 0;

double fitness(Gene &g) {
	double f = base_fitness;
	for(int i=0;i<BIT_LEN;++i) {
		if (g[i] == true) {
			f += 1.0;
		}
	}
	return f;
}

void crossover() {
	if (POP_SIZE % 2 == 1) {
		std::cout << "crossover warning" << std::endl;
		return;
	}
	for(int i=0;i<POP_SIZE;i+=2) {
		int cut = rand() % (BIT_LEN - 2);
		for(int j=0;j<cut;++j) {
			bool tmp = mating_pool[i][j];
			mating_pool[i][j] = mating_pool[i+1][j];
			mating_pool[i+1][j] = tmp;
		}
	}
}

void roulette_wheel_selection() {
	static std::array<double, POP_SIZE> wheel;
	double acc = 0;
	for(int i=0;i<POP_SIZE;++i) {
		acc += fit[i];
		wheel[i] = acc / sum_fit;
	}
	for(int i=0;i<POP_SIZE;++i) {
		double p = ((double)rand() / (double)RAND_MAX);
		for(int j=0;j<POP_SIZE;++j) {
			if (p < wheel[j]) {
				mating_pool[i] = creatures[j];
				break;
			}
		}
	}
}

void tournament_selection() {
	for(int i=0;i<POP_SIZE;++i) {
		int p = rand() % POP_SIZE;
		int q = rand() % POP_SIZE;

		if (fit[p] > fit[q]) {
			mating_pool[i] = creatures[p];
		} else {
			mating_pool[i] = creatures[q];
		}
	}
}

void replacement() {
	creatures = mating_pool;
}

void udpate_fitness() {
	best_fit = -1;
	sum_fit = 0;
	for(int i=0;i<POP_SIZE;++i) {
		fit[i] = fitness(creatures[i]);
		best_fit = std::max(best_fit, fit[i]);
		sum_fit += fit[i];
	}
	max_fitness_log.push_back(best_fit);
}

void print_status(int gen) {
	std::cout << "Generation " << gen << ": " << best_fit << std::endl;
}

void init() {
	for(int i=0;i<POP_SIZE;++i) {
		for(int j=0;j<BIT_LEN;++j) {
			creatures[i][j] = rand() % 1000 > 500;
		}
	}
	udpate_fitness();
}

void save_log(std::string filename) {
	std::fstream fp;
	fp.open(filename, std::ios::out);
	for(int i=0;i<max_fitness_log.size();++i) {
		fp << max_fitness_log[i] << std::endl;
	}
	fp.close();
}

int main(int argc, char *argv[]) {
	if (argc > 3) {
		base_fitness = std::stod(argv[3]);
	}
	bool is_tournament = false;
	if (argc > 4) {
		is_tournament = true;
	}
	srand(time(0)+std::stoul(argv[1]));

	init();
	print_status(0);

	for(int i=1;i<=ITERATION;++i) {
		// selection
		if (is_tournament) {
			tournament_selection();
		} else {
			roulette_wheel_selection();
		}
		crossover(); // pc = 1.0
		replacement(); // generational

		udpate_fitness();
		print_status(i);
	}

	if (argc > 2) {
		save_log(argv[2]);
	}

	return 0;
}