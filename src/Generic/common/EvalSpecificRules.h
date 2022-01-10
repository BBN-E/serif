// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVAL_SPECIFIC_RULES_H
#define EVAL_SPECIFIC_RULES_H

#include <boost/shared_ptr.hpp>

class SynNode;
class Mention;
class EntityType;


class EvalSpecificRules {
public:
	/** Create and return a new EvalSpecificRules. */
	static void NamesToNominals(const SynNode* root, Mention *ment, EntityType &etype) { _factory()->NamesToNominals(root, ment, etype); }
	/** Hook for registering new EvalSpecificRules factories */
	struct Factory { virtual void NamesToNominals(const SynNode* root, Mention *ment, EntityType &etype) = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~EvalSpecificRules() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};
//#if defined(ENGLISH_LANGUAGE)
//	#include "English/common/en_EvalSpecificRules.h"
//#else
//	#include "Generic/common/xx_EvalSpecificRules.h"
//#endif

#endif
