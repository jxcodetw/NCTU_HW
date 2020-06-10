#include <vector>
#include <iostream>
#include <limits>
#include "agent.h"

Agent::Agent(float learning_rate, unsigned seed) {
	std::cout << "seed:" << seed << std::endl;
	std::srand(seed);
	this->alpha = learning_rate;
}

void print_state(State &s) {
	for(int i=0;i<4;++i) {
		for(int j=0;j<4;++j) {
			if (s.board[i][j] == 0) {
				printf("%5d ",0 );
			} else {
				printf("%5d ", 1<<s.board[i][j]);
			}
		}
		puts("");
	}
}

std::vector<Move> Agent::play_game() {
	std::vector<Move> history;
	game.init();
	while (!game.is_terminate_state()) {
		Move *best_move = &(game.move[0]);
		for(size_t i=0;i<4;++i) {
			if (game.move[i].is_valid()) {	
				game.move[i].value = (float)game.move[i].reward + tuple_net.V(game.move[i].afterState);
				if (game.move[i].value > best_move->value) {
					best_move = &(game.move[i]);
				}
			}
		}
		Move best = *best_move;
		game.take(*best_move);
		history.push_back(best);
	}
	return history;
}

float Agent::evaluate_state(State s) {
	float result = 0;
	float cnt_empty = 0;
	for(int i=0;i<16;++i) {
		if (s.flatten[i] == 0) {
			cnt_empty += 1;
			// try 2
			State s2 = s;
			s2.flatten[i] = 1;
			result += 0.9 * tuple_net.V(s2);
			// try 4
			State s4 = s;
			s4.flatten[i] = 2;
			result += 0.1 * tuple_net.V(s4);

		}
	}
	return result / cnt_empty;
}

std::vector<Move> Agent::play_game_tdstate() {
	std::vector<Move> history;
	game.init();
	while(!game.is_terminate_state()) {
		Move *best_move = &(game.move[0]);
		for(size_t i=0;i<4;++i) {
			if (game.move[i].is_valid()) {	
				game.move[i].value = (float)game.move[i].reward + evaluate_state(game.move[i].afterState);
				if (game.move[i].value > best_move->value) {
					best_move = &(game.move[i]);
				}
			}
		}
		Move best = *best_move;
		game.take(*best_move);
		best.afterState = game.move[0].beforeState;
		history.push_back(best);
	}
	return history;
}

void Agent::learn_backward_review(std::vector<Move> &history) {
	for(size_t i=0;i<history.size();++i) {
		Move &h = history[i];
		h.value = (float)h.reward + tuple_net.V(h.afterState);
	}
	float last = 0;
	while(!history.empty()) {
		Move &h = history.back();
		last = h.reward + this->tuple_net.update_V(h.afterState, this->alpha * (last - (h.value - h.reward)));
		history.pop_back();
	}
}

void Agent::learn_backward_tdstate(std::vector<Move> &history) {
	float last = 0;
	while(!history.empty()) {
		Move &h = history.back();
		float diff = last - tuple_net.V(h.beforeState);
		last = this->tuple_net.update_V(h.beforeState, this->alpha * (h.reward + diff));
		history.pop_back();
	}
}

void Agent::learn_backward(std::vector<Move> &history) {
	float last = 0;
	while(!history.empty()) {
		Move &h = history.back();
		last = h.reward + this->tuple_net.update_V(h.afterState, this->alpha * (last - (h.value - h.reward)));
		history.pop_back();
	}
}
