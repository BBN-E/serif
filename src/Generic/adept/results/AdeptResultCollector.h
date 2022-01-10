// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ADEPTRESULTCOLLECTOR_H
#define ADEPTRESULTCOLLECTOR_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/hash_set.h"
#include "Generic/theories/Mention.h"
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
class AdeptResultCollector: public ResultCollector {
public:
	// just construct with the doc theory
	// and info will be extracted
	AdeptResultCollector();
	~AdeptResultCollector();

	virtual void loadDocTheory(DocTheory *docTheory);

	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t *document_filename);

	const wchar_t * retrieveOutput();
private:

	/***** utility ADEPT methods *****/
	void loadRelationTypeNameMap();
	void loadRelationFilter();
	void appendBeginDocTag(const wchar_t* file);
	void appendEndDocTag();
	
	bool AdeptResultCollector::isValidProposition(const Proposition *proposition, const int sentence_num);
	void appendEntity(const Entity *entity);
	void appendExtraEntity(int uid, Mention *mention, int sentence_num);
	void appendMention(const Mention *mention, const Entity *entity, int uid=-1);
	void appendRelMention(RelMention *relMention,
						  const Relation *relation);
	void appendVerbRelation(const Proposition *proposition,
							const int uid,
							const int sentenceNumber);
	void appendVerbRelationSlots(const Proposition *proposition,
								 const int uid,
								 const int sentenceNumber);
	void appendSlots(RelMention *relMention,
					 const Relation *relation);
	void appendNodeText(const SynNode* node, const TokenSequence *tokens);
	void appendSentence(const Sentence* sentence);
	
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

	int obtainRelMentionUID(const RelMention *mention, int suggestedUID = -1);
	int obtainEntityMentionUID(const Mention *mention, int suggestedUID = -1);
	int obtainSentenceUID(int suggestedUID = -1);
	int obtainSlotUID(int suggestedUID = -1);

	const Mention *fixAppositiveCase(const Mention *mention);
	Mention::Type fixPartitiveCase(Mention::Type type);

	void resetResultString();
	void createResultString();

	void appendEscapedToResultString(const wchar_t *str);
	void appendToResultString(const wchar_t *str);
	void appendToResultString(const char *str);
	void appendToResultString(int num);

	wchar_t *_resultString;
	size_t _maxResultString;
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

	typedef hash_map <SymArray*, SymArray*, HashKey, EqualKey> 
		SymArrayMap;
	typedef hash_map <SymArray*, bool, HashKey, EqualKey> 
		SymArraySet;

	SymArrayMap *_relationTypeMap;
	SymArraySet *_relationFilter;
};

#endif
