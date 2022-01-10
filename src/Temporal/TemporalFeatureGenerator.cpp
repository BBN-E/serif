#include "Generic/common/leak_detection.h"
#include "TemporalFeatureGenerator.h"

#include <string>
#include <sstream>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/foreach_pair.hpp"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/patterns/QueryDate.h"
#include "English/timex/en_TemporalNormalizer.h"
#include "Generic/common/TimexUtils.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "ActiveLearning/EventUtilities.h"
#include "Temporal/features/TemporalRelationFeature.h"
#include "Temporal/features/TemporalEventFeature.h"
#include "Temporal/features/TemporalWordInSentenceFeature.h"
#include "Temporal/features/ParticularFeatures.h"
#include "Temporal/features/DatePropSpine.h"
#include "Temporal/features/TemporalBiasFeature.h"
//#include "Temporal/features/RelationSourceFeature.h"
#include "Temporal/features/InstanceSourceFeature.h"
#include "Temporal/features/TemporalAdjacentWordsFeature.h"
#include "Temporal/features/TemporalTreePathFeature.h"
#include "Temporal/features/InterveningDatesFeature.h"
#include "Temporal/features/PropFeatures.h"
#include "TemporalInstance.h"
#include "TemporalAttribute.h"
#include "FeatureMap.h"

using std::vector;
using std::wstring;
using std::wstringstream;
using boost::make_shared;

TemporalFeatureVectorGenerator::TemporalFeatureVectorGenerator(FeatureMap_ptr featureMap) :

_tempNormalizer(make_shared<EnglishTemporalNormalizerFactory>()->build()),
_featureMap(featureMap) 
,_adjacentWords(make_shared<TemporalAdjacentWordsFeatureProposer>()) ,
	_treePaths(make_shared<TemporalTreePathFeatureProposer>())
{}

TemporalFV_ptr TemporalFeatureVectorGenerator::fv(const DocTheory* dt, int sn, 
												  const TemporalInstance& inst) 
{
	wstring docDateString;
	wstring dummy;

	_tempNormalizer->normalizeDocumentDate(dt, docDateString, dummy);

	if (docDateString == L"XXXX-XX-XX") {
		wstringstream err;
		err << "Could not resolve document date for " 
			<< dt->getDocument()->getName();
		throw UnexpectedInputException("TemporalFeatureVectorGenerator::fv", err);
	}

	vector<TemporalFeature_ptr> fv;
	const SentenceTheory* st = dt->getSentenceTheory(sn);

	addTimexShapeFeatures(inst, fv);
	addRelationFeatures(dt, sn, inst, fv);
	addEventFeatures(dt, sn, inst, fv);
	addWordFeatures(inst, st, fv);
	addPropSpines(inst, st, fv);
	//RelationSourceFeature::addFeaturesToInstance(inst, fv);
	InstanceSourceFeature::addFeaturesToInstance(inst, fv);
	_adjacentWords->addApplicableFeatures(inst, st, fv);
	_treePaths->addApplicableFeatures(inst, st, fv);
	PropAttWord::addToFV(inst, dt, sn, fv);
	fv.push_back(make_shared<TemporalBiasFeature>(inst.attribute()->type()));
	fv.push_back(make_shared<TemporalBiasFeature>(
				inst.attribute()->type(), inst.relation()->get_name()));
	InterveningDatesFeature::addFeaturesToInstance(inst, st, fv);


	//addRelativeToDocFeatures(*fv, inst);

	TemporalFV_ptr temporalFV = make_shared<TemporalFV>();
	BOOST_FOREACH(const TemporalFeature_ptr& feat, fv) {
		temporalFV->push_back(_featureMap->index(feat));
	}

	return temporalFV;
}

void TemporalFeatureVectorGenerator::addTimexShapeFeatures(
	const TemporalInstance& inst, vector<TemporalFeature_ptr>& fv)
{
	const ValueMention* vm = inst.attribute()->valueMention();
	if (vm->isTimexValue()) {
		Symbol date = vm->getDocValue()->getTimexVal();
		if (!date.is_null()) {
			wstring dateStr = date.to_string();
			wstringstream featureName;

			if (TimexUtils::isParticularDay(dateStr)) {
				fv.push_back(make_shared<ParticularDayFeature>());
				fv.push_back(make_shared<ParticularDayFeature>(
										inst.relation()->get_name()));
			} else if (TimexUtils::isParticularMonth(dateStr)) {
				fv.push_back(make_shared<ParticularMonthFeature>());
				fv.push_back(make_shared<ParticularMonthFeature>(
										inst.relation()->get_name()));
			} else if (TimexUtils::isParticularYear(dateStr)) {
				fv.push_back(make_shared<ParticularYearFeature>());
				fv.push_back(make_shared<ParticularYearFeature>(
										inst.relation()->get_name()));
			}
		} else {
			throw UnexpectedInputException(
				"TemporalFeatureVectorGenerator::addtimexShapeFeatures",
				"Date has null TIMEX value; this should be impossible");
		}
	}
}

void TemporalFeatureVectorGenerator::addRelationFeatures(
	const DocTheory* dt, unsigned int sn,
	const TemporalInstance& inst, vector<TemporalFeature_ptr>& fv)
{
	const RelMentionSet* relations = dt->getSentenceTheory(sn)->getRelMentionSet();
	const EntitySet* es = dt->getEntitySet();

	for (int i =0; i < relations->getNRelMentions(); ++i) {
		const RelMention* rel = relations->getRelMention(i);

		if (rel->getTimeArgument() ==  inst.attribute()->valueMention()
				&& inst.attribute()->valueMention())
		{ // our temporal attribute matches that of the event
			if (TemporalRelationFeature::relMatchesRel(rel, inst.relation(), es)) {
				fv.push_back(make_shared<TemporalRelationFeature>(
							rel->getType(), inst.relation()->get_name()));
			}
		}
	}
}

