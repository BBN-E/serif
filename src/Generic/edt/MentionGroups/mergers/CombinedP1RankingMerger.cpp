// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/MentionGroups/mergers/CombinedP1RankingMerger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/WordConstants.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/edt/HobbsDistance.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/MentionGroups/LinkInfoCache.h"
#include "Generic/edt/MentionGroups/MentionGroup.h"
#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"
#include "Generic/edt/MentionGroups/constraints/CompositeMentionGroupConstraint.h"
#include "Generic/edt/MentionGroups/constraints/HeadWordClashConstraint.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/edt/EntityGuess.h"

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

// Note: As things stand now, _featureTypesArray and _p1Weights get passed to the _p1Decoder constructor 
// before they've been fully initialized (in the initialization list).  This only works because the _p1Decoder
// constructor doesn't access the pointers - it just makes a copy of them.
CombinedP1RankingMerger::CombinedP1RankingMerger(MentionGroupConstraint_ptr constraints) : 
	MentionGroupMerger(Symbol(L"P1Ranking"), constraints),
	_infoMap(_new DocumentMentionInformationMapper()),
	_name_overgen_threshold(0), _desc_overgen_threshold(0), _pron_overgen_threshold(0),
	_tagSet(_new DTTagSet(ParamReader::getRequiredParam("dt_coref_tag_set_file").c_str(), false, false)),
	_nameLinkFeatureTypes(_new DTFeatureTypeSet(ParamReader::getRequiredParam("dt_name_coref_features_file").c_str(), DTCorefFeatureType::modeltype)),
	_nameNoneFeatureTypes(DTCorefFeatureTypes::makeNoneFeatureTypeSet(Mention::NAME)),
	_nameFeatureTypesArray(_new DTFeatureTypeSet*[2]), // 2 for the # of tags: always 'link' and 'no-link'
	_descLinkFeatureTypes(_new DTFeatureTypeSet(ParamReader::getRequiredParam("dt_coref_features_file").c_str(), DTCorefFeatureType::modeltype)),
	_descNoneFeatureTypes(DTCorefFeatureTypes::makeNoneFeatureTypeSet(Mention::DESC)),
	_descFeatureTypesArray(_new DTFeatureTypeSet*[2]), // 2 for the # of tags: always 'link' and 'no-link'
	_pronLinkFeatureTypes(_new DTFeatureTypeSet(ParamReader::getRequiredParam("dt_pron_features_file").c_str(), DTCorefFeatureType::modeltype)),
	_pronNoneFeatureTypes(DTCorefFeatureTypes::makeNoneFeatureTypeSet(Mention::PRON)),
	_pronFeatureTypesArray(_new DTFeatureTypeSet*[2]), // 2 for the # of tags: always 'link' and 'no-link'
	_nameWeights(_new DTFeature::FeatureWeightMap(500009)),
	_nameDecoder(_new P1Decoder(_tagSet.get(), _nameFeatureTypesArray.get(), _nameWeights.get())),
	_descWeights(_new DTFeature::FeatureWeightMap(500009)),
	_descDecoder(_new P1Decoder(_tagSet.get(), _descFeatureTypesArray.get(), _descWeights.get())),
	_pronWeights(_new DTFeature::FeatureWeightMap(500009)),
	_pronDecoder(_new P1Decoder(_tagSet.get(), _pronFeatureTypesArray.get(), _pronWeights.get())),
	_observation(_new DTCorefObservation(_infoMap.get()))
{
	
	std::string name_file = ParamReader::getRequiredParam("dt_name_coref_model_file") + "-rank";
	DTFeature::readWeights(*_nameWeights, name_file.c_str(), DTCorefFeatureType::modeltype);

	std::string desc_file = ParamReader::getRequiredParam("dt_coref_model_file") + "-rank";
	DTFeature::readWeights(*_descWeights, desc_file.c_str(), DTCorefFeatureType::modeltype);

	std::string pron_file = ParamReader::getRequiredParam("dt_pron_model_file") + "-rank";
	DTFeature::readWeights(*_pronWeights, pron_file.c_str(), DTCorefFeatureType::modeltype);	

	_nameFeatureTypesArray[_tagSet->getNoneTagIndex()] = _nameNoneFeatureTypes.get();
	_nameFeatureTypesArray[_tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol())] = _nameLinkFeatureTypes.get();

	_descFeatureTypesArray[_tagSet->getNoneTagIndex()] = _descNoneFeatureTypes.get();
	_descFeatureTypesArray[_tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol())] = _descLinkFeatureTypes.get();

	_pronFeatureTypesArray[_tagSet->getNoneTagIndex()] = _pronNoneFeatureTypes.get();
	_pronFeatureTypesArray[_tagSet->getTagIndex(DescLinkFeatureFunctions::getLinkSymbol())] = _pronLinkFeatureTypes.get();

	_name_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_name_coref_rank_overgen_threshold", 0);
	_desc_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_coref_rank_overgen_threshold", 0);
	_pron_overgen_threshold = ParamReader::getOptionalFloatParamWithDefaultValue("dt_pron_rank_overgen_threshold", 0);

	CompositeMentionGroupConstraint *descOnlyConstraints = _new CompositeMentionGroupConstraint();
	if (ParamReader::isParamTrue("block_headword_clashes"))
		descOnlyConstraints->add(MentionGroupConstraint_ptr(_new HeadWordClashConstraint()));
	_descOnlyConstraints = MentionGroupConstraint_ptr(descOnlyConstraints);
}

