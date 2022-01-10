// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef DISTILL_EXPANDER_H
#define DISTILL_EXPANDER_H

#include <boost/make_shared.hpp>
#include "Generic/PropTree/PropNode.h"
#include "Generic/PropTree/expanders/SequenceExpander.h"

// DistillationDocExpander and DistillationQueryExpander are identical with the following two exceptions
// 1) DistillationDocExpander does not do wordnet expansion, because this is symmetric and only needs to be done on one side
// 2) DistillationQueryExpander does not do mention expansion, because this only makes sense when there is coreference

class DistillationDocExpander : public SequenceExpander {
public:
	DistillationDocExpander();
private:
	static float BOOST_MEMBER_PROB;
	static float BOOST_UNKNOWN_PROB;
	static float BOOST_POSS_PROB;
	static float BOOST_OF_PROB;
	static float WN_STEM_PROB;
	static float PORTER_STEM_PROB;
};

class DistillationQueryExpander : public SequenceExpander {
public:
	typedef std::map<std::wstring,double> NameSynonyms;
	typedef std::map<std::wstring,NameSynonyms> NameDictionary;

	DistillationQueryExpander(const NameDictionary &equivNames);


private:
	static float BOOST_MEMBER_PROB;
	static float BOOST_UNKNOWN_PROB;
	static float BOOST_POSS_PROB;
	static float BOOST_OF_PROB;
	static float WN_STEM_PROB;
	static float WN_SYNONYM_PROB;
	static float PORTER_STEM_PROB;
	static float NOMINALIZATION_PROB;
	static float WN_HYPERNYM_PROB;
	static float WN_HYPONYM_PROB;
	static float EQUIV_NAME_THRESHOLD;
};

#endif

