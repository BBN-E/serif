// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "ICEWS/ActorMentionSet.h"
#include "ICEWS/ActorInfo.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include <xercesc/util/XMLString.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include "Generic/patterns/PatternMatcher.h"

namespace ICEWS {

ActorMentionSet::ActorMentionSet() {}
ActorMentionSet::~ActorMentionSet() {}


void ActorMentionSet::addActorMention(ActorMention_ptr mention) {
	_actor_mentions[mention->getEntityMention()->getUID()] = mention;
}

void ActorMentionSet::discardActorMention(ActorMention_ptr mention) {
	_actor_mentions.erase(mention->getEntityMention()->getUID());
}

ActorMention_ptr ActorMentionSet::find(MentionUID mention_uid) const {
	ActorMentionMap::const_iterator it = _actor_mentions.find(mention_uid);
	if (it == _actor_mentions.end())
		return ActorMention_ptr();
	else
		return (*it).second;
}

void ActorMentionSet::addEntityLabels(PatternMatcher_ptr patternMatcher, ActorInfo &actorInfo) const
{
	static const Symbol ACTOR(L"ACTOR"); // any actor
	static const Symbol KNOWN_ACTOR(L"KNOWN_ACTOR"); // proper noun actor or agent of known actor
	static const Symbol COMPOSITE_ACTOR(L"COMPOSITE_ACTOR"); // agent of known or unknown actor
	static const Symbol PROPER_NOUN_ACTOR(L"PROPER_NOUN_ACTOR");

	const EntitySet* entitySet = patternMatcher->getDocTheory()->getEntitySet();
	for (int entity_num=0; entity_num<entitySet->getNEntities(); ++entity_num) {
		const Entity* entity = entitySet->getEntity(entity_num);
		for (int mention_num=0; mention_num<entity->getNMentions();++mention_num) {
			MentionUID mention_uid = entity->getMention(mention_num);
			ActorMention_ptr actorMention = find(mention_uid);
			if (!actorMention) continue;

			patternMatcher->setEntityLabelConfidence(ACTOR, entity->getID(), 1);

			if (ProperNounActorMention_ptr pnActorMention = boost::dynamic_pointer_cast<ProperNounActorMention>(actorMention)) {
				patternMatcher->setEntityLabelConfidence(PROPER_NOUN_ACTOR, entity->getID(), 1);
				patternMatcher->setEntityLabelConfidence(KNOWN_ACTOR, entity->getID(), 1);
				ActorId actorId = pnActorMention->getActorId();
				std::vector<Symbol> sectors = actorInfo.getAssociatedSectorCodes(actorId);
				BOOST_FOREACH(const Symbol &sector, sectors) {
					std::wostringstream sectorLabel;
					sectorLabel << L"SECTOR_" << sector.to_string();
					Symbol sectorLabelSym(sectorLabel.str().c_str());
					patternMatcher->setEntityLabelConfidence(sectorLabelSym, entity->getID(), 1);
				}
			}
			if (CompositeActorMention_ptr cActorMention = boost::dynamic_pointer_cast<CompositeActorMention>(actorMention)) {
				patternMatcher->setEntityLabelConfidence(COMPOSITE_ACTOR, entity->getID(), 1);
				if (!cActorMention->getPairedActorId().isNull())
					patternMatcher->setEntityLabelConfidence(KNOWN_ACTOR, entity->getID(), 1);
				AgentId agentId = cActorMention->getPairedAgentId();
				std::vector<Symbol> sectors = actorInfo.getAssociatedSectorCodes(agentId);
				BOOST_FOREACH(const Symbol &sector, sectors) {
					std::wostringstream sectorLabel;
					sectorLabel << L"SECTOR_" << sector.to_string();
					Symbol sectorLabelSym(sectorLabel.str().c_str());
					patternMatcher->setEntityLabelConfidence(sectorLabelSym, entity->getID(), 1);
				}
			}
		}
	}

	// Label entities now that these above labels are set
	patternMatcher->relabelEntitiesWithoutClearing();
}


void ActorMentionSet::updateObjectIDTable() const {
	throw InternalInconsistencyException("ActorMentionSet::updateObjectIDTable",
		"ActorMentionSet does not currently have state file support");
}
void ActorMentionSet::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("ActorMentionSet::saveState",
		"ActorMentionSet does not currently have state file support");
}
void ActorMentionSet::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("ActorMentionSet::resolvePointers",
		"ActorMentionSet does not currently have state file support");
}

ActorMentionSet::ActorMentionSet(SerifXML::XMLTheoryElement elem, const DocTheory* theory) {
	static const XMLCh* X_ActorMention = xercesc::XMLString::transcode("ActorMention");

	using namespace SerifXML;
	elem.loadId(this);
	XMLTheoryElementList emElems = elem.getChildElementsByTagName(X_ActorMention);
	size_t n_ementions = emElems.size();
	for (size_t i=0; i<n_ementions; ++i) {
		addActorMention(ActorMention::loadXML(emElems[i]));
	}
}

void ActorMentionSet::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	static const XMLCh* X_ActorMention = xercesc::XMLString::transcode("ActorMention");

	size_t n_ementions = _actor_mentions.size();
	for (const_iterator it=begin(); it!=end(); ++it) {
		elem.saveChildTheory(X_ActorMention, (*it).get());
	}
}


} // end of ICEWS namespace
