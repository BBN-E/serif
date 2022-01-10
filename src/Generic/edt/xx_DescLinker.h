// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_DESC_LINKER_H
#define xx_DESC_LINKER_H

#include "Generic/edt/DescLinker.h"
#include "Generic/edt/LexEntitySet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"
#include "Generic/common/SessionLogger.h"

// generic unimplemented descriptor linker. This does almost nothing
// it displays an error message upon initialization
class GenericDescLinker : public DescLinker {
private:
	friend class GenericDescLinkerFactory;

public:
	virtual ~GenericDescLinker() {}
	int linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType type, LexEntitySet *results[], int max_results) { return 0; }

private:
	GenericDescLinker()
	{	
		SessionLogger::warn("SERIF") << "<<<<<WARNING: Using unimplemented generic descriptor linking!>>>>>\n";
	}
};

class GenericDescLinkerFactory: public DescLinker::Factory {
	virtual DescLinker *build() { return _new GenericDescLinker(); }
};

#endif

