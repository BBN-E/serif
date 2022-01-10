// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HY_RELATION_INSTANCE_H
#define HY_RELATION_INSTANCE_H

#include "Generic/relations/discmodel/DTRelSentenceInfo.h"

class HYRelationInstance {
public:
	HYRelationInstance()
		: _index1(-1), _index2(-1), _sentenceInfo(NULL), _margin(0), _invalid(false) {
	}

	HYRelationInstance(int index1, int index2, 
		DTRelSentenceInfo *sentenceInfo, 
		Symbol docID = Symbol(L"somedoc")) 
		: _margin(0), _invalid(false) {
		populate(index1, index2, sentenceInfo, docID);
	}

	void populate(int index1, int index2, 
		DTRelSentenceInfo *sentenceInfo, Symbol docID);
	
	Symbol getID() const {
		return _uid;
	}

	Symbol getDocID() const {
		return _docID;
	}

	bool isReversed() const {
		return _sentenceInfo->relSets[0]->hasReversedRelation(_index1, _index2);
	}

	bool isValid() const {
		return !_invalid;
	}

	// use to indicate a bad EDT or a bad pronoun
	void invalidate() {
		_invalid = true;
	}

	bool operator<(HYRelationInstance& other) const {
		return _margin < other._margin;
	}

	Symbol getRelationSymbol() const {
		Symbol decodedSym = _sentenceInfo->relSets[0]->getRelation(_index1, _index2);
		if (_annoSym.is_null())
			return decodedSym;
		else
			return _annoSym;
	}

	Symbol getAnnotationSymbol() const {
		return _annoSym;
	}

	void setAnnotationSymbol(Symbol sym) {
		_annoSym = sym;
	}

	MentionSet const * getMentionSet() const {
		return _sentenceInfo->mentionSets[0];
	}

	DTRelSentenceInfo* getSentenceInfo() const {
		return _sentenceInfo;
	}

	int getFirstIndex() const {
		return _index1;
	}

	int getSecondIndex() const {
		return _index2;
	}

	Mention* getFirstMention() const {
		return _sentenceInfo->mentionSets[0]->getMention(_index1);
	}

	Mention* getSecondMention() const {
		return _sentenceInfo->mentionSets[0]->getMention(_index2);
	}

public:  // public data for the use of other classes
	double _margin; 

private:
	int _index1, _index2;  // indices into the MentionSet owned by our DTRelSentenceInfo
	bool _invalid;
	DTRelSentenceInfo *_sentenceInfo;
	Symbol _annoSym;
	Symbol _uid;  // should be unique for every HYRelationInstance
	Symbol _docID;
};

#endif
