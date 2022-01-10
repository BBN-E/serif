// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_METONYMY_ADDER_H
#define xx_METONYMY_ADDER_H

#include "Generic/metonymy/MetonymyAdder.h"


class MentionSet;
class PropositionSet;
class DocTheory;

class GenericMetonymyAdder : public MetonymyAdder {
private:
	friend class GenericMetonymyAdderFactory;

public:

	void resetForNewSentence() {}
	void resetForNewDocument(DocTheory *docTheory) {}

	virtual void addMetonymyTheory(const MentionSet *mentionSet,
				                   const PropositionSet *propSet)
	{ 
		return; 
	}

private:
	GenericMetonymyAdder() {}
};

class GenericMetonymyAdderFactory: public MetonymyAdder::Factory {
	virtual MetonymyAdder *build() { return _new GenericMetonymyAdder(); }
};


#endif
