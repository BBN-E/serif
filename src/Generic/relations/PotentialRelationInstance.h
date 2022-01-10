// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef POTENTIAL_RELATION_INSTANCE_H
#define POTENTIAL_RELATION_INSTANCE_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Mention.h"
class Argument;
class Proposition;
class MentionSet;
class MentionSet;
class Mention;
class RelationObservation;
class TokenSequence;
#include <string>

#define POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE 13


class PotentialRelationInstance {
public:

	PotentialRelationInstance();
	~PotentialRelationInstance() { };
	PotentialRelationInstance(const PotentialRelationInstance &other);

	virtual void setStandardInstance(RelationObservation *obs);
	void setStandardInstance(Mention *first, Mention *second, 
		const TokenSequence *tokenSequence);

	void setStandardInstance(Argument *first, 
		Argument *second, const Proposition *prop, const MentionSet *mentionSet);
	virtual void setStandardListInstance(Argument *first, 
		Argument *second, bool listIsFirst, const Mention *member, const Proposition *prop, 
		const MentionSet *mentionSet);
	virtual void setStandardNestedInstance(Argument *first, Argument *intermediate, 
		Argument *second, const Proposition *outer_prop, const Proposition *inner_prop,
		const MentionSet *mentionSet);

	// this one is only used in English, but it's not hurting anybody being here
	void setPartitiveInstance(Symbol top_headword, Symbol bottom_headword, Symbol entity_type);

	virtual void setFromTrainingNgram(Symbol *ngram);
	virtual Symbol* getTrainingNgram() { return _ngram; }

	Symbol getRelationType();
	Symbol getPredicate();
	Symbol getStemmedPredicate();
	Symbol getLeftHeadword();
	Symbol getRightHeadword();
	Symbol getNestedWord();
	Symbol getLeftEntityType();
	Symbol getRightEntityType();
	Symbol getLeftRole();
	Symbol getRightRole();
	Symbol getNestedRole();
	bool isReversed();
	void setReverse(bool reverse);

	MentionUID getLeftMention();
	MentionUID getRightMention();
		
	void setRelationType(Symbol sym);
	bool isNested();
	bool isOnePlacePredicate();
	bool isMultiPlacePredicate();
	Symbol getPredicationType();

	virtual std::wstring toString();
	virtual std::string toDebugString();

	void setLeftEntityType(Symbol sym);
	void setRightEntityType(Symbol sym);
	void setPredicate(Symbol sym);
	void setStemmedPredicate(Symbol sym);
	void setLeftHeadword(Symbol sym);
	void setRightHeadword(Symbol sym);
	void setNestedWord(Symbol sym);
	void setLeftRole(Symbol sym);
	void setRightRole(Symbol sym);
	void setNestedRole(Symbol sym);
	void setReverse(Symbol sym);
	void setPredicationType(Symbol sym);

	static Symbol NULL_SYM;
	static Symbol CONFUSED_SYM;

protected:
	Symbol _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE];
	MentionUID _leftMention;
	MentionUID _rightMention;

	static Symbol ONE_PLACE;
	static Symbol MULTI_PLACE;
	static Symbol REVERSED_SYM;
	static Symbol NONE_SYM;
	static Symbol PARTITIVE_TOP;
	static Symbol PARTITIVE_BOTTOM;
	static Symbol SET_SYM;
	static Symbol COMP_SYM;

	Symbol getReverse();

};


#endif
