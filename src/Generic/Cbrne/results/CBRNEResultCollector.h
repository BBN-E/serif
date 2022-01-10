// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CBRNERESULTCOLLECTOR_H
#define CBRNERESULTCOLLECTOR_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/hash_set.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Event.h"
#include "Generic/results/ResultCollector.h"

class DocTheory;
class Entity;
class EntitySet;
class Sentence;
class RelationSet;
class Relation;
class RelMention;
class SynNode;
class Proposition;
class TokenSequence;


// acquires all relevant entity, mention, relation, etc. data
// and provides various ways to deliver it
class CBRNEResultCollector: public ResultCollector {
public:
	// just construct with the doc theory
	// and info will be extracted
	CBRNEResultCollector();
	~CBRNEResultCollector();

	virtual void loadDocTheory(DocTheory *docTheory);

	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t *document_filename);

	virtual void produceOutput(std::wstring *results);
	void produceOutput(OutputStream &str);


private:

	/***** utility CBRNE methods *****/
	void loadRelationTypeNameMap();
	void loadRelationFilter();
	void appendBeginDocTag(OutputStream &stream, const wchar_t* file);
	void appendEndDocTag(OutputStream &stream);
	
	bool isValidProposition(const Proposition *proposition, const int sentence_num);
	void appendEvent(OutputStream &stream, Event *e);
	void appendEventMention(OutputStream &stream, EventMention *em, Event *e);
	void appendEventSlots(OutputStream &stream, Event *e);
	void appendEventMentionSlots(OutputStream &stream, EventMention *em);
	void appendEntity(OutputStream &stream, const Entity *entity);
	void appendExtraEntity(OutputStream &stream, int uid, Mention *mention, int sentence_num);
	void appendMention(OutputStream &stream, const Mention *mention, const Entity *entity, int uid=-1);
	void appendRelMention(OutputStream &stream, 
						  RelMention *relMention,
						  const Relation *relation);
	void appendVerbRelation(OutputStream &stream, 
							const Proposition *proposition,
							const RelMentionUID uid,
							const int sentenceNumber);
	void appendVerbRelationSlots(OutputStream &stream, 
								 const Proposition *proposition,
								 const RelMentionUID uid,
								 const int sentenceNumber);
	void appendSlots(OutputStream &stream, 
					 RelMention *relMention,
					 const Relation *relation);
	void appendNodeText(OutputStream &stream, const SynNode* node, const TokenSequence *tokens);
	void appendSentence(OutputStream &stream, const Sentence* sentence);
	
	//bool isUnknownMentionCandidate(const Mention *mention);
	//bool isFilteredUnknownMention(const Mention *mention);
	//bool isValidKnownMention(Mention *mention);

	bool isValidRelationMention(const RelMention *mention);
	bool isValidEntityMention(const Mention *mention);
	bool isValidEntity(const Entity *entity);
	bool isLinkedEntityMention(const Mention *mention);
	bool isUnlinkedTypedEntityMention(const Mention *mention);
	bool isUnlinkedUntypedEntityMention(const Mention *mention);
	bool isTypedEntityMention(const Mention *mention);

	int obtainEventMentionUID(const EventMention *em, EventMentionUID suggestedUID);
	int obtainEventUID(const Event *e, int suggestedUID);
	
	int obtainRelMentionUID(const RelMention *mention, RelMentionUID suggestedUID);
	int obtainEntityMentionUID(const Mention *mention, MentionUID suggestedUID);
	int obtainSentenceUID(int suggestedUID);
	int obtainSlotUID(int suggestedUID);

	const Mention *fixAppositiveCase(const Mention *mention);
	Mention::Type fixPartitiveCase(Mention::Type type);

	void appendEscaped(OutputStream &stream, const wchar_t *str);

	DocTheory *_docTheory;

	//MEMORY: CREATES-OWN-COPY, DELETES-OWN-COPY
	class SymArray {
	public:
		Symbol *array;
		int length;
		SymArray(): length(0), array(0) { }
		SymArray(Symbol array_[], int length_) {
			length = length_;
			array = _new Symbol[length];
			for(int i=0; i<length; i++)
				array[i] = array_[i];
		}
		SymArray(SymArray &other) {
			length = other.length;
			array = _new Symbol[length];
			for(int i=0; i<length; i++)
				array[i] = other.array[i];
		}

		~SymArray() {
			delete[] array;
		}
	};

	struct HashKey {
        size_t operator()(const SymArray *s) const {
            size_t val = 0;
            for (int i = 0; i < s->length; i++)
                val = (val << 2) + s->array[i].hash_code();
            return val;
        }
	};

    struct EqualKey {
        bool operator()(const SymArray *s1, const SymArray *s2) const {
			if(s1->length != s2->length)
				return false;

            for (int i = 0; i < s1->length; i++) {
                if (s1->array[i] != s2->array[i]) {
                    return false;
                }
            }
            return true;
        }
	}; 

	typedef serif::hash_map <SymArray*, SymArray*, HashKey, EqualKey>
		SymArrayMap;
	typedef serif::hash_map <SymArray*, bool, HashKey, EqualKey>
		SymArraySet;

	SymArrayMap *_relationTypeMap;
	SymArraySet *_relationFilter;
};

#endif
