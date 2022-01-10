#include <map>
#include <boost/make_shared.hpp>
#include "Generic/common/leak_detection.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"
#include "MentionToEntityMap.h"

MentionToEntityMap_ptr MentionToEntityMapper::getMentionToEntityMap(const DocTheory* doc_theory) {
	MentionToEntityMap_ptr returnVal = boost::make_shared<MentionToEntityMap>();
	EntitySet* entity_set = doc_theory->getEntitySet();
	for( int i = 0; i < entity_set->getNEntities(); ++i ){
		Entity* entity = entity_set->getEntity(i);
		for( int j = 0; j < entity->getNMentions(); ++j ){
			(*returnVal)[entity->getMention(j)] = entity;
		}
	}
	return returnVal;
}

MentionToEntityMap_ptr MentionToEntityMapper::getMentionToEntityMap(const AlignedDocSet_ptr& doc_set) {
	MentionToEntityMap_ptr returnVal = boost::make_shared<MentionToEntityMap>();
	BOOST_FOREACH(LanguageVariant_ptr lv, doc_set->getLanguageVariants()) {
		const DocTheory* doc_theory = doc_set->getDocTheory(lv);
		EntitySet* entity_set = doc_theory->getEntitySet();
		for( int i = 0; i < entity_set->getNEntities(); ++i ){
			Entity* entity = entity_set->getEntity(i);
			for( int j = 0; j < entity->getNMentions(); ++j ){
				(*returnVal)[entity->getMention(j)] = entity;
			}
		}
	}
	return returnVal;
}
