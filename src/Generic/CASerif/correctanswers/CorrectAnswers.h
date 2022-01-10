// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CORRECT_ANSWERS_H
#define CORRECT_ANSWERS_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/Sentence.h"
#include "Generic/parse/Constraint.h"

#include "Generic/edt/MentionLinker.h"


const int MAX_NAMES_IN_SENTENCE = 200;

#define CORRECT_ANSWER_PARAM_BUFFER_SIZE 2048

class CorrectAnswers
{

private:	
	CorrectAnswers();
	~CorrectAnswers();

	CorrectAnswers(const CorrectAnswers&);  // undefined copy constructor
	CorrectAnswers& operator=(const CorrectAnswers&); // undefined equals operator
public:

	static CorrectAnswers& getInstance();

	void resetForNewDocument();

	int getNameTheories(NameTheory **results, TokenSequence *tokenSequence, Symbol docName);
	int getNestedNameTheories(NestedNameTheory **results, TokenSequence *tokenSequence, Symbol docName, NameTheory *nameTheory);
	int getValueTheories(ValueMentionSet **results, TokenSequence *tokenSequence, Symbol docName);
	int correctMentionTheories(MentionSet *results[], int sentence_number,
							   TokenSequence *tokenSequence, Symbol docName, bool use_correct_types);
	EntitySet *getEntityTheory(const MentionSet *mentionSet, 
		size_t n_sentences, Symbol docName);
	CorrectDocument* getCorrectDocument(Symbol docname);
	void adjustParseScores(Parse *results[], size_t n_parses, TokenSequence *tokenSequence, Symbol docName);
	void setSentAndTokenNumbersOnCorrectMentions(TokenSequence *tk, Symbol docName);
	std::vector<Constraint> getConstraints(TokenSequence *tokenSequence, 
									Symbol docName);
	std::vector<CharOffset> getMorphConstraints(const Sentence *sentence, 
									Symbol docName);
	void hookUpCorrectValuesToSystemValues(int sentno,
		ValueMentionSet *valueMentionSet, Symbol docName);
	void assignCorrectTimexNormalizations(DocTheory* docTheory);
	void assignSynNodesToCorrectMentions(int sentence_number, Parse *parse, Symbol docName);
	const SynNode *getBestNodeMatch(CorrectMention *correctMention, const SynNode *root);
	const SynNode *getBestNodeHeadMatch(CorrectMention *correctMention, const SynNode *node);
	const SynNode *getBestNodeHeadTokenMatch(CorrectMention *correctMention, const SynNode *node);

	static bool isWithinTokenSequence(TokenSequence *tokenSequence, EDTOffset start, EDTOffset end);
	static bool isWithinTokenSequence(TokenSequence *tokenSequence, CorrectMention *correctMention);
	static bool findTokens(TokenSequence *tokenSequence, EDTOffset s_offset, EDTOffset e_offset,
							  int& s_token, int& e_token);

	RelMentionSet *correctRelationTheory(const Parse *parse, MentionSet *mentionSet,
			                         EntitySet *entitySet, const PropositionSet *propSet, 
									 ValueMentionSet *valueMentionSet, Symbol docName);
	EventMentionSet *correctEventTheory(TokenSequence *tokenSequence,
		ValueMentionSet *valueMentionSet, const Parse *parse, MentionSet *mentionSet, 
		PropositionSet *propSet, Symbol docName);

	Symbol getEventID(EventMention *em, Symbol docName);

	bool usingCorrectRelations() { return _use_correct_relations; }
	bool usingCorrectEvents() { return _use_correct_events; }
	bool usingCorrectCoref() { return _use_correct_coref; }
	bool usingCorrectSubtypes() { return _use_correct_subtypes; }
	bool usingCorrectTypes() { return _use_correct_types; }

private:
	bool _debug;
	UTF8OutputStream _debugStream;
	
	int _n_documents;
	CorrectDocument *_documents;

	EntitySet *_documentEntitySet;

	bool _matched;
	bool _exact_match;
	bool _too_large_match;
	
	bool _use_correct_relations;
	bool _use_correct_coref;
	bool _use_correct_events;
	bool _use_correct_subtypes;
	bool _use_correct_types;
	bool _unify_appositives;
	bool _add_ident_relations;

	std::set<Symbol> _eventTypesAllowUndet;
	
	// for storing mentions as we put them in the SentenceTheory
	EntityType *_typeBuffer[MAX_NAMES_IN_SENTENCE];
	CorrectMention *_mentionBuffer[MAX_NAMES_IN_SENTENCE];
	int _num_mentions_in_buffer;

	float getNumberOfMatchedCorrectMentions(Parse *parse, TokenSequence *tokenSequence, Symbol docName);
	void checkIfNodeMatches(const SynNode *node, CorrectMention *cm);

};


#endif
