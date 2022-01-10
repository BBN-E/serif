// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ch_DESCCLASSFUNCTIONS_H
#define ch_DESCCLASSFUNCTIONS_H
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/descriptors/DescClassModifiers.h"

class ProbModel;
class ProbModelWriter;
class SynNode;

								  
class ChineseDescClassModifiers : public DescClassModifiers {
private:
	friend class ChineseDescClassModifiersFactory;

	ProbModel* _modModel;
	ProbModel* _modBackoffModel;

	ProbModelWriter* _modWriter;
	ProbModelWriter* _modBackoffWriter;
	
	UTF8OutputStream _modStream;
	UTF8OutputStream _modBackoffStream;
	
	static const int _LOG_OF_ZERO;
	static const int _VOCAB_SIZE;

public:
	~ChineseDescClassModifiers();

	virtual void initialize(const char* model_prefix) { initialize(boost::lexical_cast<std::string>(model_prefix)); }
	void initialize(std::string model_prefix);
	virtual void initializeForTraining(const char* model_prefix) { initializeForTraining(boost::lexical_cast<std::string>(model_prefix)); }
	void initializeForTraining(std::string model_prefix);
	void writeModifiers();
	void closeFiles();

	double getModifierScore(const SynNode* node, Symbol proposedType);
	void addModifers(const SynNode *node, Symbol type);

private:
	ChineseDescClassModifiers();


};

class ChineseDescClassModifiersFactory: public DescClassModifiers::Factory {
	virtual DescClassModifiers *build() { return _new ChineseDescClassModifiers(); }
};

#endif
