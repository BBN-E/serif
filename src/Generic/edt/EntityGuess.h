// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITYGUESS_H
#define ENTITYGUESS_H

struct EntityGuess {
	static const int NEW_ENTITY = -1;
	//entity ID for this guess.  The value -1 is reserved to indicate a new entity
	int id;
	EntityType type;
	double score;
	// experimental (for ace08 eval)
	float linkConfidence;
};

#endif
