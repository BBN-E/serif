// Copyright (c) 2014 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/FactSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/TopLevelPFeature.h"
#include "Generic/factfinder/SectorFactFinder.h"
#include "Generic/factfinder/FactPatternManager.h"

const std::wstring SectorFactFinder::FF_SECTOR = L"ff_sector";
const std::wstring SectorFactFinder::FF_ROLE = L"ff_role";
const std::wstring SectorFactFinder::FF_AGENT1 = L"AGENT1";

const Symbol SectorFactFinder::FF_ACTOR_SECTOR = Symbol(L"ActorSector");
const Symbol SectorFactFinder::FF_SECTOR_SYM = Symbol(L"Sector");
const Symbol SectorFactFinder::FF_ACTOR = Symbol(L"Actor");

SectorFactFinder::SectorFactFinder() { 

	_patternManager = _new FactPatternManager(ParamReader::getRequiredParam("sector_fact_pattern_list"));
}

SectorFactFinder::~SectorFactFinder() { }

void SectorFactFinder::findFacts(DocTheory *docTheory) {
	FactSet *factSet = docTheory->getFactSet();
	if (!factSet) 
		// FactSet should have been initized by FactFinder which calls this function
		throw InternalInconsistencyException("SectorFactFinder::findFacts", "docTheory lacks existing FactSet");

	PatternSet_ptr currentPatternSet;
	for (size_t setno = 0; setno < _patternManager->getNFactPatternSets(); setno++) {
		currentPatternSet = _patternManager->getFactPatternSet(setno);
		Symbol patternSetEntityTypeSymbol = _patternManager->getEntityTypeSymbol(setno);		
		Symbol extraFlagSymbol = _patternManager->getExtraFlagSymbol(setno);
		
		PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(docTheory, currentPatternSet, 0, PatternMatcher::COMBINE_SNIPPETS_BY_COVERAGE);
		processPatternSet(pm, patternSetEntityTypeSymbol, extraFlagSymbol);
	}
}

void SectorFactFinder::processPatternSet(PatternMatcher_ptr pm, Symbol patSetEntityTypeSymbol, Symbol extraFlagSymbol) { 
	
	const Symbol ALLOW_DESC_SYM(L"ALLOW_DESC");

	bool names_only = true;
	if (extraFlagSymbol == ALLOW_DESC_SYM)
		names_only = false;

	const DocTheory* docTheory = pm->getDocTheory();
	const EntitySet *entitySet = docTheory->getEntitySet();
	int docEntitiesCount = entitySet->getNEntities();
	for (int entity_id = 0; entity_id < docEntitiesCount; entity_id++) {
		Entity* entity = entitySet->getEntity(entity_id);
		if (entity == NULL)
			continue;
		EntityType entityType = entity->getType();
		// Only process entities of the type that matches the pattern set type
		if (patSetEntityTypeSymbol != entityType.getName()){
			continue;
		}
		// Only process entities with names
		if (!names_only || entity->hasNameMention(entitySet)) {
			// Should we restrict entities here, as in FactFinder?
			processEntity(pm, entity_id);
		}
	}
}

void SectorFactFinder::processEntity(PatternMatcher_ptr pm, int entity_id) {

	// Limit the maximum number of features that can fire per pattern per sentence
	size_t max_sets = 50;

	pm->clearEntityLabels();
	pm->setEntityLabelConfidence(FF_AGENT1, entity_id, 1.0);
	pm->labelPatternDrivenEntities();

	const DocTheory* docTheory = pm->getDocTheory();
	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		SentenceTheory* sTheory = docTheory->getSentenceTheory(sentno);
		std::vector<PatternFeatureSet_ptr> featureSets = 
			pm->getSentenceSnippets(sTheory, 0, false);
		for (size_t i = 0; i < featureSets.size() && i < max_sets; ++i) {
			if (featureSets[i] != NULL){				
				addFactsFromFeatureSet(pm, entity_id, featureSets[i]);
			}
		}
	}
	pm->clearEntityLabels();
}

