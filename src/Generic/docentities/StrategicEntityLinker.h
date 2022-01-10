// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STRATEGIC_ENTITY_LINKER_H
#define STRATEGIC_ENTITY_LINKER_H

#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/DebugStream.h"

/**
 * Do document-wide entity linking
 */
class StrategicEntityLinker {
public:
	StrategicEntityLinker();
	~StrategicEntityLinker();
	void linkEntities(DocTheory* docTheory);
private:
	DebugStream _debug;
	bool _doStrategicLinking;
	bool _linkToNames;
	double _singletonDescriptorEntityPercentage;
	int _singletonMergeSize;
	void _linkStrategically(DocTheory* docTheory);
	void _linkToNamesStrategically(DocTheory* docTheory);
	Entity* _getSmallestNameEntity(const EntitySet* ents, EntityType type);
	size_t _getMergeSetMentionLength(Entity** set, size_t size, const EntitySet* ents);

};
#endif
