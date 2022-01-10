// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_DESCCLASSFUNCTIONS_H
#define ar_DESCCLASSFUNCTIONS_H
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/descriptors/DescClassModifiers.h"

class ProbModel;
class ProbModelWriter;
class SynNode;

								  
class ArabicDescClassModifiers : public DescClassModifiers {
private:
	friend class ArabicDescClassModifiersFactory;

	ProbModel* _modModel;
	ProbModel* _modBackoffModel;

	ProbModelWriter* _modWriter;
	ProbModelWriter* _modBackoffWriter;
	
	UTF8OutputStream _modStream;
	UTF8OutputStream _modBackoffStream;
	
	static const int _LOG_OF_ZERO;
	static const int _VOCAB_SIZE;

	ArabicDescClassModifiers();

public:
	~ArabicDescClassModifiers();


	virtual void initialize(const char* model_prefix) { initialize(boost::lexical_cast<std::string>(model_prefix)); }
	void initialize(std::string model_prefix);
	virtual void initializeForTraining(const char* model_prefix) { initializeForTraining(boost::lexical_cast<std::string>(model_prefix)); }
	void initializeForTraining(std::string model_prefix);
	void writeModifiers();
	void closeFiles();

	double getModifierScore(const SynNode* node, Symbol proposedType);
	void addModifers(const SynNode *node, Symbol type);




};

class ArabicDescClassModifiersFactory: public DescClassModifiers::Factory {
	virtual DescClassModifiers *build() { return _new ArabicDescClassModifiers(); }
};

#endif