void CombinedP1RankingMerger::merge(MentionGroupList& groups, LinkInfoCache& cache) {
	const DocTheory *docTheory = cache.getDocTheory();
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		std::vector<const Mention*> names;
		std::vector<const Mention*> descriptors;
		std::vector<const Mention*> pronouns;

		MentionSet *mentSet = docTheory->getSentenceTheory(i)->getMentionSet();
		_observation->resetForNewSentence(mentSet);

		for (int j = 0; j < mentSet->getNMentions(); j++) {
			Mention *mention = mentSet->getMention(j);
			if (mention->getMentionType() == Mention::NAME || mention->getMentionType() == Mention::NEST) {
				names.push_back(mention);
			} else if (mention->getMentionType() == Mention::DESC && mention->getEntityType().isRecognized()) {
				descriptors.push_back(mention);
			} else if (mention->getMentionType() == Mention::PRON && WordConstants::isLinkingPronoun(mention->getNode()->getHeadWord())) {
				pronouns.push_back(mention);
			}
		}

		BOOST_FOREACH(const Mention* ment, names) {
			processMention(ment, _nameDecoder.get(), _name_overgen_threshold, groups, cache);
		}
		BOOST_FOREACH(const Mention* ment, descriptors) {
			processMention(ment, _descDecoder.get(), _desc_overgen_threshold, groups, cache);
		}
		BOOST_FOREACH(const Mention* ment, pronouns) {
			processMention(ment, _pronDecoder.get(), _pron_overgen_threshold, groups, cache);
		}
	}

}

bool CombinedP1RankingMerger::isRepresentativeMention(const Mention *linkMention, MentionGroup_ptr group) {
	// If linkMention is name, it is representative if it's the earliest one
	if (linkMention->getMentionType() == Mention::NAME) {
		for (MentionGroup::const_iterator iter = group->begin(); iter != group->end(); ++iter) {
			const Mention *ment = *iter;
			if (ment == linkMention)
				return true;
			if (ment->getMentionType() == Mention::NAME)
				return false;
		}
		return false;
	}

	// If it's not a name, it cannot be representative if there is a name in the group
	for (MentionGroup::const_iterator iter = group->begin(); iter != group->end(); ++iter) {
		const Mention *ment = *iter;
		if (ment->getMentionType() == Mention::NAME)
			return false;
	}

	// If no name in the group, linkMention is representative if it is the earliest valid mention
	for (MentionGroup::const_iterator iter = group->begin(); iter != group->end(); ++iter) {
		const Mention *ment = *iter;
		Mention::Type type = ment->getMentionType();

		// Must be a type that the P1Ranker recognizes
		if (type != Mention::NAME && type != Mention::NEST && type != Mention::DESC && type != Mention::PRON)
			continue;

		if (linkMention == ment)
			return true;

		return false;
	}
	
	return false;
}

