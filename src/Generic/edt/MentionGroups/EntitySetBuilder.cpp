// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/edt/MentionGroups/EntitySetBuilder.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupConfiguration.h"
#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"
#include "Generic/edt/MentionGroups/MentionGroupMerger.h"
#include "Generic/apf/APF4GenericResultCollector.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SentenceTheory.h"

#include <boost/foreach.hpp>

EntitySetBuilder::EntitySetBuilder() : 
	_config(MentionGroupConfiguration::build()),
	_mergers(_config->buildMergers()),
	_cache(_new LinkInfoCache(_config.get()))	
{
	_create_entities_of_undetermined_type = ParamReader::isParamTrue("mg_create_entities_of_undetermined_type");
}

EntitySetBuilder::~EntitySetBuilder() {}

EntitySet* EntitySetBuilder::buildEntitySet(DocTheory *docTheory) const {
	_cache->setDocTheory(docTheory);
	_cache->populateMentionFeatureTable();
	//initializeProfileTable();
	MentionGroupList groups = initializeMentionGroups(docTheory);
	merge(groups);
	return produceEntitySet(groups, docTheory);
}

void EntitySetBuilder::merge(MentionGroupList &groups) const {
	_mergers->merge(groups, *_cache);
}

MentionGroupList EntitySetBuilder::initializeMentionGroups(DocTheory *docTheory) const {
	MentionGroupList results;
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		MentionSet *mentionSet = docTheory->getSentenceTheory(i)->getMentionSet();
		for (int m = 0; m < mentionSet->getNMentions(); m++) {
			if (isCoreferenceableMention(mentionSet->getMention(m)))
				results.push_back(boost::make_shared<MentionGroup>(mentionSet->getMention(m)));
		}
	}
	return results;
}

EntitySet* EntitySetBuilder::produceEntitySet(MentionGroupList& groups,
                                              DocTheory *docTheory) const
{
	EntitySet *result = _new EntitySet(docTheory->getNSentences());
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		result->loadMentionSet(docTheory->getSentenceTheory(i)->getMentionSet());
	}
	BOOST_FOREACH(MentionGroup_ptr group, groups) {
		EntityType entityType = group->getEntityType();
		if (!_create_entities_of_undetermined_type && !entityType.isDetermined())
			continue;
		for (MentionGroup::const_iterator it = group->begin(); it != group->end(); ++it) {
			const Mention *ment = *it;
			if (it == group->begin()) {
				result->addNew((*it)->getUID(), entityType);
			} else {		
				result->add((*it)->getUID(), result->getNEntities()-1);
			}
			if (ment->getEntityType() != entityType) {
				const SentenceTheory *sentTheory = docTheory->getSentenceTheory(ment->getUID().sentno());
				Mention *non_const_sent_ment = sentTheory->getMentionSet()->getMention(ment->getUID());
				non_const_sent_ment->setEntityType(entityType);
				Mention *non_const_result_ment = result->getMention(ment->getUID());
				non_const_result_ment->setEntityType(entityType);
			}
		}
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_history")) 
			SessionLogger::dbg("MentionGroups_history") << "Entity " << result->getNEntities() << "\n" << group->getMergeHistoryString();
	}
	return result;
}

bool EntitySetBuilder::isCoreferenceableMention(const Mention *ment) const {	
	Mention::Type mentType =  ment->getMentionType();
	EntityType entType = ment->getEntityType();
	return (((mentType == Mention::NAME || mentType == Mention::DESC || 
		      mentType == Mention::PART || mentType == Mention::NEST ||
			  mentType == Mention::APPO) && 
			 entType.isRecognized()) ||
		   (mentType == Mention::PRON));
}

