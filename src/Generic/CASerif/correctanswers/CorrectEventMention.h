// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CORRECT_EVENT_MENTION_H
#define CORRECT_EVENT_MENTION_H

#include "common/Symbol.h"
#include "common/Sexp.h"
#include "theories/Mention.h"
#include "theories/ValueMention.h"
#include "theories/Token.h"
#include "Generic/CASerif/correctanswers/CASymbolicConstants.h"

class CorrectDocument;
class CorrectEvent;

class CorrectEventMention 
{
public:

	CorrectEventMention();
	~CorrectEventMention();

	struct CorrectEventArgument {
		Symbol role;
		Symbol annotationID;
		const Mention *mention;
		const ValueMention *valueMention;
	};

	void loadFromSexp(Sexp *mentionSexp);
	//void loadFromERE(ERE *mentionERE);
	void setMentionArgs(MentionSet *mentionSet, CorrectDocument *correctDocument, const std::set<Symbol> &eventTypesAllowUndet);
	void setValueArgs(ValueMentionSet *valueMentionSet, CorrectDocument *correctDocument);
	void setCorrectEvent(CorrectEvent *correctEvent) { _event = correctEvent; }

	Symbol getType(); 

	int getSentenceNumber() { return _sentence_number; }
	int getNArgs() { return _n_args; }
	Symbol getNthRole(int n) { return _args[n].role; }
	const Mention *getNthMention(int n) { return _args[n].mention; }
	const ValueMention *getNthValueMention(int n) { return _args[n].valueMention; }
	EDTOffset getAnchorStart() { return _anchor_start; }
	EDTOffset getAnchorEnd() { return _anchor_end; }
	Symbol getAnnotationID() { return _annotationID; }
	
	void dump(UTF8OutputStream &out, int indent) {
	}

private:
	int _sentence_number;

	EDTOffset _anchor_start;
	EDTOffset _anchor_end;
	EDTOffset _extent_start;
	EDTOffset _extent_end;

	Token *_anchor;
	Symbol _annotationID;
	CorrectEvent *_event;
	int _n_args;
	CorrectEventArgument *_args;

	bool unifyAppositives();

};


#endif
