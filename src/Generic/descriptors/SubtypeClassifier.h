// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SUBTYPE_CLASSIFIER_H
#define SUBTYPE_CLASSIFIER_H

#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/theories/EntitySubtype.h"
class PartOfSpeechSequence;

class SubtypeClassifier : public MentionClassifier {
public:
	SubtypeClassifier();
	virtual ~SubtypeClassifier();

	/**
	 * The ClassifierTreeSearch (branchable) way to classify.
	 * The classifier uses a combination of prob models that focus on head word and
	 * functional parent head to pick a type (or OTH
	 * if no type can be found)
	 * @param currSolution The extant mentionSet
	 * @param currMention The mention to classify
	 * @param results The resultant mentionSet(s)
	 * @param max_results The largest possible size of results
	 * @param isBranching Whether we should bother with forking and branching or just overwrite
	 * @return the number of elements in results (which is used by the method calling this
	 */
	virtual int classifyMention (MentionSet *currSolution, Mention *currMention,
		MentionSet *results[], int max_results, bool isBranching);
	/*hacky way to use part of speech for PartOfSpeechSequence to determine subtype
		(Arabic Paser POS doesn't use number, number is used to set PER.group
		the part of speech sequence should get passed to all MentionClassifiers, but
		I don't have time for that now
	*/
	void setPOSSequence(const PartOfSpeechSequence* pos){_posSequence = pos;};

private:
	struct EntitySubtypeItem
	{
		EntitySubtype entitySubtype;
		EntitySubtypeItem *next;
	};

	void classifyName(Mention *currMention);
	void classifyDesc(Mention *currMention);
	void classifyList(Mention *currMention);
	void classifyAppo(Mention *currMention);
	void classifyPart(Mention *currMention);

	const PartOfSpeechSequence* _posSequence;

	// define hash_map mapping DTFeatures to floats
	struct HashKey {
        size_t operator()(const Symbol& s) const {
            return s.hash_code();
        }
    };
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const {
            return s1 == s2;
        }
    };
	typedef serif::hash_map<Symbol, EntitySubtypeItem *, HashKey, EqualKey>
		WordToSubtypeMap;
	
	WordToSubtypeMap *_wordnetMap;
	WordToSubtypeMap *_descHeadwordMap;
	WordToSubtypeMap *_nameWordMap;
	WordToSubtypeMap *_fullNameMap;
	WordToSubtypeMap *_wikiWordMap;

	EntitySubtype* lookupSubtype(WordToSubtypeMap *map, Symbol sym, EntityType type);
	void deleteItems(WordToSubtypeMap *map);
};

#endif
