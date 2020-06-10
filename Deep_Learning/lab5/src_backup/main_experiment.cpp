#include <iostream>
#include <vector>
#include <ctime>
#include <cstdint>
#include "game.h"
#include "agent.h"
#include "statistic.h"

extern void print_state(State &s);

void pause() {
	char ch;
	std::cin.get(ch);
}

int main(int argc, char* argv[]) {
	int log_interval = 2000;
	int save_interval = 10000;
	int review_interval = save_interval;
	int total_episodes = 10000000;
	float learning_rate = 0.02;
	std::string load_weights_path = "weights.bin";
	std::string save_weights_path = "weights.bin";
	std::string log_path = "2048.log";

	// print arguments
	std::cout << "============================" << std::endl;
	std::cout << "total episodes: " << total_episodes << std::endl;
	std::cout << "  log interval: " << log_interval << std::endl;
	std::cout << " save interval: " << save_interval << std::endl;
	std::cout << " learning rate: " << learning_rate << std::endl;
	std::cout << "  load weights: " << load_weights_path << std::endl;
	std::cout << "  save weights: " << save_weights_path << std::endl;
	std::cout << "      log file: " << log_path << std::endl;
	std::cout << "============================" << std::endl;
	

	Statistic stat(log_path);
	//Agent ai(learning_rate, time(NULL));
	Agent ai(learning_rate, 9487);

	// Leon 5 tuple
	//ai.tuple_net.add({0, 1, 2, 3, 4});
	//ai.tuple_net.add({0, 1, 4, 5, 6});
	//ai.tuple_net.add({1, 2, 3, 5, 6});
	//ai.tuple_net.add({5, 6, 7, 9, 10});

	// TA 6 tuple
	ai.tuple_net.add({ 0, 1, 2, 3, 4, 5 });
	ai.tuple_net.add({ 4, 5, 6, 7, 8, 9 });
	ai.tuple_net.add({ 0, 1, 2, 4, 5, 6 });
	ai.tuple_net.add({ 4, 5, 6, 8, 9, 10 });

	// From Systematic Selection of N-Tuple Networks with Consideration of Interinfluence for Game 2048
	// 6 tuple
	//ai.tuple_net.add({ 0, 1, 2, 4, 5, 6 });
	//ai.tuple_net.add({ 1, 2, 5, 6, 9, 13 });
	//ai.tuple_net.add({ 0, 1, 2, 3, 4, 5 });
	//ai.tuple_net.add({ 0, 1, 5, 6, 7, 10 });
	//ai.tuple_net.add({ 0, 1, 2, 5, 9, 10 });
	//ai.tuple_net.add({ 0, 1, 5, 9, 13, 14 });
	//ai.tuple_net.add({ 0, 1, 5, 8, 9, 13 });
	//ai.tuple_net.add({ 0, 1, 2, 4, 6, 10 });

	// 7 tuple
	//ai.tuple_net.add({ 0, 1, 2, 3, 4, 5, 9 });
	//ai.tuple_net.add({ 0, 1, 2, 5, 8, 9, 13 });
	//ai.tuple_net.add({ 0, 1, 2, 5, 8, 9, 10 });
	//ai.tuple_net.add({ 0, 1, 2, 3, 4, 5, 6 });
	//ai.tuple_net.add({ 0, 1, 4, 5, 6, 10, 11 });
	//ai.tuple_net.add({ 0, 1, 2, 5, 6, 9, 13 });
	//ai.tuple_net.add({ 0, 1, 2, 3, 4, 8, 12 });
	//ai.tuple_net.add({ 0, 1, 5, 6, 9, 10, 13 });

	ai.tuple_net.load(load_weights_path);
	std::cout << "Start Training..." << std::endl;
	std::vector<std::vector<Move>> pool;
	for(int i=1;i<=total_episodes;++i) {
		std::vector<Move> h = ai.play_game();
		stat.log(h.back(), ai.game.score);
		pool.push_back(h);
		std::vector<Move> hc = h;
		ai.learn_backward(hc);

		if (i % log_interval == 0) {
			std::cout << "==============================" << std::endl;
			std::cout << "Step: " << i << std::endl;
			stat.print();
			stat.save(i);
			stat.reset();
		}
		if (i % review_interval == 0) {
			std::cout << "learning" << std::endl;
			for(int bd=0;bd<10;++bd) {
				std::cout << "random sample learning" << std::endl;
				for(int k=0;k<review_interval;++k) {
					std::vector<Move> hhh = pool[rand() % review_interval];
					ai.learn_backward_review(hhh);
				}
			}
			pool.clear();
			std::cout << "learned" << std::endl;
		}

		if (i % save_interval == 0) {
			ai.tuple_net.save(save_weights_path);
		}
	}

	return 0;
}
