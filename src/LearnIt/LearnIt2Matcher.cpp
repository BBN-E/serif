#include "Generic/common/leak_detection.h"

#include <limits>
#include <vector>
#include <map>
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include "boost/tuple/tuple.hpp"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/foreach_pair.hpp"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "ActiveLearning/alphabet/FromDBFeatureAlphabet.h"
#include "LearnIt/db/LearnItDB.h"
#include "LearnIt/MatchInfo.h"
#include "LearnIt/SlotFiller.h"
#include "LearnIt/Target.h"
#include "LearnIt/features/Feature.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/features/BrandyPatternFeature.h"
#include "LearnIt/features/PropPatternFeatureCollection.h"
#include "LearnIt/features/TextPatternFeatureCollection.h"
//#include "LearnIt/features/RelationFeatureCollection.h"
//#include "LearnIt/features/EventFeatureCollection.h"

#include "LearnIt2.h"
#include "LearnIt2Matcher.h"


using std::vector;
using std::wstringstream;
using std::map;
using std::pair;
using boost::make_shared;

LearnIt2Matcher::LearnIt2Matcher(LearnItDB_ptr db) 
: _record_source(ParamReader::getOptionalTrueFalseParamWithDefaultVal(
			"learnit2_record_features", false))
{
	FromDBFeatureAlphabet_ptr sentAlphabet =
		db->getAlphabet(LearnIt2::SENTENCE_ALPHABET_INDEX);
	_alphabet = boost::dynamic_pointer_cast<FeatureAlphabet>(sentAlphabet);
	//FeatureAlphabet_ptr slotAlphabet = db->getAlphabet(LearnIt2::SLOT_ALPHABET_INDEX);

	vector<Target_ptr> targets = db->getTargets();
	if (targets.size() < 1) {
		throw UnexpectedInputException("LearnIt2Matcher::LearnIt2Matcher",
			"Database must have at least one target!");
	}
	if (targets.size() > 1) {
		throw UnexpectedInputException("LearnIt2Matcher::LearnIt2Matcher",
			"LearnIt2 databases currently cannot have more than one "
			"target!");
	}
	_target = targets[0];
	_name = _target->getName();

	_prob_threshold = db->probabilityThreshold();

	// alphabet 0 is the sentence feature alphabet
	_nonTriggerFeatures = Feature::getNonTriggerFeatures(db->getAlphabet(0), _target,
		ParamReader::getRequiredFloatParam("learnit2_feature_weight_threshold"),
		true);

	double feature_threshold = ParamReader::getRequiredFloatParam(
									"learnit2_feature_weight_threshold");
	_textFeatures = TextPatternFeatureCollection::createFromTextPatterns(targets[0],
		sentAlphabet, feature_threshold, false);
	_propFeatures = PropPatternFeatureCollection::create(targets[0], sentAlphabet,
		feature_threshold, false);
	_keywordInSentenceFeatures = 
		TextPatternFeatureCollection::createFromKeywordInSentencePatterns(
		targets[0], sentAlphabet, feature_threshold, false);
/*	_eventFeatures =
		EventFeatureCollection::create(targets[0], *sentAlphabet,
			feature_threshold, false);
	_relationFeaturees =
		RelationFeatureCollection::create(targets[0], *sentAlphabet,
			feature_threshold, false); */
	initEventAndRelationFeatures(sentAlphabet, feature_threshold);
}

void LearnIt2Matcher::initEventAndRelationFeatures(
		const FromDBFeatureAlphabet_ptr alphabet, double threshold) 
{
	FeatureList_ptr tmpFeatures =
		Feature::featuresOfClass(alphabet, L"RelationFeature", _target, 
				threshold, false);
	BOOST_FOREACH(const Feature_ptr& f, *tmpFeatures) {
		BrandyPatternFeature_ptr bpfp = 
			boost::dynamic_pointer_cast<BrandyPatternFeature>(f);
		if (bpfp) {
			_relationFeatures.push_back(bpfp);
		} else {
			throw UnrecoverableException("LearnIt2Matcher::initEventAndRelationFeatures",
					"InternalInconsistency: features cannot be cast to BrandyPatternFeatures");
		}
	}
	tmpFeatures = Feature::featuresOfClass(alphabet, L"EventFeature", _target, 
			threshold, false);
	BOOST_FOREACH(const Feature_ptr& f, *tmpFeatures) {
		BrandyPatternFeature_ptr bpfp = 
			boost::dynamic_pointer_cast<BrandyPatternFeature>(f);
		if (bpfp) {
			_eventFeatures.push_back(bpfp);
		} else {
			throw UnrecoverableException("LearnIt2Matcher::initEventAndRelationFeatures",
					"InternalInconsistency: features cannot be cast to BrandyPatternFeatures");
		}
	}
	std::cout << "Loaded " << _relationFeatures.size() << " relation features and " 
		<< _eventFeatures.size() << " event features." << std::endl;
}	

