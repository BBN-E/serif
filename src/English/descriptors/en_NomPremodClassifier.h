#ifndef EN_NOM_PREMOD_CLASSIFIER_H
#define EN_NOM_PREMOD_CLASSIFIER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/theories/EntityType.h"
#include "Generic/descriptors/discmodel/P1DescriptorClassifier.h"
#include "Generic/descriptors/NomPremodClassifier.h"

class SynNode;
class MentionSet;
class SymbolListMap;


class EnglishNomPremodClassifier : public NomPremodClassifier {
private:
	friend class EnglishNomPremodClassifierFactory;

public:

	~EnglishNomPremodClassifier();
	virtual void cleanup();

private:
	EnglishNomPremodClassifier();

	// returns the number of possibilities selected
	virtual int classifyNomPremod(
		MentionSet *currSolution, const SynNode* node, EntityType types[],
		double scores[], int maxResults);

	EntityType getTitleType(MentionSet *currSolution, const SynNode* node);


	P1DescriptorClassifier *_p1DescriptorClassifier;
	SymbolListMap *_certainNomPremods;

	bool _dout_open;
	/// Debugging output stream
	UTF8OutputStream _dout;
};

class EnglishNomPremodClassifierFactory: public NomPremodClassifier::Factory {
	virtual NomPremodClassifier *build() { return _new EnglishNomPremodClassifier(); }
};




#endif
