// Copyright 2017 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CAUSE_EFFECT_RELATION_H
#define CAUSE_EFFECT_RELATION_H

#include "Generic/common/BoostUtil.h"

#include <boost/shared_ptr.hpp>
#include <string>


class Proposition;
class SentenceTheory;

class CauseEffectRelation {

public:
	~CauseEffectRelation() { }

	const Proposition* getCause();
	const SentenceTheory* getCauseSentenceTheory();

	const Proposition* getEffect();
	const SentenceTheory* getEffectSentenceTheory();

	const Symbol getRelationType();

	std::wstring getJsonString(Symbol docid);


private:
	CauseEffectRelation(const Proposition *cause, const Proposition *effect, const SentenceTheory *sentTheory, Symbol relationType);
	BOOST_MAKE_SHARED_4ARG_CONSTRUCTOR(CauseEffectRelation, const Proposition*, const Proposition*, const SentenceTheory*, const Symbol);

	const Proposition *_cause;
	const SentenceTheory *_causeSentenceTheory;

	const Proposition *_effect;
	const SentenceTheory *_effectSentenceTheory;

	const Symbol _relationType;

	// Helper functions
	int getPropStartOffset(const Proposition* prop);
	int getPropEndOffset(const Proposition* prop);
	std::wstring escapeJsonString(const std::wstring& input);
};


typedef boost::shared_ptr<CauseEffectRelation> CauseEffectRelation_ptr;

#endif

