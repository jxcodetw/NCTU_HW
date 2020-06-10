#include <iostream>
#include <cstdint>
#include <limits>
#include "state.h"

bool State::operator==(const State &s) const {
	for(int i=0;i<4;++i) {
		if (row[i] != s.row[i]) {
			return false;
		}
	}
	return true;
}

bool State::operator!=(const State &s) const {
	return !(*this == s);
}


Move::Move() {
	// place holder
}

Move::Move(State s, int a) {
	reward = 0;
	beforeState = s;
	action = a;
	switch(action) {
	case 0:
		afterState = move_up(beforeState);
		break;
	case 1:
		afterState = move_right(beforeState);
		break;
	case 2:
		afterState = move_down(beforeState);
		break;
	case 3:
		afterState = move_left(beforeState);
		break;
	default:
		std::cout << "Fuckkkk" << std::endl;
		break;
	}
	if (!is_valid()) {
		reward = -1;
		value = -std::numeric_limits<float>::max();
	}
}

State Move::mirror(State s) {
	State ret;
	for(int r=0;r<4;++r) {
		for(int c=0;c<4;++c) {
			ret.board[r][3-c] = s.board[r][c];
		}
	}
	return ret;
}

State Move::rotate_right(State s) {
	State ret;
	for(int r=0;r<4;++r) {
		for(int c=0;c<4;++c) {
			ret.board[c][3-r] = s.board[r][c];
		}
	}
	return ret;
}

State Move::rotate_left(State s) {
	State ret;
	for(int r=0;r<4;++r) {
		for(int c=0;c<4;++c) {
			ret.board[3-c][r] = s.board[r][c];
		}
	}
	return ret;
}

State Move::move_right(State s) {
	s = mirror(s);
	s = move_left(s);
	s = mirror(s);
	return s;
}

State Move::move_down(State s) {
	s = rotate_right(s);
	s = move_left(s);
	s = rotate_left(s);
	return s;
}

State Move::move_up(State s) {
	s = rotate_left(s);
	s = move_left(s);
	s = rotate_right(s);
	return s;
}

State Move::move_left(State s) {
	for(int r=0;r<4;++r) {
		int top = 0;
		int tmp = 0;
		for(int c=0;c<4;++c) {
			int tile = s.board[r][c];
			if (tile == 0) {continue;}
			s.board[r][c] = 0;

			if (tmp != 0) {
				if (tile == tmp) {
					tile += 1;
					s.board[r][top++] = tile;
					reward += (1<<tile);
					tmp = 0;
				} else {
					s.board[r][top++] = tmp;
					tmp = tile;
				}
			} else {
				tmp = tile;
			}
		}
		if (tmp != 0) {
			s.board[r][top] = tmp;
		}
	}
	return s;
}

bool Move::is_valid() {
	return beforeState != afterState;
}
