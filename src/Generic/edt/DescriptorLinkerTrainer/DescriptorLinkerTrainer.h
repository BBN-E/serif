// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCRIPTOR_LINKER_TRAINER_H
#define DESCRIPTOR_LINKER_TRAINER_H

#include "Generic/trainers/AnnotatedParseReader.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/descriptors/ClassifierTreeSearch.h"
#include "Generic/descriptors/PartitiveClassifier.h"
#include "Generic/descriptors/AppositiveClassifier.h"
#include "Generic/descriptors/ListClassifier.h"
#include "Generic/descriptors/NestedClassifier.h"
#include "Generic/descriptors/PronounClassifier.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/OtherClassifier.h"
#include "Generic/maxent/OldMaxEntEvent.h"
#include "Generic/maxent/OldMaxEntModel.h"

class SynNode;
class Entity;
class EntitySet;

class DescriptorLinkerTrainer {
public:
	DescriptorLinkerTrainer(const char* output_file);
	~DescriptorLinkerTrainer();

	virtual void openInputFile(const char *inputFile);
	virtual void closeInputFile();
	
	// this is the major function. Using the given input file, extract the descriptor linking events.
	virtual void extractDescLinkEvents();

	virtual void train(char* output_file_prefix); 
	virtual void writeEvents(const char *file_prefix);

private:
	struct HashKey {
        size_t operator()(const int a) const {
			return a;
        }
	};

    struct EqualKey {
        bool operator()(const int a, const int b) const {
			return (a == b);
		}
	}; 

	typedef serif::hash_map<int, int, HashKey, EqualKey> IntegerMap;
	typedef GrowableArray<CorefItem *> CorefItemList;

	void processDocument(CorefDocument *document);
	void processNode(const SynNode* node);

	void findSentenceCorefItems(CorefItemList &items, const SynNode *node, CorefDocument *document);
	void addCorefItemsToMentionSet(MentionSet* mentionSet, CorefItemList &items, int ids[]);
	void processMentions(const SynNode* node, MentionSet *mentionSet);
	void processEntities(MentionSet *mentionSet, EntitySet *entitySet, int ids[]);
	void collectLinkEvents(Mention *ment, int id, EntitySet *entitySet);

	void getContext(Mention *ment, Entity *entity, EntitySet *entitySet, OldMaxEntEvent *e);

	void printModel(const char *file_prefix);

	// hacky method to find the largest mention number in an entity
	MentionUID _getLatestMention(Entity* ent);

	AnnotatedParseReader _inputSet;

	ClassifierTreeSearch _searcher;
	PartitiveClassifier _partitiveClassifier;
	AppositiveClassifier _appositiveClassifier;
	ListClassifier _listClassifier;
	NestedClassifier _nestedClassifier;
	PronounClassifier* _pronounClassifier;
	OtherClassifier _otherClassifier;

	IntegerMap *_alreadySeenIDs;
	GrowableArray<OldMaxEntEvent *> _primaryEvents;
	GrowableArray<OldMaxEntEvent *> _secondaryEvents;

	OldMaxEntModel *_primaryModel;
	OldMaxEntModel *_secondaryModel;
	int _cutoff;
	double _threshold;
}; 

#endif
