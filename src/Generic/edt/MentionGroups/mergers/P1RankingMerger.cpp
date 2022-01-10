// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/MentionGroups/mergers/P1RankingMerger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/edt/EntityGuess.h"

// Note: As things stand now, _featureTypesArray and _p1Weights get passed to the _p1Decoder constructor 
// before they've been fully initialized (in the initialization list).  This only works because the _p1Decoder
// constructor doesn't access the pointers - it just makes a copy of them.
P1RankingMerger::P1RankingMerger(Mention::Type target, std::string model_file,
                                 std::string tag_set_file, std::string features_file, 
                                 std::string overgen_threshold,
                                 MentionGroupConstraint_ptr constraints) : 
	MentionGroupMerger(Symbol(L"P1Ranking"), constraints),
	_targetType(target), _overgen_threshold(0),
	_infoMap(_new DocumentMentionInformationMapper()),
	_tagSet(_new DTTagSet(ParamReader::getRequiredParam(tag_set_file).c_str(), false, false)),
	_linkFeatureTypes(_new DTFeatureTypeSet(ParamReader::getRequiredParam(features_file).c_str(), DTCorefFeatureType::modeltype)),
	_noneFeatureTypes(DTCorefFeatureTypes::makeNoneFeatureTypeSet(target)),
	_featureTypesArray(_new DTFeatureTypeSet*[2]), // 2 for the # of tags: always 'link' and 'no-link'
	_p1Weights(_new DTFeature::FeatureWeightMap(500009)),
	_p1Decoder(_new P1Decoder(_tagSet.get(), _featureTypesArray.get(), _p1Weights.get())),
	_observation(_new DTCorefObservation(_infoMap.get()))
{
	
	std::string file = ParamReader::getRequiredParam(model_file) + "-rank";
	DTFeature::readWeights(*_p1Weights, file.c_str(), DTCorefFeatureType::modeltype);

	_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue(overgen_threshold.c_str(), 0);		

	_featureTypesArray[_tagSet->getNoneTagIndex()] = _noneFeatureTypes.get();
	_featureTypesArray[_tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol())] = _linkFeatureTypes.get();
}

