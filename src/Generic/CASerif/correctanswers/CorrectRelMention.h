// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CORRECT_REL_MENTION_H
#define CORRECT_REL_MENTION_H

#include "common/Symbol.h"
#include "common/Sexp.h"
#include "theories/Mention.h"
#include "theories/ValueMentionSet.h"
#include "Generic/CASerif/correctanswers/CASymbolicConstants.h"

class CorrectDocument;
class CorrectRelation;

class CorrectRelMention 
{

public:

	CorrectRelMention();
	~CorrectRelMention();

	void loadFromSexp(Sexp *mentionSexp);
	//void loadFromERE(ERE *mentionERE);
	bool setMentionArgs(MentionSet *mentionSet, CorrectDocument *correctDocument);
	void setTimeArg(ValueMentionSet *valueMentionSet, CorrectDocument *correctDocument);
	void setCorrectRelation(CorrectRelation *correctRelation) { _relation = correctRelation; }

	Symbol getType(); 
	const Mention *getLeftMention() { return _lhs; }
	const Mention *getRightMention() { return _rhs; }
	const ValueMention *getTimeArg() { return _timeArg; }
	Symbol getTimeRole() { return _timeRole; }

	int getSentenceNumber() { return _sentence_number; }
	
	void dump(UTF8OutputStream &out, int indent) {
		out << L"(" << _lhs->getIndex() << L" " << _rhs->getIndex() << L" ";
		if (_timeArg != 0) {
			out << _timeRole.to_string() << L" ";		
			out << _timeArg->getUID().toInt() << L" ";
		}
		else
			out << L"-1 -1 ";
		out << getType().to_string() << L")";
	}

private:
	int _sentence_number;

	// Argument mentions:
	Symbol _leftAnnotationID, _rightAnnotationID;
	const Mention *_lhs, *_rhs;
	Symbol _timeAnnotationID;
	const ValueMention *_timeArg;
	Symbol _timeRole;

	Symbol _annotationID;

	CorrectRelation *_relation;

	bool unifyAppositives();
};


#endif
