#pragma once
#include "game.h"
#include "tuple_network.h"

class Agent {
private:
public:
	Game game;
	float alpha;
public:
	TupleNetwork tuple_net;

	Agent(float learning_rate, unsigned seed);
	std::vector<Move> play_game();
	void learn_backward_review(std::vector<Move> &history);
	void learn_backward(std::vector<Move> &history);

	float evaluate_state(State s);
	std::vector<Move> play_game_tdstate();
	void learn_backward_tdstate(std::vector<Move> &history);
};