void CombinedP1RankingMerger::processMention(const Mention *linkMention, P1Decoder *decoder, double overgen_threshold, MentionGroupList& groups, LinkInfoCache& cache) {
	std::set<RankingRecord> scores;

	MentionGroup_ptr group1;
	for (MentionGroupList::iterator it1 = groups.begin(); it1 != groups.end(); ++it1) {
		if ((*it1)->contains(linkMention)) 
			group1 = *it1;
	}

	if (group1.get() == NULL) 
		return;

	if (!isRepresentativeMention(linkMention, group1))
		return;	

	boost::scoped_ptr<EntitySet> entitySet(buildFakeEntitySet(groups, cache.getDocTheory(), linkMention));
	_observation->resetForNewDocument(entitySet.get());

	if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
		SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------\n";
		SessionLogger::dbg("MentionGroups_p1ranking") << "PROCESSING Mention #" << linkMention->getUID() << ": "
				<<  linkMention->getNode()->toTextString() << "\n";
	}

	// score the no-link option
	DTNoneCorefObservation *noneObs = static_cast<DTNoneCorefObservation*>(_observation.get());
	noneObs->populate(linkMention->getUID());
	double score = decoder->getScore(noneObs, _tagSet->getNoneTagIndex()) - overgen_threshold;
	scores.insert(RankingRecord(_tagSet->getNoneTagIndex(), boost::shared_ptr<MentionGroup>(), score));
	if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
		SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------";
		SessionLogger::dbg("MentionGroups_p1ranking") << "CONSIDERING NO-LINK option";
		SessionLogger::dbg("MentionGroups_p1ranking") << decoder->getDebugInfo(noneObs, _tagSet->getNoneTagIndex());
		SessionLogger::dbg("MentionGroups_p1ranking") << "ADJUSTED NO-LINK SCORE: " << score;
	}

	// score linking against each of the other MentionGroups
	for (MentionGroupList::iterator it2 = groups.begin(); it2 != groups.end(); ++it2) {
		if (*it2 == group1)
			continue;
	
		if (!isLegalMerge(**it2, *group1))
			continue;

		MentionGroup_ptr group2 = *it2;
		const Mention *firstMention = *group2->begin();

		// Skip if the first mention of the group occurs after linkMention's sentence.
		if (firstMention->getUID().sentno() > linkMention->getUID().sentno())
			continue;

		if ((firstMention->getUID().sentno() == linkMention->getUID().sentno())) {
			int cmp = compareMentionTypes(linkMention, firstMention);
			if ((cmp == -1) || (cmp == 0 && firstMention->getUID() > linkMention->getUID()))
				continue;
		}

		// The group's entity type must match linkMention's.
		if (linkMention->getEntityType().isRecognized() && linkMention->getEntityType() != group2->getEntityType() && linkMention->getEntityType().getName() != Symbol(L"POG") && group2->getEntityType().getName() != Symbol(L"POG"))
			continue;

		int link_index = _tagSet->getLinkTagIndex();
		Entity *linkEntity = entitySet->getEntityByMentionWithoutType(firstMention->getUID());
		int hobbs_distance = getHobbsDistance(linkMention, group2, cache.getDocTheory());

		_observation->populate(linkMention->getUID(), linkEntity->getID(), hobbs_distance);
		scores.insert(RankingRecord(link_index, group2, decoder->getScore(_observation.get(), link_index)));
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
			SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------";
			SessionLogger::dbg("MentionGroups_p1ranking") << "CONSIDERING LINK TO MentionGroup " << group2->toString();
			SessionLogger::dbg("MentionGroups_p1ranking") << decoder->getDebugInfo(_observation.get(), link_index);
		}
	}

	// Iterate through the options starting with the highest score until we find a 
	// valid merge or reach the no-link option.
	for (std::set<RankingRecord>::iterator it3 = scores.begin(); it3 != scores.end() && (*it3).tag_index != _tagSet->getNoneTagIndex(); ++it3) {
		MentionGroup_ptr group2 = (*it3).group;
		if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
			const Mention *m1 = *(group1->begin());
			const Mention *m2 = *(group2->begin());
			SessionLogger::dbg("MentionGroups_shouldMerge") << "Testing merge of " 
					<< m1->getUID() << " (" << m1->toCasedTextString() << ") and " 
					<< m2->getUID() << " (" << m2->toCasedTextString() << ")";
			SessionLogger::dbg("MentionGroups_shouldMerge") << "P1 RANKING " << /*Mention::getTypeString(_targetType) <<*/ " MODEL LINK FOUND";
		}
		if (_constraints.get() != 0 && _constraints->violatesMergeConstraint(*group1, *group2, cache))
			continue;
		if (_descOnlyConstraints != 0 && 
			(linkMention->getMentionType() == Mention::PART || linkMention->getMentionType() == Mention::DESC) &&
			_descOnlyConstraints->violatesMergeConstraint(*group1, *group2, cache)) 
		{
			continue;
		}
			
		SessionLogger::dbg("MentionGroupMerger") << "Merge identified.";
		group1->merge(*group2, this, (*it3).score);
		groups.erase(std::remove(groups.begin(), groups.end(), group2), groups.end());
		break;
	}	

	if (SessionLogger::dbg_or_msg_enabled("MentionGroups_p1ranking")) {
		SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------";
		SessionLogger::dbg("MentionGroups_p1ranking") << "DONE PROCESSING MENTION #" << linkMention->getUID();
		SessionLogger::dbg("MentionGroups_p1ranking") << "-------------------------------";
	}

	_infoMap->cleanUpAfterDocument();
}

