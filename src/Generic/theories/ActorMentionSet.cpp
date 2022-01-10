// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/ActorMentionSet.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include <xercesc/util/XMLString.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include "Generic/patterns/PatternMatcher.h"

ActorMentionSet::ActorMentionSet() {}
ActorMentionSet::~ActorMentionSet() {}

ActorMentionSet::ActorMentionSet(
	const std::vector<ActorMentionSet*> splitActorMentionSets, 
	const std::vector<int> sentenceOffsets, 
	const std::vector<MentionSet*> mergedMentionSets, 
	const std::vector<SentenceTheory*> mergedSentenceTheories, 
	boost::unordered_map<ActorMention_ptr, ActorMention_ptr> &actorMentionMap)
{
	for (size_t i = 0; i < splitActorMentionSets.size(); i++) {
		ActorMentionSet *ams = splitActorMentionSets[i];
		for (ActorMentionSet::const_iterator iter = ams->begin(); iter != ams->end(); ++iter) {
			BOOST_FOREACH(ActorMention_ptr am, *iter) {
				// Calculate new MentionUID
				MentionUID uid = am->getEntityMentionUID();
				MentionUID newUid = MentionUID(uid.sentno() + sentenceOffsets[i], uid.index());

				// Get Mention and SentenceTheory pointers
				SentenceTheory *newSentenceTheory = mergedSentenceTheories[uid.sentno() + sentenceOffsets[i]];
				MentionSet *newMentionSet = mergedMentionSets[uid.sentno() + sentenceOffsets[i]];
				Mention *newMention = newMentionSet->getMention(newUid);

				// Make new ActorMention
				ActorMention_ptr newActorMention = am->copyWithNewEntityMention(newSentenceTheory, newMention, L"");
				newActorMention->setSourceNote(am->getSourceNote());

				// Additional things that need to be set on ProperNounActorMention
				ProperNounActorMention_ptr pnam = boost::dynamic_pointer_cast<ProperNounActorMention>(am);
				if (pnam) {
					ProperNounActorMention_ptr newPnam = boost::dynamic_pointer_cast<ProperNounActorMention>(newActorMention);
					if (pnam->isResolvedGeo())
						newPnam->setGeoResolution(pnam->getGeoResolution());
					newPnam->copyScores(pnam);
				}

				actorMentionMap[am] = newActorMention;
				
				// Finally, add new ActorMention to set being created
				appendActorMention(newActorMention);
			}
		}
	}
}

void ActorMentionSet::addActorMention(ActorMention_ptr mention) {
	_actor_mentions[mention->getEntityMentionUID()] = std::vector<ActorMention_ptr>(1, mention);
}

void ActorMentionSet::appendActorMention(ActorMention_ptr mention) {
	if (_actor_mentions.find(mention->getEntityMentionUID()) == _actor_mentions.end()) {
		addActorMention(mention);
		return;
	}

	_actor_mentions[mention->getEntityMentionUID()].push_back(mention);
}

void ActorMentionSet::discardActorMention(ActorMention_ptr mention) {
	std::vector<ActorMention_ptr> &actorMentions = _actor_mentions[mention->getEntityMentionUID()];
	actorMentions.erase(
		std::remove(actorMentions.begin(), actorMentions.end(),	mention), actorMentions.end());

	if (actorMentions.size() == 0)
		_actor_mentions.erase(mention->getEntityMentionUID());
}

ActorMention_ptr ActorMentionSet::find(MentionUID mention_uid) const {
	ActorMentionMap::const_iterator it = _actor_mentions.find(mention_uid);
	if (it == _actor_mentions.end())
		return ActorMention_ptr();
	else {
		std::vector<ActorMention_ptr> results = (*it).second;
		if (results.size() > 1) 
			throw UnrecoverableException("ActorMentionSet::find", "Multiple entries, use findAll instead"); 
		return results[0];
	}
}

std::vector<ActorMention_ptr> ActorMentionSet::findAll(MentionUID mention_uid) const {
	ActorMentionMap::const_iterator it = _actor_mentions.find(mention_uid);
	if (it == _actor_mentions.end())
		return std::vector<ActorMention_ptr>();

	return (*it).second;
}

std::vector<ActorMention_ptr> ActorMentionSet::getAll() const {
	std::vector<ActorMention_ptr> results;

	for (const_iterator it=begin(); it!=end(); ++it) {
		std::vector<ActorMention_ptr> actorMentions = it.dereference();

		BOOST_FOREACH(ActorMention_ptr am, actorMentions) 
			results.push_back(am);
	}
	return results;
}

void ActorMentionSet::addEntityLabels(PatternMatcher_ptr patternMatcher, ActorInfo_ptr actorInfo) const
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
				std::vector<Symbol> sectors = actorInfo->getAssociatedSectorCodes(actorId);
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
				std::vector<Symbol> sectors = actorInfo->getAssociatedSectorCodes(agentId);
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
	using namespace SerifXML;

	elem.loadId(this);
	XMLTheoryElementList emElems = elem.getChildElementsByTagName(X_ActorMention);
	size_t n_ementions = emElems.size();
	for (size_t i=0; i<n_ementions; ++i) {
		appendActorMention(ActorMention::loadXML(emElems[i]));
	}
}

bool compareActorMentions(const ActorMention_ptr a, const ActorMention_ptr b) { 
	if (a->getEntityMentionUID() < b->getEntityMentionUID())
		return true;
	if (b->getEntityMentionUID() < a->getEntityMentionUID())
		return false;

	// ProperNounActorMentions
	ProperNounActorMention_ptr apn = boost::dynamic_pointer_cast<ProperNounActorMention>(a);
	ProperNounActorMention_ptr bpn = boost::dynamic_pointer_cast<ProperNounActorMention>(b);
	if (apn && bpn)
		return apn->getActorId().getId() < bpn->getActorId().getId();

	// should never get here, as there is only multiple ActorMentions  
	// per Mention in the case of ProperNounActorMentions
	return false;
}

void ActorMentionSet::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	using namespace SerifXML;

	std::vector<ActorMention_ptr> allActorMentions = getAll();

	// sort ActorMentions for consistent output
	std::sort(allActorMentions.begin(), allActorMentions.end(), compareActorMentions);

	BOOST_FOREACH(ActorMention_ptr am, allActorMentions) 
	{
		elem.saveChildTheory(X_ActorMention, am.get(), this);
	}
}

void ActorMentionSet::setSentenceTheories(const SentenceTheory *theory) {
	for (const_iterator it=begin(); it!=end(); ++it) {

		std::vector<ActorMention_ptr> actorMentions = (*it);
		BOOST_FOREACH(ActorMention_ptr am, actorMentions) 
			am->setSentenceTheory(theory);
	}
}

void ActorMentionSet::resolvePointers(SerifXML::XMLTheoryElement elem) {
	using namespace SerifXML;

	std::vector<XMLTheoryElement> amElems = elem.getChildElementsByTagName(X_ActorMention);
	if (amElems.size() > 0) {
		const SentenceTheory *sentenceTheory = amElems[0].loadTheoryPointer<SentenceTheory>(X_sentence_theory_id);
		setSentenceTheories(sentenceTheory);
	}
}