const Target_ptr LearnIt2Matcher::target() const {
	return _target;
}

const std::wstring& LearnIt2Matcher::name() const {
	return _name;
}

// see LearnIt2Matcher::match
LearnIt2Matcher::SlotFillerNormalizer LearnIt2Matcher::buildSlotFillerNormalizer(
	const SlotFillersVector& slotFillers) 
{
	SlotFillerNormalizer slotFillerNormalizer;

	for (size_t slot_num = 0; slot_num < slotFillers.size(); ++slot_num) {
		// we should only only need to normalize for those slots which are using
		// best names because for the others we are interested in the exact
		// matched string
		if (_target->getSlotConstraints(static_cast<int>(slot_num))->useBestName()) {
			bool found = false;
			for (size_t i = 0; i<slotFillers[slot_num]->size(); ++i) {
				SlotFiller_ptr filler = (*slotFillers[slot_num])[i];
				SlotFiller_ptr norm;
				// for each slot filler, if something coreferent with it is
				// already in the map, we map it to the same thing. Otherwise,
				// we map it to itself..
				bool quickBreakFix = false;
				BOOST_FOREACH_PAIR(SlotFiller_ptr key, SlotFiller_ptr val, 
									slotFillerNormalizer) 
				{
					if (!quickBreakFix && key->sameReferent(*filler)) {
						norm = val;
						quickBreakFix = true;
					}
				}
				if (norm) {
					slotFillerNormalizer.insert(std::make_pair(filler, norm));
				} else {
					slotFillerNormalizer.insert(std::make_pair(filler, filler));
				}
			}
		} 
	}

	return slotFillerNormalizer;
}

// it's assumed that we already know slotsFillerMap has a sufficient
// slots set
MatchInfo::PatternMatch slotFillerMapToPatternMatch(
	const Target_ptr target, const SentenceTheory* sent_theory,
	const SlotFillerMap& slotFillerMap, double score, std::wstring source) 
{
	int earliest_token = std::numeric_limits<int>::max();
	int latest_token = -1;
	std::vector<SlotFiller_ptr> slotFillersVector;

	for (int i = 0; i < target->getNumSlots(); ++i) {
		SlotFillerMap::const_iterator it = slotFillerMap.find(i);
		if (it!=slotFillerMap.end()) {
			SlotFiller_ptr filler = it->second;
			slotFillersVector.push_back(filler);
			if (filler->getStartToken() < earliest_token) {
				earliest_token = filler->getStartToken();
			}
			if (filler->getEndToken() > latest_token) {
				latest_token = filler->getEndToken();
			}
		} else {
			slotFillersVector.push_back(boost::make_shared<SlotFiller>(
				SlotConstraints::UNFILLED_OPTIONAL));
		}
	}

	return MatchInfo::PatternMatch(slotFillersVector, earliest_token, 
									latest_token, score, source);
}

void LearnIt2Matcher::normalizeMatch(SlotFillerMap& match, 
					const SlotFillerNormalizer& normalizer) 
{
	BOOST_FOREACH_PAIR(int slot_num, SlotFiller_ptr& filler, match) {
		if (_target->getSlotConstraints(slot_num)->useBestName()) {
			SlotFillerNormalizer::const_iterator it = normalizer.find(filler);
			if (it != normalizer.end()) {
				filler = it->second;
			}
		}
	}
}

LearnIt2Matcher::MatchRecord::MatchRecord(bool track_features) 
	: triggered(false), score(0.0), matchedFeatures() 
{
	if (track_features) {
		matchedFeatures = boost::make_shared<FeatureScoreList>();
	}
}