void SectorFactFinder::addFactsFromFeatureSet(PatternMatcher_ptr pm, int entity_id, PatternFeatureSet_ptr featureSet) {
	Symbol factType;
	Symbol patternID;

	// Look for both a MentionReturnFeature with an (return (ff_role Actor)) and another ReturnFeature with ff_sector as a return value
	MentionReturnPFeature_ptr actorReturnFeature = MentionReturnPFeature_ptr();
	ReturnPatternFeature_ptr sectorReturnFeature = ReturnPatternFeature_ptr();

	for (size_t i = 0; i < featureSet->getNFeatures(); i++) {
		PatternFeature_ptr feature = featureSet->getFeature(i);
		if (TopLevelPFeature_ptr topLevelFeature = boost::dynamic_pointer_cast<TopLevelPFeature>(feature)) {
			patternID = topLevelFeature->getPatternLabel();	
		}

		if (ReturnPatternFeature_ptr returnFeature = boost::dynamic_pointer_cast<ReturnPatternFeature>(feature)) {

			if (returnFeature->hasReturnValue(FF_ROLE) && returnFeature->getReturnValue(FF_ROLE) == FF_ACTOR.to_string()) {
				MentionReturnPFeature_ptr menFeature = boost::dynamic_pointer_cast<MentionReturnPFeature>(returnFeature);
				if (!menFeature)
					throw UnexpectedInputException("SectorFactFinder::addFactsFromFeatureSet", "(ff_role Actor) must be on mention pattern");

				actorReturnFeature = menFeature;
			}

			if (returnFeature->hasReturnValue(FF_SECTOR))
				sectorReturnFeature = returnFeature;
		}
	}

	if (actorReturnFeature == MentionReturnPFeature_ptr() || sectorReturnFeature == ReturnPatternFeature_ptr())
		return;

	const Mention *mention = actorReturnFeature->getMention();
	
	// Check to make sure part of the head of the mention is within the sector triggering pattern
	int head_start = mention->getHead()->getStartToken();
	int head_end = mention->getHead()->getEndToken();
	int sector_start = sectorReturnFeature->getStartToken();
	int sector_end = sectorReturnFeature->getEndToken();

	if ((head_start >= sector_start && head_start <= sector_end) ||
		(head_end >= sector_start && head_end <= sector_end) ||
		(head_start <= sector_start && head_end >= sector_end))
	{
		Fact_ptr fact = boost::make_shared<Fact>(FF_ACTOR_SECTOR, patternID, featureSet->getScore(), featureSet->getBestScoreGroup(), featureSet->getStartSentence(), 
			featureSet->getEndSentence(), featureSet->getStartToken(), featureSet->getEndToken());

		Symbol sector = sectorReturnFeature->getReturnValue(FF_SECTOR);
		fact->addFactArgument(boost::make_shared<StringFactArgument>(FF_SECTOR_SYM, sector.to_string()));
		fact->addFactArgument(boost::make_shared<MentionFactArgument>(FF_ACTOR, mention->getUID()));
		pm->getDocTheory()->getFactSet()->addFact(fact);
	} else if (mention->getMentionType() == Mention::NAME) {
		// Test to see if pattern is matching any nominal premod descriptor of the name
		const DocTheory* docTheory = pm->getDocTheory();
		const EntitySet *entitySet = docTheory->getEntitySet();		
		const Entity *entity = entitySet->getEntity(entity_id);

		for (int i = 0; i < entity->getNMentions(); i++) {
			MentionUID muid = entity->getMention(i);
			const Mention *mention = entitySet->getMention(muid);
			if (mention->getSentenceNumber() != featureSet->getStartSentence())
				continue;
			const SynNode *node = mention->getNode();
			int start_token = node->getStartToken();
			int end_token = node->getEndToken();
			if (start_token != end_token)
				continue;
			if (start_token >= sector_start && start_token <= sector_end) {
				Fact_ptr fact = boost::make_shared<Fact>(FF_ACTOR_SECTOR, patternID, featureSet->getScore(), featureSet->getBestScoreGroup(), featureSet->getStartSentence(), 
					featureSet->getEndSentence(), featureSet->getStartToken(), featureSet->getEndToken());

				Symbol sector = sectorReturnFeature->getReturnValue(FF_SECTOR);
				fact->addFactArgument(boost::make_shared<StringFactArgument>(FF_SECTOR_SYM, sector.to_string()));
				fact->addFactArgument(boost::make_shared<MentionFactArgument>(FF_ACTOR, mention->getUID()));
				pm->getDocTheory()->getFactSet()->addFact(fact);
				break;
			}
		}
	}
}
