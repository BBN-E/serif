// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_RULE_DESC_LINKER_H
#define xx_RULE_DESC_LINKER_H

#include "Generic/edt/RuleDescLinker.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/SessionLogger.h"

// generic unimplemented descriptor linker. This does almost nothing
// it displays an error message upon initialization
class GenericRuleDescLinker : public RuleDescLinker {
private:
	friend class GenericRuleDescLinkerFactory;

public:
	int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType type, LexEntitySet *results[], int max_results) { return 0; }

private:
	GenericRuleDescLinker()
	{	
		SessionLogger::warn("unimplemented_class") << "<<<<<WARNING: Using unimplemented generic descriptor linking!>>>>>\n";
	}
};

class GenericRuleDescLinkerFactory: public RuleDescLinker::Factory {
	virtual RuleDescLinker *build() { return _new GenericRuleDescLinker(); }
};

#endif