bool CombinedP1RankingMerger::shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const {
	SessionLogger::warn("MentionGroups_shouldMerge") << "CombinedP1RankingMerger::shouldMerge() should never be called.";
	return false;
}


EntitySet* CombinedP1RankingMerger::buildFakeEntitySet(MentionGroupList& groups, const DocTheory *docTheory, const Mention* linkMention) const {
	int n_sentences = docTheory->getNSentences();
	int sent_no = linkMention->getUID().sentno();
	EntitySet *result = _new EntitySet(docTheory->getNSentences());
	
	// Load all MentionSets into the EntitySet to mimic the state of the EntitySet at the 
	// current sentence. Copy only the last MentionSet because it will be deleted by the EntitySet 
	// destructor.
	for (int i = 0; i < sent_no; i++) {
		result->loadDoNotCopyMentionSet(docTheory->getSentenceTheory(i)->getMentionSet());
	}
	result->loadMentionSet(docTheory->getSentenceTheory(sent_no)->getMentionSet());

	// Add a new Entity for each MentionGroup.
	for (MentionGroupList::iterator it1 = groups.begin(); it1 != groups.end(); ++it1) {
		MentionGroup_ptr group = (*it1);	
		bool first_mention = true;
		for (MentionGroup::const_iterator it2 = group->begin(); it2 != group->end(); ++it2) {
			const Mention *ment = *it2;
			// Note: MentionGroups are sorted by uid, so so any remaining mentions will be after this one.
			if (ment->getUID().sentno() > sent_no) 
				break;
			if (ment->getUID().sentno() == sent_no) {
				int cmp = compareMentionTypes(linkMention, ment);
				if ((cmp == -1) || (cmp == 0 && ment->getUID() > linkMention->getUID()))
					continue;
			}
			if (first_mention) {
				result->addNew(ment->getUID(), group->getEntityType());
				first_mention = false;
			} else {
				result->add(ment->getUID(), result->getNEntities()-1);
			}
			_infoMap->addMentionInformation(ment);
		}
	}

	return result;
}

int CombinedP1RankingMerger::compareMentionTypes(const Mention *ment1, const Mention *ment2) const {
	Mention::Type type1 = ment1->getMentionType();
	Mention::Type type2 = ment2->getMentionType();
	if (type1 == type2)
		return 0;
	if ((type1 == Mention::NAME) ||
		(type1 == Mention::DESC && (type2 == Mention::PRON || type2 == Mention::PART)))
	{
		return -1;
	}
	return 1;
}

int CombinedP1RankingMerger::getHobbsDistance(const Mention *mention, MentionGroup_ptr group, const DocTheory *docTheory) const {
	
	if (mention->getMentionType() != Mention::PRON)
		return -1;
	
	const int MAX_CANDIDATES = 50;
	HobbsDistance::SearchResult hobbsCandidates[MAX_CANDIDATES];

	std::vector<const Parse*> prevParses;
	for (int i = 0; i < mention->getUID().sentno(); i++) {
		prevParses.push_back(docTheory->getSentenceTheory(i)->getFullParse());
	}

	int nHobbsCandidates = HobbsDistance::getCandidates(mention->getNode(), prevParses, hobbsCandidates, MAX_CANDIDATES);
	for (int i = 0; i < nHobbsCandidates; i++) {
		const MentionSet *mset = docTheory->getSentenceTheory(hobbsCandidates[i].sentence_number)->getMentionSet();
		const Mention *ment = mset->getMentionByNode(hobbsCandidates[i].node);		
		if (ment != NULL && group->contains(ment))
			return i;
	}
	return -1;
}
