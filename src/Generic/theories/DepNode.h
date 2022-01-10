// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEP_NODE_H
#define DEP_NODE_H

#include "Generic/theories/SynNode.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED DepNode : public SynNode {

private:

	Symbol _word;
	Symbol _partOfSpeech;

	int _token_number;


public:
	DepNode(int ID, DepNode *parent, const Symbol &tag, int n_children, const Symbol &word, const Symbol &partOfSpeech, int token_number);
	DepNode(StateLoader *stateLoader);
	explicit DepNode(SerifXML::XMLTheoryElement elem, DepNode* parent, int &node_id_counter);

	void saveState(StateSaver *stateSaver) const;
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;

	const Symbol &getHeadWord() const;
	Symbol getPartOfSpeech() const;
	int getTokenNumber() const;
	Symbol getWord() const;

	void setID(int ID);

	
};

#endif
