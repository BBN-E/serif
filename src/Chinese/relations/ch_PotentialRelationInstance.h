// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_POTENTIAL_RELATION_INSTANCE_H
#define CH_POTENTIAL_RELATION_INSTANCE_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
#include "Generic/relations/PotentialRelationInstance.h"
class Argument;
class Proposition;
class MentionSet;
#include <string>

#define CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE + 5


class ChinesePotentialRelationInstance : public PotentialRelationInstance {
public:
	ChinesePotentialRelationInstance();
	ChinesePotentialRelationInstance(const ChinesePotentialRelationInstance &other);

	virtual void setStandardInstance(RelationObservation *obs) { PotentialRelationInstance::setStandardInstance(obs); }
	void setStandardInstance(Argument *first, 
		Argument *second, const Proposition *prop, const MentionSet *mentionSet);
	void setStandardListInstance(Argument *first, 
		Argument *second, bool listIsFirst, const Mention *member, const Proposition *prop, 
		const MentionSet *mentionSet);
	void setStandardNestedInstance(Argument *first, Argument *intermediate, 
		Argument *second, const Proposition *outer_prop, const Proposition *inner_prop,
		const MentionSet *mentionSet);

	void setFromTrainingNgram(Symbol *ngram);
	Symbol* getTrainingNgram();

	Symbol getLastHanziOfPredicate();
	Symbol getLeftMentionType();
	Symbol getRightMentionType();
	Symbol getLeftMetonymy();
	Symbol getRightMetonymy();

	std::wstring toString();
	std::string toDebugString();

	void setLastHanziOfPredicate(Symbol sym);
	void setLeftMentionType(Symbol sym);
	void setRightMentionType(Symbol sym);
	void setLeftMentionType(Mention::Type type);
	void setRightMentionType(Mention::Type type);
	void setLeftMetonymy(Symbol sym);
	void setRightMetonymy(Symbol sym);

private:
	Symbol _ngram[CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE];

	Symbol convertMentionType(Mention::Type type);
};


#endif