LearnIt2Matcher::MatchRecord::MatchRecord(Feature_ptr feature,
		 FeatureAlphabet_ptr alphabet) 
	: triggered(false), score(0.0) , matchedFeatures()
{
	matchedFeatures = boost::make_shared<FeatureScoreList>();
	matchedByFeature(feature, alphabet);
}

void LearnIt2Matcher::MatchRecord::matchedByFeature(Feature_ptr feature,
		FeatureAlphabet_ptr alphabet) {
	triggered = triggered || feature->trigger();
	score += feature->weight();
	if (matchedFeatures) {
		matchedFeatures->push_back(
			boost::make_tuple(feature->index(), 
				alphabet->getFeatureName(feature->index()), feature->weight()));
	}
}

std::wstring LearnIt2Matcher::MatchRecord::sourceString() const {
	if (matchedFeatures) {
		wstringstream str;
		str << L"L2(";
		BOOST_FOREACH(const FeatureScore& val, *matchedFeatures) {
			str << val.get<0>() << L":" << val.get<1>() 
				<< L"=" << std::setprecision(3) 
				<< val.get<2>() << L"; ";
		}
		str << L")";
		return str.str();
	} else {
		return L"";
	}
}

MatchProvenance_ptr LearnIt2Matcher::MatchRecord::createMatchProvenance() const {
	MatchProvenance_ptr prov = make_shared<MatchProvenance>();

	BOOST_FOREACH(const FeatureScore& val, *matchedFeatures) {
		wstringstream featStr;

		featStr << L"L2(" << val.get<1>() << L")";
		prov->push_back(featStr.str());
	}
	return prov;
}

void LearnIt2Matcher::processFeatureMatch(Feature_ptr feature, SlotFillerMap& featureMatch,
	const SlotFillerNormalizer& slotFillerNormalizer, ScoredMatches& returnMatches) 
{
	normalizeMatch(featureMatch, slotFillerNormalizer);
	ScoredMatches::iterator it = returnMatches.find(featureMatch);
	if (it == returnMatches.end()) {
		returnMatches.insert(make_pair(featureMatch, 
					MatchRecord(feature, _alphabet)));
	} else {
		it->second.matchedByFeature(feature, _alphabet);
	}
}

MatchInfo::PatternMatches LearnIt2Matcher::match(const DocTheory* dt, 
	Symbol docid, SentenceTheory* sent_theory)  {
		AlignedDocSet_ptr doc_set = boost::shared_ptr<AlignedDocSet>();
		doc_set->loadDocTheory(LanguageVariant::getLanguageVariant(),dt);
		return match(doc_set,docid,sent_theory);
}


