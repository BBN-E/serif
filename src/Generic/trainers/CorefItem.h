// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef COREFITEM_H
#define COREFITEM_H

#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

class CorefItem {
public:
	static const int NO_ID = -1;

	CorefItem() : mention(0), node(0), corefID(NO_ID), prolinkID(NO_ID) { }
	~CorefItem() {
		node = 0;
		delete mention;
	}

	bool corefersWith(CorefItem &other) {
		//return true if corefIDs or prolinkIDs agree
		return (corefID != NO_ID   && other.corefID == corefID) || 
			   (prolinkID != NO_ID && other.prolinkID == prolinkID);
	}

	bool hasID() {
		return (corefID != NO_ID || prolinkID != NO_ID);
	}

	void setID(int id) {
		//corefIDs are always <100, prolinkIDs are always >100
		if(id < 100)
			corefID = id;
		else prolinkID = id;
	}

	Mention *mention;
	SynNode *node;
	int corefID;
	int prolinkID;
};

#endif
