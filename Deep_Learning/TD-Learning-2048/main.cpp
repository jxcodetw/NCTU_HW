#include <iostream>
#include <vector>
#include <ctime>
#include <cstdint>
#include <unistd.h>

#include "game.h"
#include "agent.h"
#include "statistic.h"

int main(int argc, char* argv[]) {
	int log_interval = 2000;
	int save_interval = log_interval * 10;
	int total_episodes = 10000000;
	float learning_rate = 0.01;
	std::string load_weights_path = "weights.bin";
	std::string save_weights_path = "weights.bin";
	std::string log_path = "2048.log";
	std::string mode = "td-afterstate";

	// print arguments
	std::cout << "============================" << std::endl;
	std::cout << "total episodes: " << total_episodes << std::endl;
	std::cout << "  log interval: " << log_interval << std::endl;
	std::cout << " save interval: " << save_interval << std::endl;
	std::cout << " learning rate: " << learning_rate << std::endl;
	std::cout << "  load weights: " << load_weights_path << std::endl;
	std::cout << "  save weights: " << save_weights_path << std::endl;
	std::cout << "      log file: " << log_path << std::endl;
	std::cout << "          mode: " << mode << std::endl;
	std::cout << "============================" << std::endl;


	Statistic stat(log_path);
	//Agent ai(learning_rate, time(NULL));
	Agent ai(learning_rate, 9487);

	#ifdef AI_JS_OUTPUT
		int save_stdout = dup(STDOUT_FILENO);
		// redirect stdout to file
		freopen("ai.ntuple", "w", stdout);
		std::cout << "this.ntuple = [ " << std::endl;
	#endif
	// ai.tuple_net.add({0, 1, 2, 3});
	// ai.tuple_net.add({0, 1, 4, 5});
	// ai.tuple_net.add({1, 2, 3, 5});
	// ai.tuple_net.add({5, 6, 7, 9});
	// ai.tuple_net.add({1, 2, 6, 10});
	// Leon 5 tuple
	ai.tuple_net.add({0, 1, 2, 3});
	ai.tuple_net.add({0, 1, 4, 5});
	ai.tuple_net.add({1, 2, 3, 5});
	ai.tuple_net.add({5, 6, 7, 9});
	ai.tuple_net.add({1, 2, 6, 10});
	#ifdef AI_JS_OUTPUT
		std::cout << "];" << std::endl;
		// restore stdout
		fclose(stdout);
		stdout = fdopen(save_stdout, "w");
		std::cout << "ai.ntuple saved." << std::endl;
	#endif

	// TA 6 tuple
	//ai.tuple_net.add({ 0, 1, 2, 3, 4, 5 });
	//ai.tuple_net.add({ 4, 5, 6, 7, 8, 9 });
	//ai.tuple_net.add({ 0, 1, 2, 4, 5, 6 });
	//ai.tuple_net.add({ 4, 5, 6, 8, 9, 10 });

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

	// ai.tuple_net.load(load_weights_path);
	std::cout << "Start Training..." << std::endl;
	for(int i=1;i<=total_episodes;++i) {
		if (mode == "td-state") {
			std::vector<Move> h = ai.play_game_tdstate();
			stat.log(h.back(), ai.game.score);
			ai.learn_backward_tdstate(h);
		} else if (mode == "td-afterstate") {
			std::vector<Move> h = ai.play_game();
			stat.log(h.back(), ai.game.score);
			ai.learn_backward(h);
		} else {
			std::cout << "unkown mode: " << mode << std::endl;
			exit(-1);
		}

		if (i % log_interval == 0) {
			std::cout << "==============================" << std::endl;
			std::cout << "Step: " << i << std::endl;
			stat.print();
			stat.save(i);
			stat.reset();
		}
		if (save_interval > 0 && i % save_interval == 0) {
			ai.tuple_net.save(save_weights_path);
		}
	}

	return 0;
}
