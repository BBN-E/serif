// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCCLASSMODIFIERS_H
#define DESCCLASSMODIFIERS_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/Symbol.h"
#include <iostream>


class ProbModel;
class ProbModelWriter;
class SynNode;

class DescClassModifiers  {
public:
	/** Create and return a new DescClassModifiers. */
	static DescClassModifiers *build() { return _factory()->build(); }
	/** Hook for registering new DescClassModifiers factories */
	struct Factory { virtual DescClassModifiers *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

/*
private:
	ProbModel* _modModel;
	ProbModel* _modBackoffModel;

	ProbModelWriter* _modWriter;
	ProbModelWriter* _modBackoffWriter;
	
	UTF8OutputStream _modStream;
	UTF8OutputStream _modBackoffStream;
	
	static const int _LOG_OF_ZERO;
	static const int _VOCAB_SIZE;
*/
public:
	virtual ~DescClassModifiers(){};
	/**
	* intialize a _modModel and _modBackoffModel for decoding
	*/
	virtual void initialize(const char* model_prefix) {};
	/*
	* initialize _modWriter, and _modBackoffWriter for training
	* open coresponding streams
	*/
	virtual void initializeForTraining(const char* model_prefix) {};
	virtual void writeModifiers() {};
	virtual void closeFiles() {};

	/*
	* get the Modifier part of the descriptor classification score
	* for node with proposed type
	*/
	virtual double getModifierScore(const SynNode* node, Symbol proposedType) { return 0.0; }
	/*
	* record node's modifiers
	*
	*/
	virtual void addModifers(const SynNode *node, Symbol type) {};

protected:
	DescClassModifiers(){};

private:
	static boost::shared_ptr<Factory> &_factory();
};
//#ifdef ENGLISH_LANGUAGE
//	#include "English/descriptors/en_DescClassModifiers.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/descriptors/ch_DescClassModifiers.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/descriptors/ar_DescClassModifiers.h"
//#elif defined(HINDI_LANGUAGE)
//	#includee "English/descriptors/en_DescClassModifiers.h"
//#endif



#endif
