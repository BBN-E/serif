// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H

class IDGenerator {
public:
	IDGenerator(int first_ID = 0) : _first_ID(first_ID) { _next_ID = first_ID; }
	IDGenerator(const IDGenerator &other) : _first_ID(other._first_ID), _next_ID(other._next_ID) {}

	int getID() { return _next_ID++; }
	int peek() { return _next_ID; }

	int getNUsedIDs() { return _next_ID - _first_ID; }

	void reset(int first_ID = 0) { _next_ID = _first_ID = first_ID; }

private:
	int _next_ID;
	int _first_ID;
};

#endif
