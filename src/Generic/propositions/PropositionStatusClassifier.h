// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROPOSITION_STATUS_CLASSIFIER_H
#define PROPOSITION_STATUS_CLASSIFIER_H

#include <boost/shared_ptr.hpp>


class PropositionSet;
class SynNode;
class MentionSet;
class DocTheory;

/** Identify and label propositions that might not be true because of the
  * context that they occur in (such as a hypothetical or a negative). */
class PropositionStatusClassifier {
public:
	/** Create and return a new PropositionStatusClassifier. */
	static PropositionStatusClassifier *build() { return _factory()->build(); }
	/** Hook for registering new PropositionStatusClassifier factories */
	struct Factory { virtual PropositionStatusClassifier *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual void augmentPropositionTheory(DocTheory *docTheory) = 0;
	virtual ~PropositionStatusClassifier() {}
protected:
	PropositionStatusClassifier() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};

//#if defined(ENGLISH_LANGUAGE)
//	#include "English/propositions/en_PropositionStatusClassifier.h"
//#else
//	#include "Generic/propositions/xx_PropositionStatusClassifier.h"
//#endif


#endif
