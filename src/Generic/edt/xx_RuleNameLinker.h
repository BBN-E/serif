// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_RULE_NAME_LINKER_H
#define xx_RULE_NAME_LINKER_H

#include "Generic/edt/RuleNameLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/SessionLogger.h"

// generic unimplemented rule-based name linker. This does almost nothing
// it displays an error message upon initialization
class GenericRuleNameLinker : public RuleNameLinker {
private:
	friend class GenericRuleNameLinkerFactory;

public:
	int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType type, LexEntitySet *results[], int max_results) { return 0; }

private:
	GenericRuleNameLinker()
	{	
		SessionLogger::warn("unimplemented_class") << "<<<<<WARNING: Using unimplemented generic rule-based name linking!>>>>>\n";
	}
};

class GenericRuleNameLinkerFactory: public RuleNameLinker::Factory {
	virtual RuleNameLinker *build() { return _new GenericRuleNameLinker(); }
};

#endif

