#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include "game.h"

Game::Game() {

}

void Game::init() {
	this->score = 0;
	for(int i=0;i<16;++i) {
		this->board.flatten[i] = 0;
	}
	this->add_random_tile();
	this->add_random_tile();
	this->update_after_state();
}

void Game::add_random_tile() {
	int empty[16], top = 0;
	for(int i=0;i<16;++i) {
		if (this->board.flatten[i] == 0) {
			empty[top++] = i;
		}
	}
	if (top == 0) {
		// if no empty
		// then don't do anything
		return;
	}
	int idx = rand() % top;
	int prob = rand() % 10;
	this->board.flatten[empty[idx]] = prob ? 1 : 2;
}

void Game::update_after_state() {
	for(int i=0;i<4;++i) {
		this->move[i] = Move(this->board, i);
	}
}

State Game::take(Move &m) {
	this->score += m.reward;
	this->board = m.afterState;
	this->add_random_tile();
	this->update_after_state();
	return this->board;
}

bool Game::is_terminate_state() {
	for(int i=0;i<4;++i) {
		if (this->move[i].is_valid()) {
			return false;
		}
	}
	return true;
}
