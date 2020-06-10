#pragma once

#include <iostream>
#include <cstdint>

union State {
	uint8_t flatten[16];
	uint8_t board[4][4];
	uint32_t row[4];
	bool operator==(const State &s) const;
	bool operator!=(const State &s) const;
};

class Move {
public:
	State beforeState;
	State afterState;
	int action;
	int reward;
	float value;

	Move();
	Move(State s, int a);

private:
	State mirror(State s);
	State rotate_right(State s);
	State rotate_left(State s);

	State move_right(State s);
	State move_down(State s);
	State move_up(State s);
	State move_left(State s);

public:
	bool is_valid();
};