MatchInfo::PatternMatches LearnIt2Matcher::match(const AlignedDocSet_ptr doc_set, 
	Symbol docid, SentenceTheory* sent_theory) 
{
	MatchInfo::PatternMatches matches;
	SlotFillersVector slotFillers;
	SlotFiller::getSlotFillers(doc_set, sent_theory, _target, slotFillers, doc_set->getDefaultLanguageVariant());

	// if it's completely impossible for us to match because there's 
	// no candidate to fill some required slot, don't worry even bother
	// matching features
	if (!_target->satisfiesSufficientSlots(slotFillers)) {
		return matches;
	}

	// we don't care about mentions, only things that resolve to the same
	// tuple of bestnames, so we need to normalize SlotFillers referring
	// to the same things. We do this arbitrarily by taking the first one
	// with each referent..
	SlotFillerNormalizer slotFillerNormalizer = 
		buildSlotFillerNormalizer(slotFillers);

	vector<SlotFillerMap> matchesForFeature;
	// first of pair is if seen with triggering feature; second is score
	ScoredMatches returnMatches;
	/*BOOST_FOREACH(SentenceMatchableFeature_ptr feature, _triggerFeatures) {
		matchesForFeature.clear();
		feature->matchesSentence(slotFillers, dt, docid,
			sent_theory->getSentNumber(), *mentionToEntityMap, 
			matchesForFeature);
		BOOST_FOREACH(SlotFillerMap& featureMatch, matchesForFeature) {
			normalizeMatch(_target, featureMatch, slotFillerNormalizer);
			ScoredMatches::iterator it = returnMatches.find(featureMatch);
			if (it == returnMatches.end()) {
				returnMatches.insert(make_pair(featureMatch, MatchRecord(feature)));
			} else {
				it->second.matchedByFeature(feature);
			}
		}
	}*/

	for (FeatureCollectionIterator<BrandyPatternFeature> it = 
		_propFeatures->applicableFeatures(slotFillers, doc_set->getDefaultDocTheory(), 
			sent_theory->getSentNumber()); it; ++it) 
	{
		matchesForFeature.clear();
		(*it)->matchesSentence(slotFillers, doc_set, docid,
			sent_theory->getSentNumber(), 
			matchesForFeature);
		removeDuplicateMatches(matchesForFeature);
		BOOST_FOREACH(SlotFillerMap& featureMatch, matchesForFeature) {
			processFeatureMatch(*it, featureMatch, slotFillerNormalizer,
			returnMatches);
		}
	}

	for (FeatureCollectionIterator<BrandyPatternFeature> it = 
		_textFeatures->applicableFeatures(slotFillers, doc_set->getDefaultDocTheory(), 
			sent_theory->getSentNumber()); it; ++it) 
	{
		matchesForFeature.clear();
		(*it)->matchesSentence(slotFillers, doc_set, docid,
			sent_theory->getSentNumber(), 
			matchesForFeature);
		removeDuplicateMatches(matchesForFeature);
		BOOST_FOREACH(SlotFillerMap& featureMatch, matchesForFeature) {
			processFeatureMatch(*it, featureMatch, slotFillerNormalizer,
			returnMatches);
		}
	}

	for (FeatureCollectionIterator<BrandyPatternFeature> it = 
		_keywordInSentenceFeatures->applicableFeatures(slotFillers, doc_set->getDefaultDocTheory(), 
			sent_theory->getSentNumber()); it; ++it) 
	{
		matchesForFeature.clear();
		(*it)->matchesSentence(slotFillers, doc_set, docid,
			sent_theory->getSentNumber(), 
			matchesForFeature);
		removeDuplicateMatches(matchesForFeature);
		BOOST_FOREACH(SlotFillerMap& featureMatch, matchesForFeature) {
			processFeatureMatch(*it, featureMatch, slotFillerNormalizer,
			returnMatches);
		}
	}

	if (sent_theory->getRelMentionSet()->getNRelMentions() > 0) {
		BOOST_FOREACH(BrandyPatternFeature_ptr feat, _relationFeatures) {
			matchesForFeature.clear();
			feat->matchesSentence(slotFillers, doc_set, docid,
				sent_theory->getSentNumber(),
				matchesForFeature);
			removeDuplicateMatches(matchesForFeature);
			BOOST_FOREACH(SlotFillerMap& featureMatch, matchesForFeature) {
				processFeatureMatch(feat, featureMatch, slotFillerNormalizer,
						returnMatches);
			}
		}
	}
					
	if (sent_theory->getEventMentionSet()->getNEventMentions() > 0) {
		BOOST_FOREACH(BrandyPatternFeature_ptr feat, _eventFeatures) {
			matchesForFeature.clear();
			feat->matchesSentence(slotFillers, doc_set, docid,
				sent_theory->getSentNumber(), 
				matchesForFeature);
			removeDuplicateMatches(matchesForFeature);
			BOOST_FOREACH(SlotFillerMap& featureMatch, matchesForFeature) {
				processFeatureMatch(feat, featureMatch, slotFillerNormalizer,
						returnMatches);
			}
		}
	}
					

	/*for (FeatureCollectionIterator<BrandyPatternFeature> it = 
		_relationFeatures->applicableFeatures(slotFillers, dt, 
			sent_theory->getSentNumber(),*mentionToEntityMap); it; ++it) 
	{
		matchesForFeature.clear();
		(*it)->matchesSentence(slotFillers, dt, docid,
			sent_theory->getSentNumber(), *mentionToEntityMap, 
			matchesForFeature);
		BOOST_FOREACH(SlotFillerMap& featureMatch, matchesForFeature) {
			processFeatureMatch(*it, featureMatch, slotFillerNormalizer,
			returnMatches);
		}
	}

	for (FeatureCollectionIterator<BrandyPatternFeature> it = 
		_eventFeatures->applicableFeatures(slotFillers, dt, 
			sent_theory->getSentNumber(),*mentionToEntityMap); it; ++it) 
	{
		matchesForFeature.clear();
		(*it)->matchesSentence(slotFillers, dt, docid,
			sent_theory->getSentNumber(), *mentionToEntityMap, 
			matchesForFeature);
		BOOST_FOREACH(SlotFillerMap& featureMatch, matchesForFeature) {
			processFeatureMatch(*it, featureMatch, slotFillerNormalizer,
			returnMatches);
		}
	}
*/

	// turn every triggered slot filler map into a feature map
	BOOST_FOREACH_PAIR(const SlotFillerMap& slotFillerMap, MatchRecord& info, 
						returnMatches) 
	{
		if (info.triggered) { // if this matched at least one triggering feature...
			// add prior to each match and then transform scores into
			// probabilities
			// currently we ignore slot score... what is the right thing to do?
			scoreMatchForNonTriggerFeatures(slotFillerMap, 
				info, slotFillers, doc_set, sent_theory->getSentNumber());
			double score = info.score;
			score = 1.0/(1.0 + exp(-score));
			// then check we passed the probability threshold
			if (score > _prob_threshold) {
				Target::SlotSet slotsFilled;
				for (SlotFillerMap::const_iterator slot_num_i = slotFillerMap.begin(); slot_num_i != slotFillerMap.end(); ++slot_num_i) {
					slotsFilled.push_back(slot_num_i->first);
				}
				if (_target->slotsSuffice(slotsFilled) 
					&& _target->checkCoreferentArguments(slotFillerMap)) 
				{
					std::wstring source=L"";

					if (_record_source) {
						source = info.sourceString();
					}

					MatchInfo::PatternMatch m = 
						slotFillerMapToPatternMatch(_target, 
						sent_theory, slotFillerMap,	score, source);

					m.provenance = info.createMatchProvenance();
					matches.push_back(m);
				}
			}
		}
	}

	return matches;
}

