#pragma once

#include <iostream>
#include <cstdint>
#include <limits>
#include "state.h"

class Game {
private:
public:
	int score;
	State board;
	Move move[4];

	void update_after_state();
	void add_random_tile();
public:
	Game();
	void init();
	bool is_terminate_state();
	State take(Move &m);
};