void TemporalFeatureVectorGenerator::addEventFeatures(
	const DocTheory* dt, unsigned int sn,
	const TemporalInstance& inst, vector<TemporalFeature_ptr>& fv)
{
	const EventMentionSet* events = dt->getSentenceTheory(sn)->getEventMentionSet();
	const EntitySet* es = dt->getEntitySet();

	for (int i =0; i < events->getNEventMentions(); ++i) {
		const EventMention* event = events->getEventMention(i);

		if (TemporalEventFeature::matchesEvent(event, inst, es)) {
			Symbol temporalRoleName = 
				TemporalEventFeature::getTemporalRoleName(event, inst);
			fv.push_back(make_shared<TemporalEventFeature>(
					event->getEventType(), temporalRoleName, 
					inst.relation()->get_name()));

			Symbol anchArg = ALEventUtilities::anchorArg(event);

			if (!anchArg.is_null()) {
				fv.push_back(make_shared<TemporalEventFeature>(event->getEventType(),
							temporalRoleName, inst.relation()->get_name(), anchArg));
			}
		}
	}
}

void TemporalFeatureVectorGenerator::addWordFeatures(const TemporalInstance& inst,
		const SentenceTheory* st, vector<TemporalFeature_ptr>& fv)
{
	addWordFeatures(inst, st->getTokenSequence(), 
		st->getPrimaryParse()->getRoot(), fv);
}

void TemporalFeatureVectorGenerator::addWordFeatures(
		const TemporalInstance& inst, const TokenSequence* ts, 
		const SynNode* node, vector<TemporalFeature_ptr>& fv)
{
	if (node->isPreterminal()) {
		Symbol pos = node->getTag();
		Symbol tok = node->getHeadWord();

		if (TemporalWordInSentenceFeature::isWordFeatureTriggerPOS(pos)) {
			fv.push_back(make_shared<TemporalWordInSentenceFeature>(tok));
			fv.push_back(make_shared<TemporalWordInSentenceFeature>(
				tok, inst.relation()->get_name()));
		}
	} else {
		for (int i=0; i<node->getNChildren(); ++i) {
			addWordFeatures(inst, ts, node->getChild(i), fv);
		}
	}
}

void TemporalFeatureVectorGenerator::addPropSpines(
		const TemporalInstance& inst, const SentenceTheory* st,
		vector<TemporalFeature_ptr>& fv)
{
	const ValueMention* vm = inst.attribute()->valueMention();

	if (vm) {
		vector<DatePropSpineFeature::Spine> spines =
			DatePropSpineFeature::spinesFromDate(vm, st);
		BOOST_FOREACH(const DatePropSpineFeature::Spine& spine, spines) {
			//size_t hsh = 0;
			//DatePropSpineFeature::spineHash(hsh, spine);
			//SpineCountsMap::const_iterator probe = _spineCounts.find(hsh);

			//if (probe != _spineCounts.end() && probe->second > 2) {
				fv.push_back(make_shared<DatePropSpineFeature>(
							spine, inst.relation()->get_name()));
			//}
		}
	}
}

/*void TemporalFeatureVectorGenerator::observe(const TemporalInstance& inst,
		const SentenceTheory* st, const DocTheory* dt)
{
	observeWords(st->getTokenSequence(), st->getPrimaryParse()->getRoot());
	observePropSpines(st);
	_adjacentWords->observe(st);
	_treePaths->observe(inst, st);
}

void TemporalFeatureVectorGenerator::observeWords(const TokenSequence* ts,
		const SynNode* node)
{
	if (node->isPreterminal()) {
		Symbol pos = node->getTag();
		Symbol tok = node->getHeadWord();

		if (TemporalWordInSentenceFeature::isWordFeatureTriggerPOS(pos)) {
			WordCountsMap::iterator probe = _wordCounts.find(tok);

			if (probe != _wordCounts.end()) {
				++probe->second;
			} else {
				_wordCounts.insert(make_pair(tok, 1));
			}
		}
	} else {
		for (int i=0; i<node->getNChildren(); ++i) {
			observeWords(ts, node->getChild(i));
		}
	}
}

void TemporalFeatureVectorGenerator::observePropSpines(const SentenceTheory* st) {
	const ValueMentionSet* vms = st->getValueMentionSet();
	for (int i = 0; i< vms->getNValueMentions(); ++i) {
		const ValueMention* vm = vms->getValueMention(i);
		if (vm->getSentenceNumber() == st->getSentNumber()) {
			const Value* val = vm->getDocValue();
			// use only specific dates, not things like "weekly"
			if (val && vm->isTimexValue() && QueryDate::isSpecificDate(val)) {
				vector<DatePropSpineFeature::Spine> spines =
					DatePropSpineFeature::spinesFromDate(vm, st);
				BOOST_FOREACH(const DatePropSpineFeature::Spine& spine, spines) {
					size_t hsh = 0;
					DatePropSpineFeature::spineHash(hsh, spine);
					SpineCountsMap::iterator probe = _spineCounts.find(hsh);

					if (probe!=_spineCounts.end()) {
						++probe->second;
					} else {
						_spineCounts.insert(make_pair(hsh, 1));
					}
				}
			}
		}
	}
}

void TemporalFeatureVectorGenerator::finishObservations() {
	_goodWords.clear();
	BOOST_FOREACH_PAIR(const Symbol& sym, const unsigned int cnt, _wordCounts) {
		if (cnt > 3) {
			_goodWords.insert(sym);
		}
	}
	_wordCounts.clear();
	SessionLogger::info("good_words") << L"Kept " << _goodWords.size() 
		<< L" words for use in WordInSentence features";
}*/
