// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/DescriptorClassifierTrainer.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/xx_DescriptorClassifierTrainer.h"
#include "Generic/descriptors/ClassifierTreeSearch.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"

DescriptorClassifierTrainer::DescriptorClassifierTrainer() : _searcher(false), 
															 _partitiveClassifier(), _appositiveClassifier(), 
															 _listClassifier(), _nestedClassifier(), 
															 _pronounClassifier(PronounClassifier::build()), _otherClassifier() {}


DescriptorClassifierTrainer::~DescriptorClassifierTrainer() {
	closeInputFile();
}

void DescriptorClassifierTrainer::openInputFile(char *trainingFile) {
	if(trainingFile != NULL) 
		_trainingSet.openFile(trainingFile);
}

void DescriptorClassifierTrainer::closeInputFile() {
	_trainingSet.closeFile();
}

void DescriptorClassifierTrainer::trainModels() {
	//read documents, train on each document in turn
	GrowableArray <CorefDocument *> documents;
	_trainingSet.readAllDocuments(documents);
	while(documents.length()!= 0) {
		CorefDocument *thisDocument = documents.removeLast();
		SessionLogger::info("SERIF") << "\nProcessing document: " << thisDocument->documentName.to_debug_string();
		trainDocument(thisDocument);
		delete thisDocument;
	}
}

void DescriptorClassifierTrainer::trainDocument(CorefDocument *document) {
	MentionSet *mentionSet;
	GrowableArray<CorefItem *> corefItems;

	for (int i = 0; i < document->parses.length(); i++) {

		// collect all CorefItems associated with parse before creating MentionSet, 
		// because it overwrites each node's mention pointer
		findSentenceCorefItems(corefItems, document->parses[i]->getRoot(), document);
		mentionSet = _new MentionSet(document->parses[i], i);
		// add annotated mention and entity types to the new mention set
		addCorefItemsToMentionSet(mentionSet, corefItems);
		// classify mention types to rule out all but valid DESC mentions
		processMentions(document->parses[i]->getRoot(), mentionSet);
		// train only on DESC mentions
		// Note: names don't get flattened here as they do in a normal Serif run,
		// so any NPs contained within names could be mistakenly added to training  		 
		trainMentions(mentionSet);

		while (corefItems.length() > 0) 
			CorefItem *item = corefItems.removeLast();
		delete mentionSet;
	}

}

void DescriptorClassifierTrainer::findSentenceCorefItems(CorefItemList &items, 
														const SynNode *node, 
														CorefDocument *document) 
{

	if (node->hasMention()) {
		if  (document->corefItems[node->getMentionIndex()]->mention != 0)
			items.add(document->corefItems[node->getMentionIndex()]);
		// Reset the mention index to default -1, so we don't have problems later
		// with non-NPKind coref items
		document->corefItems[node->getMentionIndex()]->node->setMentionIndex(-1);
	}

	for (int i = 0; i < node->getNChildren(); i++) {
		findSentenceCorefItems(items, node->getChild(i), document);
	}

}

void DescriptorClassifierTrainer::addCorefItemsToMentionSet(MentionSet* mentionSet, CorefItemList &items)
{
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		Mention *mention = mentionSet->getMention(i);
		const SynNode *node = mention->getNode();
		for (int j = 0; j < items.length(); j++) {
			if (items[j]->node == node) {
				mention->setEntityType(items[j]->mention->getEntityType()); 
				// Only add NAME mention type, bc mention classifiers die on already-marked DESCs 
				if (items[j]->mention->mentionType == Mention::NAME)
					mention->mentionType = items[j]->mention->mentionType;
				break;
			}
		}		
	}
}

void DescriptorClassifierTrainer::processMentions(const SynNode* node, MentionSet *mentionSet) {

	// first time into the sentence, so initialize the searcher
	_searcher.resetSearch(mentionSet, 1);
	processNode(node);

}

void DescriptorClassifierTrainer::processNode(const SynNode* node) {

	// do children first
	for (int i = 0; i < node->getNChildren(); i++)
		processNode(node->getChild(i));

	// process this node only if it has a mention
	if (!node->hasMention())
		return;

	// Test 1: See if it's a partitive
	_searcher.performSearch(node, _partitiveClassifier);

	// Test 2: See if it's an appositive
	// won't do anything if it's already partitive
	_searcher.performSearch(node, _appositiveClassifier);

	// Test 3: See if it's a list
	// won't do anything if it's part or appos
	_searcher.performSearch(node, _listClassifier);

	// Test 4: See if it's a nested mention
	// won't do anything if it's part, appos, or list
	// note that being nested doesn't preclude later classification
	_searcher.performSearch(node, _nestedClassifier);

	// test 5: See if it's a pronoun (and mark it but don't classify till later)
	// won't do anything if it's part, appos, list, or nested
	_searcher.performSearch(node, *_pronounClassifier);

	// label everything else as DESC here
	_searcher.performSearch(node, _otherClassifier);
}
	

void DescriptorClassifierTrainer::trainMentions(MentionSet *mentionSet) {

	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		Mention *mention = mentionSet->getMention(i);
		if (mention->mentionType == Mention::DESC && 
			mention->getEntityType() != EntityType::getUndetType())  // JCS - make sure it's not really a pronoun 
		{
			addMention(mentionSet, mention, mention->getEntityType().getName());
		}
	}
}






	


boost::shared_ptr<DescriptorClassifierTrainer::Factory> &DescriptorClassifierTrainer::_factory() {
	static boost::shared_ptr<DescriptorClassifierTrainer::Factory> factory(new DefaultDescriptorClassifierTrainerFactory());
	return factory;
}

