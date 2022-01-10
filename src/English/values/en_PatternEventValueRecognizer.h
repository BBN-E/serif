//Copyright 2011 BBN Technologies
//All rights reserved

#ifndef EN_PATTERN_EVENT_VALUE_RECOGNIZER_H
#define EN_PATTERN_EVENT_VALUE_RECOGNIZER_H

#include "Generic/values/PatternEventValueRecognizer.h"
#include "Generic/patterns/PatternSet.h"
#include <vector>

class EventMention;
class Mention;
class ValueMention;
class SynNode;

class EnglishPatternEventValueRecognizer: public PatternEventValueRecognizer {
private:
	friend class EnglishPatternEventValueRecognizerFactory;
public:
	~EnglishPatternEventValueRecognizer();
	void createEventValues(DocTheory* docTheory);
private:
	EnglishPatternEventValueRecognizer();
	//These are the sets of patterns used to identify values
	std::vector<PatternSet_ptr> _eventValuePatternSets;
	enum { CRIME, POSITION, SENTENCE };

	//This is the list of value mentions we make in createEventValues. At the end of that method, all of the ValueMentions will be given to the DocTheory.
	std::vector<ValueMention*> _valueMentions;

	//This method will create the ValueMention and add it to the vector of ValueMentions, avoiding duplicates
	void addValueMention(DocTheory *docTheory, EventMention *vment, const Mention *ment, std::wstring returnValue, float score);
	void addValueMention(DocTheory* docTheory, EventMention* vment, const SynNode* node, std::wstring returnValue, float score);
	void addValueMention(DocTheory* docTheory, EventMention* vment, int startToken, int endToken, std::wstring returnValue, float score);

	Symbol getBaseType(Symbol fullType);

	//Adds Position Value Mentions by looking for arguments that are Job Titles according to WordNet
	void addPosition(DocTheory* docTheory, EventMention *vment);
	//Looks up if a string is a Job Title according to WordNet
	bool isJobTitle(Symbol word);

	/*	I *think* this method does the following
	*	1) Find a proposition with a role, or that matches the proposition of the ValueMention we have already found for a Crime.
	*	2) Find an event mention from the sentence we are in that either has the proposotion from 1) as it's anchor, or has an argument that has the proposition from 1) as it's anchor.
	*	3) Add the Crime ValueMention to that EventMention 
	*	Who knows why we need to do this, but we're going to do it anyway!
	*/
	void transferCrime(EventMention *vment, class MentionSet *mentionSet, const class PropositionSet *propSet, class EventMentionSet* vmSet);

};


// RelationFinder factory
class EnglishPatternEventValueRecognizerFactory: public PatternEventValueRecognizer::Factory {
	virtual PatternEventValueRecognizer *build() { return _new EnglishPatternEventValueRecognizer(); } 
};
 
#endif