void P1RankingMerger::merge(MentionGroupList& groups, LinkInfoCache& cache) {
	EntitySet* entitySet = buildFakeEntitySet(groups, cache.getDocTheory());
	_observation->resetForNewDocument(entitySet);
	
	for (MentionGroupList::iterator it1 = groups.begin(); it1 != groups.end(); ++it1) {
		MentionGroup_ptr group1 = *it1;
		const Mention *linkMention = *group1->begin();
		
		if (linkMention->getMentionType() != _targetType)
			continue;

		_observation->resetForNewSentence(linkMention->getMentionSet());
		std::set<RankingRecord> scores;

		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
			SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------\n";
			SessionLogger::dbg("MentionGroups_p1ranking") << "PROCESSING Mention #" << linkMention->getUID() << ": "
					<<  linkMention->getNode()->toTextString() << "\n";
		}

		// score the no-link option
		DTNoneCorefObservation *noneObs = static_cast<DTNoneCorefObservation*>(_observation.get());
		noneObs->populate(linkMention->getUID());
		double score = _p1Decoder->getScore(noneObs, _tagSet->getNoneTagIndex()) - _overgen_threshold;
		scores.insert(RankingRecord(_tagSet->getNoneTagIndex(), boost::shared_ptr<MentionGroup>(), score));
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
			SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------";
			SessionLogger::dbg("MentionGroups_p1ranking") << "CONSIDERING NO-LINK option";
			SessionLogger::dbg("MentionGroups_p1ranking") << _p1Decoder->getDebugInfo(noneObs, _tagSet->getNoneTagIndex());
			SessionLogger::dbg("MentionGroups_p1ranking") << "ADJUSTED NO-LINK SCORE: " << score;
		}

		// score linking against each of the other MentionGroups
		for (MentionGroupList::iterator it2 = groups.begin(); it2 != groups.end(); ++it2) {
			if (it1 == it2)
				continue;

			if (!isLegalMerge(**it1, **it2))
				continue;

			MentionGroup_ptr group2 = *it2;
			const Mention *firstMention = *group2->begin();

			// Skip if the first mention of the group occurs after linkMention's sentence.
			if (firstMention->getUID().sentno() > linkMention->getUID().sentno())
				continue;

			// The group's entity type must match linkMention's.
			if (linkMention->getEntityType().isDetermined() && linkMention->getEntityType() != group2->getEntityType() && linkMention->getEntityType().getName() != Symbol(L"POG") && group2->getEntityType().getName() != Symbol(L"POG"))
				continue;

			int link_index = _tagSet->getLinkTagIndex();
			Entity *linkEntity = entitySet->getEntityByMentionWithoutType(firstMention->getUID());
			_observation->populate(linkMention->getUID(), linkEntity->getID());
			scores.insert(RankingRecord(link_index, group2, _p1Decoder->getScore(_observation.get(), link_index)));
			if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
				SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------";
				SessionLogger::dbg("MentionGroups_p1ranking") << "CONSIDERING LINK TO MentionGroup " << group2->toString();
				SessionLogger::dbg("MentionGroups_p1ranking") << _p1Decoder->getDebugInfo(_observation.get(), link_index);
			}
		}
		
		// Iterate through the options starting with the highest score until we find a 
		// valid merge or reach the no-link option.
		for (std::set<RankingRecord>::iterator it = scores.begin(); it != scores.end() && (*it).tag_index != _tagSet->getNoneTagIndex(); ++it) {
			MentionGroup_ptr group2 = (*it).group;
			if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
				const Mention *m1 = *(group1->begin());
				const Mention *m2 = *(group2->begin());
				SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
						<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
						<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
				SessionLogger::dbg("MentionGroups_shouldMerge") << "P1 RANKING " << Mention::getTypeString(_targetType) << " MODEL LINK FOUND";
			}
			if (_constraints.get() == 0 || !_constraints->violatesMergeConstraint(*group1, *group2, cache)) {				
				SessionLogger::dbg("MentionGroupMerger") << "Merge identified.";
				group1->merge(*group2, this, (*it).score);
				groups.erase(std::remove(groups.begin(), groups.end(), group2), groups.end());
				break;
			}
		}	

		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
			SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------";
			SessionLogger::dbg("MentionGroups_p1ranking") << "DONE PROCESSING MENTION #" << linkMention->getUID();
			SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------";
		}
	}

	_infoMap->cleanUpAfterDocument();
	delete entitySet;

}

bool P1RankingMerger::shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const {
	SessionLogger::warn("mention_groups") << "P1RankingMerger::shouldMerge() should never be called.";
	return false;
}


EntitySet* P1RankingMerger::buildFakeEntitySet(MentionGroupList& groups, const DocTheory *docTheory) {
	int n_sentences = docTheory->getNSentences();
	EntitySet *result = _new EntitySet(docTheory->getNSentences());
	
	// Load all MentionSets into the EntitySet to mimic the state of the EntitySet at the 
	// end of the document. Copy only the last MentionSet because it will be deleted by the EntitySet 
	// destructor.
	for (int i = 0; i < n_sentences-1; i++) {
		result->loadDoNotCopyMentionSet(docTheory->getSentenceTheory(i)->getMentionSet());
	}
	result->loadMentionSet(docTheory->getSentenceTheory(n_sentences-1)->getMentionSet());

	// Add a new Entity for each MentionGroup.
	for (MentionGroupList::iterator it1 = groups.begin(); it1 != groups.end(); ++it1) {
		MentionGroup_ptr group = (*it1);	
		for (MentionGroup::const_iterator it2 = group->begin(); it2 != group->end(); ++it2) {
			const Mention *ment = *it2;
			if (it2 == group->begin()) {
				result->addNew(ment->getUID(), group->getEntityType());
			} else {
				result->add(ment->getUID(), result->getNEntities()-1);
			}
			_infoMap->addMentionInformation(ment);
		}
	}

	return result;
}