void LearnIt2Matcher::scoreMatchForNonTriggerFeatures(
	const SlotFillerMap& match, MatchRecord& matchRecord, 
	const SlotFillersVector& slotFillersVector, const AlignedDocSet_ptr doc_set, 
	int sent_no) 
{
	BOOST_FOREACH(MatchMatchableFeature_ptr feature, _nonTriggerFeatures) {
		if (feature->matchesMatch(match, slotFillersVector,
			doc_set->getDefaultDocTheory(), sent_no))
		{
			matchRecord.matchedByFeature(feature, _alphabet);
		}
	}
}

void LearnIt2Matcher::removeDuplicateMatches(std::vector<SlotFillerMap>& matches) {
	std::vector<SlotFillerMap> ret;

	for (size_t firstMatchIdx = 0; firstMatchIdx < matches.size(); ++firstMatchIdx) {
		const SlotFillerMap& first = matches[firstMatchIdx];
		bool add = true;
		for (size_t secondMatchIdx = 0; secondMatchIdx<firstMatchIdx; ++secondMatchIdx) {
			const SlotFillerMap& second = matches[secondMatchIdx];
			bool same = true;

			if (first.size() != second.size()) {
				throw UnexpectedInputException("LearnIt2Matcher::removeDuplicateMatches",
						"Mismatched number of slots");
			}

			for (size_t slot_idx=0; slot_idx<first.size(); ++slot_idx) {
				SlotFillerMap::const_iterator firstProbe = first.find(static_cast<int>(slot_idx));
				SlotFillerMap::const_iterator secondProbe = second.find(static_cast<int>(slot_idx));

				if (firstProbe == first.end() || secondProbe==second.end()) {
					throw UnexpectedInputException("LearnIt2Matcher::removeDuplicateMatches",
							"Slot indices don't align");
				}

				const SlotFiller& firstSlot = *firstProbe->second;
				const SlotFiller& secondSlot = *secondProbe->second;

				if (firstSlot.filled() && secondSlot.filled()) {
					if (!firstSlot.sameReferent(secondSlot)) {
						same = false;
						break;
					}
				} else if (firstSlot.filled() != secondSlot.filled()) {
					same = false;
					break;
				}
			}

			if (same) {
				add = false;
				break;
			}
		}
		if (add) {
			ret.push_back(first);
		}
	}
	matches = ret;
}
