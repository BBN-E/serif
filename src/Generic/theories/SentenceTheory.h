// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENTENCE_THEORY_H
#define SENTENCE_THEORY_H

#include "Generic/driver/Stage.h"
#include "Generic/theories/Theory.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/theories/Sentence.h"

#include <iostream>
#include <set>

class SentenceSubtheory;

class TokenSequence;
class PartOfSpeechSequence;
class NameTheory;
class NestedNameTheory;
class Parse;
class MentionSet;
class ActorMentionSet;
class ValueMentionSet;
class PropositionSet;
class EntitySet;
class RelMentionSet;
class EventMentionSet;
class NPChunkTheory;
class DependencyParseTheory;

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED SentenceTheory : public Theory {
public:
	SentenceTheory(const Sentence *sentence, Symbol primaryParse, Symbol docID);
	SentenceTheory(const SentenceTheory &other);

	~SentenceTheory();


	/** Get score for entire sentence theory based on scores for
	 *  individual subtheories. */
	float getScore() const;


	/** The different types of subtheories. Each SentenceTheory has
	* (at most) 1 SentenceSubtheory of each of these types. New 
	* subtheories can go at the end of the list to keep state files
	* backwards compatible. */
	enum SubtheoryType {
		TOKEN_SUBTHEORY = 0,
		POS_SUBTHEORY,
		NAME_SUBTHEORY,
		NESTED_NAME_SUBTHEORY,
		VALUE_SUBTHEORY,
		NPCHUNK_SUBTHEORY,
		PARSE_SUBTHEORY,
		MENTION_SUBTHEORY,
		PROPOSITION_SUBTHEORY,
		ENTITY_SUBTHEORY,
		RELATION_SUBTHEORY,
		EVENT_SUBTHEORY,
		DEPENDENCY_PARSE_SUBTHEORY,
		ACTOR_MENTION_SET_SUBTHEORY // if you add to end of this list, modify N_SUBTHEORY_TYPES below
	};
	static const int N_SUBTHEORY_TYPES = ACTOR_MENTION_SET_SUBTHEORY + 1;


	/** Take on a SentenceSubtheory. If we already have a subtheory
	* of the given type, the old theory goes away.  If a subtheory
	  * is replaced, then any pointers from other subtheories to the
	  * replaced subtheory are changed to point to the new subtheory.
	  * In particular, if a SentenceTheory's MentionSet is changed,
	  * then its PropositionSet's pointer to the mention set is updated
	  * to point to the new value.  
	  *
	  * Some replacements are currently diallowed.  For example, you may 
	  * not replace the TokenSequence for a sentence once its parse is 
	  * set. */
	void adoptSubtheory(SubtheoryType type,
						SentenceSubtheory *newSubtheory);

	SentenceSubtheory *getSubtheory(int i) const;

	TokenSequence *   getTokenSequence() const;
	PartOfSpeechSequence* getPartOfSpeechSequence() const;

	NameTheory *      getNameTheory() const;
	NestedNameTheory* getNestedNameTheory() const;

	Parse *           getFullParse() const;	
	Parse *           getPrimaryParse() const;		//the primary parse is the parse that mentions refer to	
	Parse *           getNPChunkParse() const;
	Parse *           getDependencyParse() const;	

	MentionSet *            getMentionSet() const;
	ValueMentionSet *       getValueMentionSet() const;
	PropositionSet *        getPropositionSet() const;
	EntitySet *             getEntitySet() const;
	RelMentionSet *         getRelMentionSet() const;
	EventMentionSet *       getEventMentionSet() const;
	NPChunkTheory *	        getNPChunkTheory() const;
	DependencyParseTheory * getDependencyParseTheory() const;
	ActorMentionSet *       getActorMentionSet() const;
	Symbol			        getDocID() const;

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	SentenceTheory(StateLoader *stateLoader,
				   const Sentence *sentence);
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit SentenceTheory(SerifXML::XMLTheoryElement elem, const Sentence *sentence, const Symbol &_docID);
	const wchar_t* XMLIdentifierPrefix() const;



	void dump(std::ostream &out, int indent = 0) const;
	friend std::ostream &operator <<(std::ostream &out,
									 const SentenceTheory &it)
		{ it.dump(out, 0); return out; }

	void setLexicon(Lexicon* lex){_lex = lex;};
	Lexicon* getLexicon() {return _lex;};
	static bool isValidPrimaryParseValue(Symbol p) { return (p == Symbol(L"full_parse")) ||(p == Symbol(L"npchunk_parse"));};
	Symbol getPrimaryParseSym() const { return _primary_parse; }
	int getSentNumber() const { return _sentence->getSentNumber(); }

private:
	void setPrimaryParse(Symbol primary_parse_symbol);
	static std::set<Symbol> _temporalWords;
	float scoreParseForBadTemporalAttachments(Parse* p) const;

	//keep a pointer to the lexicon here, since token theories will point to it
	Lexicon* _lex;	
	const Sentence *_sentence;
	SentenceSubtheory *_subtheories[N_SUBTHEORY_TYPES];
	Symbol _primary_parse;
	Symbol _docID;
};

#endif

