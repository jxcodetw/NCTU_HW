#include <iostream>
#include <chrono>
#include <random>
#include <array>
#include <cmath>

unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine generator(seed);
std::normal_distribution<double> gaussian(0.0, 1.0);

const int DNA_LEN = 10;
const int NUM_TEST = 10;
const int NUM_ITER = 10000000;
const double OBJECTIVE = 0.005;

const std::array<double, 3> STEP_SIZES = {0.01, 0.1, 1.0};
const std::array<int, 2> MODES = {0, 1};
const int MODE_PLUS = 0;
const int MODE_COMMA = 1;

double sphere_model(std::array<double, DNA_LEN> x) {
	double sum = 0;
	for(int i=0;i<DNA_LEN;++i) {
		sum += x[i] * x[i];
	}
	return sum;
}

int p3(double init_step_size, int mode) {
	std::array<double, DNA_LEN> parent, child;
	for(int i=0;i<DNA_LEN;++i) {
		parent[i] = 1.0;
	}

	double step_size = init_step_size;
	for(int i=1;i<=NUM_ITER;++i) {
		for(int j=0;j<DNA_LEN;++j) {
			child[j] = parent[j] + step_size * gaussian(generator);
		}
		double child_f = sphere_model(child);
		double parent_f = sphere_model(parent);

		if (child_f <= OBJECTIVE) {
			return i;
		}

		if (mode == MODE_COMMA || child_f <= parent_f) {
			parent = child;
		}
	}
	return -1;
}

int p5(double init_step_size, int mode) {
	const double eps = 0.001;
	std::array<double, DNA_LEN> parent, parent_s, child, child_s;
	for(int i=0;i<DNA_LEN;++i) {
		parent[i] = 1.0;
		parent_s[i] = init_step_size;
	}

	for(int i=1;i<=NUM_ITER;++i) {
		double tp = 1.0 / sqrt(2 * sqrt((double)i));
		double t = exp(-0.00007 * (double)i);

		for(int j=0;j<DNA_LEN;++j) {
			child_s[j] = parent_s[j] * exp(tp * gaussian(generator) + t * gaussian(generator));
			if (child_s[j] < eps) {
				child_s[j] = eps;
			}
			child[j] = parent[j] + child_s[j] * gaussian(generator);
		}

		double child_f = sphere_model(child);
		double parent_f = sphere_model(parent);

		if (child_f <= OBJECTIVE) {
			return i;
		}

		if (mode == MODE_COMMA || child_f <= parent_f) {
			parent_s = child_s;
			parent = child;
		}
	}
	return -1;
}

int p7(double init_step_size, int mode) {
	const double a = 0.85;
	const double G = 30;
	const int G_ = 30;
	double Gs = 0;

	std::array<double, DNA_LEN> parent, child;
	for(int i=0;i<DNA_LEN;++i) {
		parent[i] = 1.0;
	}

	double step_size = init_step_size;
	for(int i=1;i<=NUM_ITER;++i) {
		for(int j=0;j<DNA_LEN;++j) {
			child[j] = parent[j] + step_size * gaussian(generator);
		}
		double child_f = sphere_model(child);
		double parent_f = sphere_model(parent);

		if (child_f <= OBJECTIVE) {
			return i;
		}

		if (mode == MODE_COMMA || child_f <= parent_f) {
			Gs += 1;
			parent = child;
		}

		if (i % G_ == 0) {
			double Ps = Gs / G;
			Gs = 0;

			if (Ps > 1.0/5.0) {
				step_size /= a;
			} else if (Ps < 1.0/5.0) {
				step_size *= a;
			}

			// clip
			if (step_size > 50) {
				step_size = 50;
			}
		}
	}
	return -1;
}

void run_tests(const std::string title, int (*problem)(double, int)) {
	const std::array<std::string, 2> mode_str = {"(1+1)-ES", "(1,1)-ES"};
	const std::array<std::string, 3> step_str = {"0.01", "0.1", "1"};
	for(int m=0;m<MODES.size();++m) {
		for(int i=0;i<STEP_SIZES.size();++i) {
			for(int j=0;j<NUM_TEST;++j) {
				printf("%s, %s, step_size: %s, iter: %d\n",
					title.c_str(),
					mode_str[m].c_str(),
					step_str[i].c_str(),
					problem(STEP_SIZES[i], m));
			}
		}
	}
}

int main() {
	run_tests("Problem 3", p3);
	run_tests("Problem 5", p5);
	run_tests("Problem 7", p7);
	return 0;
}
