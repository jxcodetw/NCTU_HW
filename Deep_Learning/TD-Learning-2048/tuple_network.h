#pragma once

#include <vector>
#include <array>
#include "game.h"

class NTuple {
public:
	typedef std::vector<int> Tuple;
	typedef std::array<std::array<std::pair<int, int>, 4>, 4> Position;
	float* w;
	std::vector<int> c;
	std::vector<Tuple> isomorphic;
	size_t size;

	NTuple(std::vector<std::pair<int, int>> feature, std::vector<int> _c, std::array<Position, 8> &isomap);
	float V(State &s);
	float update_V(State &s, float val);
};

class TupleNetwork {
public:
	typedef std::array<std::array<std::pair<int, int>, 4>, 4> Position;
	std::array<Position, 8> isomap;
	std::vector<NTuple*> tuples;
	std::vector<int> c;

	TupleNetwork();
	void add(std::vector<int> f);
	float V(State &s);
	float update_V(State &s, float val);

	void load(std::string path);
	void save(std::string path);
};
