// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_DESCCLASSFUNCTIONS_H
#define en_DESCCLASSFUNCTIONS_H
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/descriptors/DescClassModifiers.h"

class ProbModel;
class ProbModelWriter;
class SynNode;

								  
class EnglishDescClassModifiers : public DescClassModifiers {
private:
	friend class EnglishDescClassModifiersFactory;

	ProbModel* _modModel;
	ProbModel* _modBackoffModel;

	ProbModelWriter* _modWriter;
	ProbModelWriter* _modBackoffWriter;
	
	UTF8OutputStream _modStream;
	UTF8OutputStream _modBackoffStream;
	
	static const int _LOG_OF_ZERO;
	static const int _VOCAB_SIZE;

public:
	~EnglishDescClassModifiers();

	void initialize(const char* model_prefix);
	void initializeForTraining(const char* model_prefix);
	void writeModifiers();
	void closeFiles();

	double getModifierScore(const SynNode* node, Symbol proposedType);
	void addModifers(const SynNode *node, Symbol type);

private:
	EnglishDescClassModifiers();

};

class EnglishDescClassModifiersFactory: public DescClassModifiers::Factory {
	virtual DescClassModifiers *build() { return _new EnglishDescClassModifiers(); }
};

#endif
