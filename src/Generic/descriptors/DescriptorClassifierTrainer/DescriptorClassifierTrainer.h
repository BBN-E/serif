// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCRIPTORCLASSIFIERTRAINER_H
#define DESCRIPTORCLASSIFIERTRAINER_H

#include <boost/shared_ptr.hpp>

#include "Generic/trainers/AnnotatedParseReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/Symbol.h"
#include "Generic/trainers/CorefDocument.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/DescClassModifiers.h"
#include "Generic/descriptors/ClassifierTreeSearch.h"
#include "Generic/descriptors/PartitiveClassifier.h"
#include "Generic/descriptors/AppositiveClassifier.h"
#include "Generic/descriptors/ListClassifier.h"
#include "Generic/descriptors/NestedClassifier.h"
#include "Generic/descriptors/PronounClassifier.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/OtherClassifier.h"

class DescriptorClassifierTrainer {
public:
	/** Create and return a new DescriptorClassifierTrainer. */
	static DescriptorClassifierTrainer *build() { return _factory()->build(); }
	/** Hook for registering new DescriptorClassifierTrainer factories */
	struct Factory { virtual DescriptorClassifierTrainer *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~DescriptorClassifierTrainer();
	
	// this is the major function. Using the given input file, train the models.
	void trainModels();

	void openInputFile(char *trainingFile);
	void closeInputFile();

	// a separate call to this function is required for writing purposes. This separation allows us
	// to train on multiple files before writing the models to disk. 
	virtual void writeModels() = 0;

protected:
	DescriptorClassifierTrainer();

private:
	typedef GrowableArray<CorefItem *> CorefItemList;

	// This function should be implemented by a language-specific derived class, 
	// since the number and type of features and models may vary by language.
	virtual void addMention(MentionSet *mentionSet, Mention *mention, Symbol type) = 0;

	void trainDocument(CorefDocument *document);
	void trainMentions(MentionSet *mentionSet);
	
	void findSentenceCorefItems(CorefItemList &items, const SynNode *node, CorefDocument *document);
	void addCorefItemsToMentionSet(MentionSet* mentionSet, CorefItemList &items);
	void processMentions(const SynNode* node, MentionSet *mentionSet);
	void processNode(const SynNode* node);

	AnnotatedParseReader _trainingSet;

	ClassifierTreeSearch _searcher;
	PartitiveClassifier _partitiveClassifier;
	AppositiveClassifier _appositiveClassifier;
	ListClassifier _listClassifier;
	NestedClassifier _nestedClassifier;
	PronounClassifier* _pronounClassifier;
	OtherClassifier _otherClassifier;

private:
	static boost::shared_ptr<Factory> &_factory();
}; 

//#ifdef ENGLISH_LANGUAGE
//	#include "English/descriptors/DescriptorClassifierTrainer/en_DescriptorClassifierTrainer.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/descriptors/DescriptorClassifierTrainer/ch_DescriptorClassifierTrainer.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/descriptors/DescriptorClassifierTrainer/ar_DescriptorClassifierTrainer.h"
//#elif defined(HINDI_LANGUAGE)
//	#include "English/descriptors/DescriptorClassifierTrainer/en_DescriptorClassifierTrainer.h"
//#elif defined(BENGALI_LANGUAGE)
//	#include "English/descriptors/DescriptorClassifierTrainer/en_DescriptorClassifierTrainer.h"
//#else 
//	#include "Generic/descriptors/DescriptorClassifierTrainer/xx_DescriptorClassifierTrainer.h"
//#endif


#endif
