// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCRIPTOR_RECOGNIZER_H
#define DESCRIPTOR_RECOGNIZER_H

#include "Generic/common/DebugStream.h"
#include "Generic/descriptors/ClassifierTreeSearch.h"
#include "Generic/descriptors/DescriptorClassifier.h"
#include "Generic/descriptors/NoneClassifier.h"
#include "Generic/descriptors/PartitiveClassifier.h"
#include "Generic/descriptors/AppositiveClassifier.h"
#include "Generic/descriptors/ListClassifier.h"
#include "Generic/descriptors/NestedClassifier.h"
#include "Generic/descriptors/PronounClassifier.h"
#include "Generic/descriptors/NomPremodClassifier.h"
#include "Generic/descriptors/SubtypeClassifier.h"
#include "Generic/theories/EntityType.h"
#include "Generic/names/discmodel/PIdFModel.h"

class Parse;
class MentionSet;
class NameTheory;
class NestedNameTheory;
class SynNode;
class PartOfSpeechSequence;
class DocTheory;


class DescriptorRecognizer {
public:
	DescriptorRecognizer(bool doStage);
	DescriptorRecognizer();
	~DescriptorRecognizer();

	void resetForNewSentence();
	void resetForNewDocument(DocTheory *docTheory);
	void cleanup();

	int getDescriptorTheories(MentionSet *results[],
	                          int max_theories,
	                          const PartOfSpeechSequence* partOfSpeechSeq,
	                          const Parse *parse,
	                          const NameTheory *nameTheory,
	                          const NestedNameTheory *nestedNameTheory,
	                          TokenSequence* tokenSequence,
	                          int sentence_number);

	void regenerateSubtypes(MentionSet *mset, const SynNode *node);

private:
	bool _doStage;
	ClassifierTreeSearch _searcher;
	DescriptorClassifier* _descriptorClassifier;
	PartitiveClassifier _partitiveClassifier;
	AppositiveClassifier _appositiveClassifier;
	ListClassifier _listClassifier;
	NestedClassifier _nestedClassifier;
	PronounClassifier* _pronounClassifier;
	NomPremodClassifier* _nomPremodClassifier;
	NoneClassifier _noneClassifier;
	SubtypeClassifier _subtypeClassifier;

	DocTheory *_docTheory;

	bool _nationalityDescriptorsTreatedAsNames;

	DebugStream _debug;
	bool _usePIdFDesc;
	bool _useNPChunkParse;
	PIdFModel *_pdescDecoder;

	bool _use_correct_answers;

	void processTheory(MentionSet* mentionSet, const Parse *parse, int branch);
	void processNode(const SynNode *node);
	void generateSubtypes(const SynNode *node);
	void reprocessNominalPremods(MentionSet *mentionSet, const SynNode *node);


	void putNameTheoryInMentionSet(const NameTheory *nameTheory,
										  MentionSet *mentionSet);
	void putNestedNameTheoryInMentionSet(const NestedNameTheory *nestedNameTheory,
										  MentionSet *mentionSet);
	void putDescTheoryInMentionSet(NameTheory *nameTheory,
										  MentionSet *mentionSet);	
	void enterPersonDescriptor(Mention *mention, MentionSet *mentionSet);
	void removeNamesFromDescTheory(const NameTheory *nameTheory, NameTheory*& descTheory);
	void findPartitvesInNPChunkParse(MentionSet* mentionSet);
	void adjustDescTheoryBoundaries(NameTheory *nameTheory, MentionSet *mentionSet);
	// Heuristic to decide if a node is an EDT-relevant pronoun
//	static bool isReferringPronoun(const SynNode *node);
};

#endif

