#include "Generic/common/leak_detection.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <vector>
#include <cassert>
#include <set>
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>

#include "Generic/common/ParamReader.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/InputUtil.h"
#include <Generic/common/UnicodeUtil.h> 
#include "Generic/theories/Token.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "Generic/patterns/multilingual/AlignmentTable.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include "ActiveLearning/EventUtilities.h"
#include "ActiveLearning/alphabet/MultiAlphabet.h"
#include "ActiveLearning/alphabet/FromDBFeatureAlphabet.h"
#include "LearnIt/db/LearnItDB.h" 
#include "LearnIt/SlotFiller.h"
#include "LearnIt/Target.h"
#include "LearnIt/ProgramOptionsUtils.h"
#include "LearnIt/MainUtilities.h"
#include "LearnIt/MatchInfo.h"
#include "LearnIt/MentionToEntityMap.h"
#include "LearnIt/Instance.h"
#include "LearnIt/Seed.h"
#include "LearnIt/LearnIt2.h"
#include "LearnIt/LearnItPattern.h"
#include "LearnIt/util/FileUtilities.h"
#include "LearnIt/features/Feature.h"
#include "LearnIt/features/BrandyPatternFeature.h"
#include "LearnIt/features/PropPatternFeatureCollection.h"
#include "LearnIt/features/TextPatternFeatureCollection.h"
#include "LearnIt/features/slot_identity/SlotsIdentityFeature.h"
#include "LearnIt/features/slot_identity/SlotsIdentityFeatureCollection.h"
#include "LearnIt/LearnIt2Matcher.h"

#include "LearnIt/pb/instances.pb.h"

//#include <gperftools/profiler.h>

using boost::make_shared;
using boost::dynamic_pointer_cast;

class InstanceFinder {
public:
	InstanceFinder(AlignedDocSet_ptr doc, Symbol docid, bool do_seeds, bool do_patterns,
		bool do_instances, bool do_scored_instances, bool do_features, LearnIt2Matcher_ptr matcher);
	int numSeedMatches() const {return _seed_matches; }
	int numPatternMatches() const {return _pattern_matches; }
	int numInstanceMatches() const {return _instance_matches; }
	int numScoredInstanceMatches() const {return _scored_instance_matches; }
	int numFeatureMatches() const {return _feature_matches; }
	void run(const std::vector<Target_ptr>& targets, 
		const std::vector<Seed_ptr>& seeds,
		const std::vector<LearnItPattern_ptr>& patterns,
		const std::vector<Instance_ptr>& instances,
		const std::vector<SentenceMatchableFeature_ptr>& features,
		const SlotsIdentityFeatureCollection_ptr slotFeatures,
		const PropPatternFeatureCollection_ptr propFeatures,
		const TextPatternFeatureCollection_ptr textFeaturees,
		const TextPatternFeatureCollection_ptr keywordInSentenceFeatures,
		learnit::Instance& out, bool all_patterns);
private:
	void startMatch(learnit::Sentence* outSent);
	void endSentenceMatches(learnit::Instance& out);
	
	void findPatternsInSentence(const std::vector<LearnItPattern_ptr>& patterns, 
				const std::vector<Target_ptr>& targets, const TargetToFillers& targetToFiller,
				learnit::Sentence* out, bool all_patterns);
	void outputPatternMatch(const LearnItPattern_ptr& pattern,
		const MatchInfo::PatternMatch& match, learnit::Sentence* out);

	void findSeedsInSentence(const std::vector<Seed_ptr>& seeds,
		const std::vector<Target_ptr>& targets, Symbol docid,
		int sent_no, const TargetToFillers& targetToFiller, learnit::Sentence* out);
	void findInstancesInSentence(const std::vector<Instance_ptr>& instances,
		const std::vector<Target_ptr>& targets, Symbol docid,
		int sent_no, const TargetToFillers& targetToFiller, learnit::Sentence* out);

	void findScoredInstancesInSentence(const std::vector<Target_ptr>& targets, 
		const DocTheory* doc, Symbol docid, int sent_no, learnit::Sentence* out);


	void findScoredInstancesInSentence(const std::vector<Target_ptr>& targets, 
		const DocTheory* doc, Symbol docid, int sent_no, UTF8OutputStream& out);

	void printSlotMatches(const SentenceTheory* st, ObjectWithSlots_ptr slotsObj,  
		const std::wstring& wrapper, const std::vector<SlotFillerMap> matches,
		learnit::Sentence* out);
	void printSlotMatches(const SentenceTheory* st, Target_ptr target, 
		const std::wstring& wrapper, const std::vector<SlotFillerMap>& matches, 
		learnit::Sentence* out, const std::vector<std::wstring>& forcedNames);
	void relationEventMatches(const SlotFillerMap& slotFillerMap,
		const SentenceTheory* st, std::vector<RelEvMatch>& relationMatches,
		std::vector<RelEvMatch>& eventMatches);

	void findInstancesMatchingFeatures(const std::vector<SentenceMatchableFeature_ptr>& features,
		const SlotsIdentityFeatureCollection_ptr slotFeatures,
		const PropPatternFeatureCollection_ptr propFeatures,
		const TextPatternFeatureCollection_ptr textFeatures,
		const TextPatternFeatureCollection_ptr keywordInSentenceFeatures,
		const std::vector<Target_ptr>& targets, const AlignedDocSet_ptr doc, Symbol docid,
		int sent_no, const TargetToFillers& targetToFiller, learnit::Sentence* out);
	
	static void generateAllSlotFillerMaps(Target_ptr target,
	   const SlotFillerMap& match, const SlotFillersVector& slotFillersVector,
	   std::vector<SlotFillerMap>& matches);


	int _seed_matches;
	int _pattern_matches;
	int _instance_matches;
	int _scored_instance_matches;
	int _feature_matches;
	int _cur_sentence;
	bool _printed_sentence;
	AlignedDocSet_ptr _docSet;
	Symbol _docid;
	SentenceTheory* _stheory;
	bool _do_patterns, _do_seeds, _do_instances, _do_scored_instances, _do_features;
	int _sentences_examined;
	int _bailed;
	LearnIt2Matcher_ptr _matcher;
		
	static std::set<Symbol> overlapIgnoreWords;
	
};

std::set<Symbol> InstanceFinder::overlapIgnoreWords = std::set<Symbol>();

InstanceFinder::InstanceFinder(AlignedDocSet_ptr docSet, Symbol docid, bool do_seeds, 
							   bool do_patterns, bool do_instances, bool do_scored_instances, 
							   bool do_features, LearnIt2Matcher_ptr matcher) :
	_seed_matches(0), _pattern_matches(0), _instance_matches(0), _scored_instance_matches(0), _cur_sentence(0),
	_printed_sentence(false), _do_patterns(do_patterns), _do_seeds(do_seeds),
	_do_instances(do_instances), _do_scored_instances(do_scored_instances), _do_features(do_features), 
	_stheory(0), _feature_matches(0), _docid(docid),
	_sentences_examined(0), _bailed(0)
{
	_matcher = matcher;
	_docSet = docSet;
	
	if (ParamReader::hasParam("overlap_seed_match_filter_word_list")) {
		if (InstanceFinder::overlapIgnoreWords.empty()) {
			SessionLogger::info("IF") << "Loading stopwords.";
			std::set<std::wstring> filterWords = 
				InputUtil::readFileIntoSet(ParamReader::getParam("overlap_seed_match_filter_word_list"), false, true);
			BOOST_FOREACH(std::wstring w, filterWords) {
				InstanceFinder::overlapIgnoreWords.insert(Symbol(w));
			}
			SessionLogger::info("IF") << "Loaded.";
		}
	}
}

void InstanceFinder::run(const std::vector<Target_ptr>& targets, 
		const std::vector<Seed_ptr>& seeds,
		const std::vector<LearnItPattern_ptr>& patterns, 
		const std::vector<Instance_ptr>& instances, 
		const std::vector<SentenceMatchableFeature_ptr>& features,
		const SlotsIdentityFeatureCollection_ptr slotFeatures,
		const PropPatternFeatureCollection_ptr propFeatures,
		const TextPatternFeatureCollection_ptr textFeatures,
		const TextPatternFeatureCollection_ptr keywordInSentenceFeatures,
		learnit::Instance& out,
		bool all_patterns) 
{
	
	//Initialize the learnit pattern pattern matchers
	BOOST_FOREACH(LearnItPattern_ptr pattern, patterns) {
		pattern->makeMatcher(_docSet);
	}
	
	for (_cur_sentence = 0; _cur_sentence < _docSet->getDefaultDocTheory()->getNSentences(); ++_cur_sentence) {
		learnit::Sentence* outSent = out.add_sentence();
		//This precomputes a map that given targetToFillers[target][slot_num] gives you a vector
		//of SlotFiller_ptr objects for that target and slot_num.  (Prevents re-computation)
		_stheory = _docSet->getDefaultDocTheory()->getSentenceTheory(_cur_sentence);
		
		TargetToFillers targetToFiller	= 
			SlotFiller::getAllSlotFillers(_docSet, _docSet->getDocumentName(), _cur_sentence, targets);

		if (_do_patterns) {
			findPatternsInSentence(patterns, targets, targetToFiller, outSent, all_patterns);
		}

		if (_do_seeds) {
			findSeedsInSentence(seeds, targets, _docSet->getDocumentName(), _cur_sentence,
				targetToFiller, outSent);
		}

		if (_do_instances) {
			findInstancesInSentence(instances, targets, _docSet->getDocumentName(), _cur_sentence,
				targetToFiller, outSent);
		}

		if (_do_scored_instances) {
			findScoredInstancesInSentence(targets, _docSet->getDefaultDocTheory(), _docSet->getDocumentName(), _cur_sentence, outSent);
		}

		if (_do_features) {
			findInstancesMatchingFeatures(features, slotFeatures, propFeatures,
				textFeatures, keywordInSentenceFeatures,
				targets, _docSet, _docSet->getDocumentName(), _cur_sentence,
				targetToFiller, outSent);
		}
		endSentenceMatches(out);
	}
}

void InstanceFinder::startMatch(learnit::Sentence* outSent) {
	if (!_printed_sentence) {
		_printed_sentence = true;
		
		//out << L"\t<sentence>\n";
		
		//handle alignment information
		BOOST_FOREACH(LanguageVariant_ptr languageVariant, _docSet->getAlignedLanguageVariants(
			_docSet->getDefaultLanguageVariant())) {
			
			learnit::SentenceAlignment* outAlignment = outSent->add_sentencealignment();
			outAlignment->set_source(UnicodeUtil::toUTF8StdString(_docSet->getDefaultLanguageVariant()->getLanguageString()));
			outAlignment->set_target(UnicodeUtil::toUTF8StdString(languageVariant->getLanguageString()));
			//token alignments
			const SentenceTheory* sentTheory = _docSet->getDocTheory(languageVariant)->getSentenceTheory(_cur_sentence);
			const TokenSequence *tokSeq = sentTheory->getTokenSequence();
			for (int i=0;i<tokSeq->getNTokens();++i) {
				learnit::TokenAlignment* tokenAlignment = outAlignment->add_tokenalignment();
				tokenAlignment->set_sourceindex(i);
				BOOST_FOREACH(int j, _docSet->getAlignedTokens(_docSet->getDefaultLanguageVariant(),
					languageVariant, _cur_sentence, i)) {
							
						tokenAlignment->add_targetindex(j);
				}
			}
			//mention alignments
			const MentionSet* ms = sentTheory->getMentionSet();
			if (ms) {
				for (int i=0; i<ms->getNMentions(); ++i) {
					const Mention* mention = ms->getMention(i);
					const Mention* alignedMention = _docSet->getAlignedMention(
						_docSet->getDefaultLanguageVariant(),languageVariant,mention);

					if (alignedMention) {
						learnit::MentionAlignment* mentionAlignment = outAlignment->add_mentionalignment();
						
						mentionAlignment->mutable_sourcemention()->set_start(mention->getNode()->getStartToken());
						mentionAlignment->mutable_sourcemention()->set_end(mention->getNode()->getEndToken());
						mentionAlignment->mutable_targetmention()->set_start(alignedMention->getNode()->getStartToken());
						mentionAlignment->mutable_targetmention()->set_end(alignedMention->getNode()->getEndToken());
						//MatchInfo::setNameSpanForMention(mention, _docSet->getDefaultDocTheory(), 
						//	mentionAlignment->mutable_sourcemention());
						//MatchInfo::setNameSpanForMention(alignedMention, _docSet->getDocTheory(languageVariant), \
						//	mentionAlignment->mutable_targetmention());
					}		
				}
			}
		}

		BOOST_FOREACH(LanguageVariant_ptr languageVariant, _docSet->getLanguageVariants()) {
			const DocTheory* doc = _docSet->getDocTheory(languageVariant);

			learnit::SentenceTheory* outSentTheory = outSent->add_sentencetheory();
			outSentTheory->set_language(UnicodeUtil::toUTF8StdString(languageVariant->getLanguageString()));
			MatchInfo::sentenceTheoryInfo(outSentTheory, doc, doc->getSentenceTheory(_cur_sentence), 
						_cur_sentence, ParamReader::isParamTrue("include_state"));	
		}
	}
}

void InstanceFinder::endSentenceMatches(learnit::Instance& out) {
	if (_printed_sentence) {
		//out << L"\t</sentence>\n";
		_printed_sentence = false;
		//SessionLogger::info("LEARNIT") << out.DebugString();
	} else {
		out.mutable_sentence()->RemoveLast();
	}
}

void InstanceFinder::findPatternsInSentence(const std::vector<LearnItPattern_ptr>& patterns, 
								  const std::vector<Target_ptr>& targets, const TargetToFillers& targetToFiller,
								  learnit::Sentence* out, bool all_patterns) 
{
	bool one_good = false;
	BOOST_FOREACH(LanguageVariant_ptr lang, _docSet->getLanguageVariants()) {
    //Standard for to avoid nesting BOOST_FOREACH (causes segfaults)
		for(unsigned i = 0; i < targets.size(); ++i) {
			Target_ptr target = targets[i];
			std::map<std::wstring, SlotFillersVector > fillerMap = targetToFiller.find(target)->second;
			SlotFillersVector fillers = fillerMap.find(lang->toString())->second;
			//if we have slots in the sentence match, otherwise don't both
			if (fillers.size() >= 2 && fillers[0]->size() > 0 && fillers[1]->size() > 0) {
				one_good = true;
			}
		}
	}
	if (!one_good) return;
	
	BOOST_FOREACH(LearnItPattern_ptr pattern, patterns) {
		if (all_patterns || pattern->active()) {
			
			const DocTheory* doc = _docSet->getDocTheory(pattern->language());
			if (!doc) {
				//we probably are running in monolingual mode TODO: better way to check
				doc = _docSet->getDefaultDocTheory();
			}
			const SentenceTheory* st = doc->getSentenceTheory(_cur_sentence);

			MatchInfo::PatternMatches matches = MatchInfo::findPatternInSentence(_docSet, st, pattern);
			BOOST_FOREACH(MatchInfo::PatternMatch match, matches) {
				outputPatternMatch(pattern, match, out);
				_pattern_matches += 1;
			}
		}
	}
}

void InstanceFinder::outputPatternMatch(const LearnItPattern_ptr& pattern,
				   const MatchInfo::PatternMatch& match, learnit::Sentence* out) 
{
	Target_ptr target = pattern->getTarget();
	startMatch(out);
	/*out << L"\t\t<PatternMatch target=\"" + MainUtilities::encodeForXML(target->getName()) 
		<< L"\" pattern=\"" << MainUtilities::encodeForXML(pattern->getName())
		<< L"\" start_token=\"" << match.start_token << "\" end_token=\"" 
		<< match.end_token << L"\">\n";*/
	learnit::PatternMatch* outPatternMatch = out->add_patternmatch();
	outPatternMatch->set_target(UnicodeUtil::toUTF8StdString(target->getName()));
	outPatternMatch->set_pattern(UnicodeUtil::toUTF8StdString(pattern->getPatternString()));
	outPatternMatch->set_starttoken(match.start_token);
	outPatternMatch->set_endtoken(match.end_token);

	for (int slot_num=0; slot_num<target->getNumSlots(); ++slot_num) {
		if (match.slots[slot_num] && 
			( match.slots[slot_num]->getType() == SlotConstraints::MENTION ||
			match.slots[slot_num]->getType() == SlotConstraints::VALUE ) )
		{
			learnit::SlotFiller* outSlotFiller = outPatternMatch->add_slot();
			MatchInfo::slotFillerInfo(outSlotFiller, _stheory, 
				target, match.slots[slot_num], slot_num);
		}
	}
	//out << L"\t\t</PatternMatch>\n\n";
}

void InstanceFinder::findSeedsInSentence(const std::vector<Seed_ptr>& seeds,
		const std::vector<Target_ptr>& targets, Symbol docid,
		int sent_no, const TargetToFillers& targetToFiller, learnit::Sentence* out) 
{
	std::vector<SlotFillerMap> matches;
	BOOST_FOREACH(Seed_ptr seed, seeds){
		LanguageVariant_ptr langVar = LanguageVariant::getLanguageVariant(Symbol(seed->language()),Symbol(L"source"));
		const DocTheory* doc = _docSet->getDocTheory(langVar);
		if (!doc) {
			//we probably are running in monolingual mode TODO: better way to check
			doc = _docSet->getDefaultDocTheory();
			langVar = _docSet->getDefaultLanguageVariant();
		}
		const SentenceTheory* st = doc->getSentenceTheory(sent_no);
		matches.clear();
		
		//put language here seed->language()
		//SessionLogger::info("IF") << targetToFiller.find(seed->getTarget())->second.find(langVar->toString())->second[0]->size();
		//SessionLogger::info("IF") << targetToFiller.find(seed->getTarget())->second.find(langVar->toString())->second[1]->size();
    
		seed->findInSentence(targetToFiller.find(seed->getTarget())->second.find(langVar->toString())->second,
									doc, docid, sent_no, matches, InstanceFinder::overlapIgnoreWords);
		//SessionLogger::info("IF") << matches.size();
		//SessionLogger::info("IF") << L"\n\n";
		printSlotMatches(st, seed, L"SeedMatch", matches, out);
		_seed_matches += matches.size();
	}
}

void InstanceFinder::findInstancesInSentence(const std::vector<Instance_ptr>& instances,
		const std::vector<Target_ptr>& targets, Symbol docid,
		int sent_no, const TargetToFillers& targetToFiller, learnit::Sentence* out) 
{
	std::vector<SlotFillerMap> matches;
	BOOST_FOREACH(Instance_ptr instance, instances){
		LanguageVariant_ptr langVar = _docSet->getDefaultLanguageVariant();
		const DocTheory* doc = _docSet->getDefaultDocTheory();
		const SentenceTheory* st = doc->getSentenceTheory(sent_no);
		matches.clear();

		instance->findInSentence(targetToFiller.find(instance->getTarget())->second.find(langVar->toString())->second, 
									doc, docid, sent_no, matches);
		printSlotMatches(st, instance, L"InstanceMatch", matches, out);
		_instance_matches += matches.size();
	}
}

void InstanceFinder::findScoredInstancesInSentence(const std::vector<Target_ptr>& targets, 
												   const DocTheory* doc, Symbol docid, int sent_no, 
												   learnit::Sentence* out) {
	MatchInfo::PatternMatches matches = _matcher->match(doc, docid, _stheory);
	BOOST_FOREACH(MatchInfo::PatternMatch match, matches) {
		
		/*
		<InstanceMatch target="ownsOrAcquiresTech" >
			<SlotFiller slot="0" type="MENTION" start_token="0" end_token="1" head_start_token="0" head_end_token="1" mention_type="name" >
				<Text>Al Jazeera</Text>
				<BestName confidence="1" mention_type="name">al jazeera</BestName>
				<SeedString>al jazeera</SeedString>
			</SlotFiller>
			<SlotFiller slot="1" type="MENTION" start_token="17" end_token="18" head_start_token="18" head_end_token="18" mention_type="desc" >
				<Text>banned motorcycles</Text>
				<BestName confidence="0.4" mention_type="desc">motorcycles</BestName>
				<SeedString>motorcycles</SeedString>
			</SlotFiller>
		</InstanceMatch>
		*/
		startMatch(out);

		//out << L"\t<InstanceMatch target=\"" << _matcher->target()->getName() << L"\" ";
		//out << L"p=\"" << boost::lexical_cast<wstring>(match.score) << L"\" >\n";
		learnit::SeedInstanceMatch* outInstanceMatch = out->add_seedinstancematch();
		outInstanceMatch->set_type("instance");
		outInstanceMatch->set_target(UnicodeUtil::toUTF8StdString(_matcher->target()->getName()));
		outInstanceMatch->set_score(match.score);

		//printing the slot fillers
		int slotnum = 0;
		BOOST_FOREACH(SlotFiller_ptr slot, match.slots) {			
			MatchInfo::slotFillerInfo(outInstanceMatch->add_slot(), _stheory, _matcher->target(), slot, 
					slotnum, slot->getBestName());
			slotnum++;
		}
		//out << L"\t</InstanceMatch>\n";
		_scored_instance_matches++;
	}
}

void InstanceFinder::findInstancesMatchingFeatures(const std::vector<SentenceMatchableFeature_ptr>& features,
		const SlotsIdentityFeatureCollection_ptr slotFeatures,
		const PropPatternFeatureCollection_ptr propFeatures,
		const TextPatternFeatureCollection_ptr textFeatures,
		const TextPatternFeatureCollection_ptr keywordInSentenceFeatures,
		const std::vector<Target_ptr>& targets, const AlignedDocSet_ptr doc, Symbol docid,
		int sent_no, const TargetToFillers& targetToFiller, learnit::Sentence* out) 
{
	const SentenceTheory* st = doc->getDefaultDocTheory()->getSentenceTheory(sent_no);

	// This function is a bit complicated because each triggering feature is
	// going to match a particular tuple of (value?)mentions within the sentence.
	// However, since the matches are going to be used for feature proposal,
	// we also want to return mention tuples which may be currently matched
	// by no features but refer to the same thing as the matched tuples. 
	//
	//For example, if we have "Bob went to Penn, and he played for the Penn 
	// basketball team", and we have a feature match on "X played for the Y
	// basketball team" returning the instance "he, Penn", we also want to
	// generate a match for "Bob, Penn" so that the feature proposer can 
	// propose "X went to Y".
	// 
	// So what we do is first to build up a set of SlotFillerMaps of matches
	// as normal. We then create a new set of matches (SlotFillerMaps) based 
	// on the first set by generating all tuples of mentions which have the 
	// same referent as match in our first set.  Finally, we remove all
	// duplicates from this second set.
	const Target_ptr target = targets[0];
	const SlotFillersVector& slotFillersVector = 
		targetToFiller.find(targets[0])->second.find(doc->getDefaultLanguageVariant()->toString())->second;
	
	// if we couldn't possibly match due to missing seed slots of some sort,
	// then bail out now. This is useful for slots with restricted slot types,
	// like dates or pharmaceuticalSubstances
	++_sentences_examined;
	if (_sentences_examined % 1000 == 0) {
		SessionLogger::info("IF") << "Examined " << _sentences_examined <<
			" sentences; could skip " << _bailed;
	}
	if (!target->satisfiesSufficientSlots(slotFillersVector)) {
		++_bailed;
		return;
	}

	std::vector<SlotFillerMap> triggered_matches;	

	BOOST_FOREACH(SentenceMatchableFeature_ptr feature, features) {
		feature->matchesSentence(slotFillersVector, doc, docid, sent_no, triggered_matches);
	}

	for (FeatureCollectionIterator<SlotsIdentityFeature> it = 
		slotFeatures->applicableFeatures(slotFillersVector, doc->getDefaultDocTheory(), sent_no); it; ++it) 
	{
		(*it)->matchesSentence(slotFillersVector, doc, docid, sent_no, triggered_matches);
	}

	for (FeatureCollectionIterator<BrandyPatternFeature> it = 
		propFeatures->applicableFeatures(slotFillersVector, doc->getDefaultDocTheory(), sent_no); it; ++it) 
	{
		(*it)->matchesSentence(slotFillersVector, doc, docid, sent_no, triggered_matches);
	}

	for (FeatureCollectionIterator<BrandyPatternFeature> it = 
		textFeatures->applicableFeatures(slotFillersVector, doc->getDefaultDocTheory(), sent_no); it; ++it) 
	{
		(*it)->matchesSentence(slotFillersVector, doc, docid, sent_no, triggered_matches);
	}

	for (FeatureCollectionIterator<BrandyPatternFeature> it =
		keywordInSentenceFeatures->applicableFeatures(slotFillersVector,
		doc->getDefaultDocTheory(), sent_no); it; ++it) 
	{
		(*it)->matchesSentence(slotFillersVector, doc, docid, sent_no, triggered_matches);
	}

	std::vector<SlotFillerMap> all_matches;
	BOOST_FOREACH(const SlotFillerMap& match, triggered_matches) {
		generateAllSlotFillerMaps(target, match, slotFillersVector, all_matches);
	}
	// remove duplicates
	// it's faster and simpler to do this than use a std::set
	// the default map < and == will do pointer comparisons here, but that's
	// okay because all slot fillers come from slotFillersVector.
	sort(all_matches.begin(), all_matches.end());
	std::vector<SlotFillerMap>::iterator new_last =
		unique(all_matches.begin(), all_matches.end());
	all_matches.erase(new_last, all_matches.end());

	std::vector<std::wstring> dummyForcedNames;

	printSlotMatches(st, target, L"InstanceMatch", all_matches, out, 
			dummyForcedNames);
	_feature_matches += all_matches.size();
}

void InstanceFinder::generateAllSlotFillerMaps(Target_ptr target,
	   const SlotFillerMap& match, const SlotFillersVector& slotFillersVector,
	   std::vector<SlotFillerMap>& matches)
{
	SlotFillersVector sameEntity(slotFillersVector.size());
	for (size_t i=0; i<sameEntity.size(); ++i) {
		sameEntity[i] = make_shared<SlotFillers>();
	}
	BOOST_FOREACH(const SlotNumAndFiller& slotNumAndFiller, match) {
		int slotNum = slotNumAndFiller.first;
		SlotFillers& optionsForSlot = *sameEntity[slotNum];
		BOOST_FOREACH(SlotFiller_ptr otherSlotFiller, *slotFillersVector[slotNum]) {
			if (slotNumAndFiller.second->sameReferent(*otherSlotFiller)) {
				optionsForSlot.push_back(otherSlotFiller);
			}
		}
	}

	if (target->satisfiesSufficientSlots(sameEntity)) {
		SlotFiller::findAllCombinations(target, sameEntity, matches);
	}
}

void InstanceFinder::printSlotMatches(const SentenceTheory* st,
		ObjectWithSlots_ptr slotsObj, const std::wstring& wrapper, 
		const std::vector<SlotFillerMap> matches, learnit::Sentence* out) 
{
	std::vector<std::wstring> bestNames;
	for (size_t i = 0; i<slotsObj->numSlots(); ++i) {
		bestNames.push_back(slotsObj->getSlot(i)->name());
	}
	printSlotMatches(st, slotsObj->getTarget(), wrapper, matches, out, bestNames);
	//BOOST_FOREACH(SentenceTheory* sent, _docSet->getAlignedSentenceTheories(_docSet->getDefaultLanguageVariant(),st)) {
	//	printSlotMatches(sent, slotsObj->getTarget(), wrapper, matches, out, bestNames);
	//}
}

void InstanceFinder::printSlotMatches(const SentenceTheory* st, Target_ptr target,
	const std::wstring& wrapper, const std::vector<SlotFillerMap>& matches, 
	learnit::Sentence* out, const std::vector<std::wstring>& forcedNames) 
{
	std::wstring name;
	std::vector<RelEvMatch> relationMatches;
	std::vector<RelEvMatch> eventMatches;
	
	BOOST_FOREACH(SlotFillerMap slotFillerMap, matches) {
		relationMatches.clear();
		eventMatches.clear();
		relationEventMatches(slotFillerMap, st, relationMatches, eventMatches);
		startMatch(out);

		learnit::SeedInstanceMatch* outMatch = out->add_seedinstancematch();
		if (wrapper == L"InstanceMatch") {
			outMatch->set_type("instance");
		}
		/*out << L"\t\t<" << wrapper << L" target=\"" + target->getName() << L"\" ";
		out << L">\n";*/
		outMatch->set_target(UnicodeUtil::toUTF8StdString(target->getName()));

		if (!relationMatches.empty()) {
			BOOST_FOREACH(const RelEvMatch& match, relationMatches) {
				outMatch->add_relationmatch(UnicodeUtil::toUTF8StdString(match.type));
			}
		}
		bool do_print = false;
		if (!eventMatches.empty()) {
			BOOST_FOREACH(const RelEvMatch& match, eventMatches) {
				if (!match.anchor.empty()) {
					do_print = true;
				}
				outMatch->add_eventmatch(UnicodeUtil::toUTF8StdString(match.type));
			}
		}
		if (do_print) {
			BOOST_FOREACH(const RelEvMatch& match, eventMatches) {
				outMatch->add_eventanchor(UnicodeUtil::toUTF8StdString(match.anchor));
			}
		}
		
		SentenceTheory* sTheory;

		BOOST_FOREACH(SlotNumAndFiller slotnumAndFiller, slotFillerMap) {
			int slotnum = slotnumAndFiller.first;
			SlotFiller_ptr slotFiller = slotnumAndFiller.second;
			if (forcedNames.empty()) {
				name = slotFiller->getBestName();
			} else {
				name = forcedNames[slotnum];
			}
			
			sTheory = _docSet->getDocTheory(slotFiller->getLanguageVariant())->getSentenceTheory(_cur_sentence);

			//out << L"\t\t\t" 	<< 
			MatchInfo::slotFillerInfo(outMatch->add_slot(), sTheory, target, slotFiller, 
					slotnum, name, relationMatches, eventMatches);
		}
		//out << L"\t\t\t" << 
		MatchInfo::propInfo(outMatch, sTheory, target, 
				slotFillerMap, ParamReader::isParamTrue("lexicalize_props"));
		//out	<< L"\t\t</" << wrapper << L">\n\n";
	}
}

void InstanceFinder::relationEventMatches(const SlotFillerMap& slotFillerMap,
	const SentenceTheory* st, std::vector<RelEvMatch>& relationMatches,
	std::vector<RelEvMatch>& eventMatches)
{
	// for the moment the implementation below assumes our slotFillerMap
	// represents a binary relation with slots 0 and 1, so we need to
	// confirm this to avoid a crash
	
	if (slotFillerMap.size() > 2) {
		SessionLogger::warn("rel_ev_feature_binary") << L"Relation and event "
			<< L"features only work for binary relations.";
		return;
	}

	BOOST_FOREACH(const SlotNumAndFiller& slotNumAndFiller, slotFillerMap) {
		if (slotNumAndFiller.first > 1) {
			std::wstringstream err;
			err << L"Expected only slots 0 and 1 but saw " << slotNumAndFiller.first;
			throw UnexpectedInputException("InstanceFinder::relationEventMatches", err);
		}
	}

	std::vector<std::wstring> roles(2);

	const RelMentionSet* rms = st->getRelMentionSet();
	for (int i=0; i<rms->getNRelMentions(); ++i) {
		roles[0].clear(); roles[1].clear();
		const RelMention* rm = rms->getRelMention(i);

		BOOST_FOREACH(const SlotNumAndFiller& slotNumAndFiller, slotFillerMap) {
			SlotFiller_ptr slotFiller = slotNumAndFiller.second;
			unsigned int slot_num = slotNumAndFiller.first;

			if (slotFiller->getMention()) {
				if (slotFiller->contains(rm->getLeftMention())) {
					roles[slot_num] = 
						rm->getRoleForMention(rm->getLeftMention()).to_string();
				}
				if (slotFiller->contains(rm->getRightMention())) {
					roles[slot_num] =
						rm->getRoleForMention(rm->getRightMention()).to_string();
				}
			}

			if (slotFiller->getValueMention() &&
					slotFiller->contains(rm->getTimeArgument())) 
			{
				roles[slot_num] = rm->getTimeRole().to_string();
			}
		}

		if (!(roles[0].empty() || roles[1].empty())) {
			relationMatches.push_back(RelEvMatch(rm->getType().to_string(), roles));
		}
	}

	const EventMentionSet* ems = st->getEventMentionSet();
	for (int i=0; i<ems->getNEventMentions(); ++i) {
		roles[0].clear(); roles[1].clear();
		const EventMention* em = ems->getEventMention(i);
		
		BOOST_FOREACH(const SlotNumAndFiller& slotNumAndFiller, slotFillerMap) {
			SlotFiller_ptr slotFiller = slotNumAndFiller.second;
			unsigned int slot_num = slotNumAndFiller.first;

			for (int j=0; j<em->getNArgs(); ++j) {
				if (slotFiller->getMention() &&
						slotFiller->contains(em->getNthArgMention(j))) 
				{
					roles[slot_num] = em->getNthArgRole(j).to_string();
				}
			}

			for (int j=0; j<em->getNValueArgs(); ++j) {
				if (slotFiller->getValueMention() &&
						slotFiller->contains(em->getNthArgValueMention(j))) 
				{
					roles[slot_num] = em->getNthArgValueRole(j).to_string();
				}
			}
		}

		if (!(roles[0].empty() || roles[1].empty())) {
			eventMatches.push_back(RelEvMatch(em->getEventType().to_string(), 
				roles, ALEventUtilities::anchorArg(em)));
		}
	}
}


int main(int argc, char** argv) {

	//ProfilerStart("instance_finder");

#ifdef NDEBUG
	try{
#endif
	std::string param_file;
	std::string doclist_filename;
	std::string learnit_db_name;
	std::string output_name;
	std::string test_pattern_file;
	bool do_seeds(false), do_patterns(false), do_instances(false), do_features(false), do_scored_instances(false), all_patterns(false);

	boost::program_options::options_description desc("Options");
	desc.add_options()
		("param-file,P", boost::program_options::value<std::string>(&param_file),
			"[required] parameter file")
		("doc-list,D", boost::program_options::value<std::string>(&doclist_filename),
			"[required] document list")
		("learnit-db,L", boost::program_options::value<std::string>(&learnit_db_name),
			"[required] learnit database file")
		("output,O", boost::program_options::value<std::string>(&output_name),
			"[required] output file name")
		("seeds,s", boost::program_options::value<bool>(&do_seeds)->zero_tokens(),
			"[optional] do seed matching")
		("patterns,p", boost::program_options::value<bool>(&do_patterns)->zero_tokens(),
			"[optional] do pattern matching")
		("instances,i", boost::program_options::value<bool>(&do_instances)->zero_tokens(),
			"[optional] do instance matching")
		("scored-instances,i", boost::program_options::value<bool>(&do_scored_instances)->zero_tokens(),
			"[optional] do scored instance matching")
		("all-patterns,a", boost::program_options::value<bool>(&all_patterns)->zero_tokens(),
			"[optional] do pattern matching with both active and inactive patterns")
		("features,f", boost::program_options::value<bool>(&do_features)->zero_tokens(),
			"[optional] do feature matching");
		
	boost::program_options::positional_options_description pos;
	pos.add("param-file", 1).add("doc-list", 1).add("learnit-db", 1).add("output", 1);

	boost::program_options::variables_map var_map;
	try {
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).positional(pos).run(), var_map);
	} catch (std::exception & exc) {
		std::cerr << "Command-line parsing exception: " << exc.what() << std::endl;
		exit(1);
	}

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	boost::program_options::notify(var_map);

	validate_mandatory_unique_cmd_line_arg(desc, var_map, "param-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "doc-list");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "learnit-db");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "output");

	if (!(do_seeds || do_patterns || do_instances || do_scored_instances || do_features)) {
		std::cerr << "Nothing to do! You must specify at least one of " << 
			"--seeds, --patterns, --instances, or --features!\n";
		exit(1);
	}

	ParamReader::readParamFile(param_file);
    ConsoleSessionLogger logger(std::vector<std::wstring>(), L"[IF]");
	SessionLogger::setGlobalLogger(&logger);
	// When the unsetter goes out of scope, due either to normal termination or an exception, it will unset the logger ptr for us.
	SessionLoggerUnsetter unsetter;
	
	FeatureModule::load();

	FeatureModule::load();

	LearnItDB_ptr learnit_db = make_shared<LearnItDB>(learnit_db_name);
	LearnIt2Matcher_ptr matcher;
	if (do_scored_instances) {
		SessionLogger::info("LEARNIT") << "Loading LearnIt2 Matcher..." << std::endl;
		matcher = boost::make_shared<LearnIt2Matcher>(learnit_db);
	} else {
		matcher = boost::shared_ptr<LearnIt2Matcher>();
	}

	//UTF8OutputStream out(output_name.c_str());

	if (ParamReader::isParamTrue("no_coref")) {
		SessionLogger::warn("LEARNIT") << "running without coref information" << std::endl;
	}

	std::vector<Target_ptr> targets = learnit_db->getTargets();

	if (targets.size() <= 0) {
		throw UnexpectedInputException("InstanceFinder::main()",
			"Targets table is empty");
	}

	std::vector<LearnItPattern_ptr> patterns = learnit_db->getPatterns(true);
	std::vector<Seed_ptr> seeds = learnit_db->getSeeds(true);
	std::vector<Instance_ptr> instances = learnit_db->getInstances();
	std::vector<SentenceMatchableFeature_ptr> features;
	SlotsIdentityFeatureCollection_ptr slotsFeatures;
	PropPatternFeatureCollection_ptr propFeatures;
	TextPatternFeatureCollection_ptr textFeatures;
	TextPatternFeatureCollection_ptr keywordInSentenceFeatures;

	if (do_seeds) {
		SessionLogger::info("LEARNIT") << "Looking for " << seeds.size() << " seeds." << std::endl;
	}
	if (do_patterns) {
		SessionLogger::info("LEARNIT") << "Matching " << patterns.size() << " patterns." << std::endl;
	}
	if (do_instances) {
		SessionLogger::info("LEARNIT") << "Looking for " << instances.size() << " instances." << std::endl;
	}
	if (do_scored_instances) {
		SessionLogger::info("LEARNIT") << "Looking for scored instances." << std::endl;
	}
	if (do_features) {
		double threshold = 0.0; //ParamReader::getRequiredFloatParam("instance_finder_feature_threshold");
		if (targets.size() > 1) {
			throw UnexpectedInputException("InstanceFinder::main()",
				"Feature matching currently only supports single targets");
		}
		
		MultiAlphabet_ptr alphabet = MultiAlphabet::create(
			dynamic_pointer_cast<FeatureAlphabet>(
				learnit_db->getAlphabet(LearnIt2::SENTENCE_ALPHABET_INDEX)),
			dynamic_pointer_cast<FeatureAlphabet>(
				learnit_db->getAlphabet(LearnIt2::SLOT_ALPHABET_INDEX)));

		slotsFeatures = 
			SlotsIdentityFeatureCollection::create(targets[0], alphabet,
			threshold, false);
		textFeatures =
			TextPatternFeatureCollection::createFromTextPatterns(targets[0], 
			alphabet, threshold, false);
		propFeatures = 
			PropPatternFeatureCollection::create(targets[0], alphabet,
			threshold, false);
		keywordInSentenceFeatures =
			TextPatternFeatureCollection::createFromKeywordInSentencePatterns(
				targets[0], alphabet, threshold, false);
	}

	// Read the list of documents to process.
	std::vector<AlignedDocSet_ptr> doc_files;
	if (ParamReader::isParamTrue("bilingual")) {
		doc_files = FileUtilities::readAlignedDocumentList(doclist_filename);
	} else {
		doc_files = FileUtilities::readDocumentListToAligned(doclist_filename);
	}
	
	int total_seed_matches = 0, total_pattern_matches = 0, 
		total_instance_matches = 0, total_scored_instance_matches = 0, total_feature_matches = 0;
	// Load and process each document.
	//out << L"<instances>\n";
	learnit::Instance instanceOut = learnit::Instance(learnit::Instance::default_instance());
	

	while (!doc_files.empty()) {
		// Load the SerifXML and get the DocTheory (Implicitly lazily done by the doc_set
		AlignedDocSet_ptr doc_set = doc_files.back();
		doc_files.pop_back();
		
		SessionLogger::info("LEARNIT") << "Processing " << doc_set->getDefaultDocFilename() << std::endl;
		
		InstanceFinder inst_finder(doc_set, doc_set->getDocumentName(),
			do_seeds, do_patterns, do_instances, do_scored_instances, do_features, matcher);
		inst_finder.run(targets, seeds, patterns, instances, features,
				slotsFeatures, propFeatures, textFeatures,
				keywordInSentenceFeatures, instanceOut, all_patterns);
		
		std::stringstream counts;
		counts << '\t';
		if (inst_finder.numSeedMatches() > 0) {
			counts << " " << inst_finder.numSeedMatches() << " seed matches;";
		} 
		if (inst_finder.numPatternMatches() > 0) {
			counts << " " << inst_finder.numPatternMatches() << " pattern matches;";
		}
		if (inst_finder.numInstanceMatches() > 0) {
			counts << " " << inst_finder.numInstanceMatches() << " instance matches";
		}
		if (inst_finder.numScoredInstanceMatches() > 0) {
			counts << " " << inst_finder.numScoredInstanceMatches() << " scored instance matches";
		}
		if (inst_finder.numFeatureMatches() > 0) {
			counts << " " << inst_finder.numFeatureMatches() << " features matches";
		}
		
		std::string count_string = counts.str();
		if (count_string.length() > 1) {
			SessionLogger::info("count_matches_0") << count_string << std::endl;
		}

		total_seed_matches += inst_finder.numSeedMatches();
		total_pattern_matches += inst_finder.numPatternMatches();
		total_instance_matches += inst_finder.numInstanceMatches();
		total_scored_instance_matches += inst_finder.numScoredInstanceMatches();
		total_feature_matches += inst_finder.numFeatureMatches();

		// Clean up
		doc_set->garbageCollect();
	}

	//out << L"</instances>" << L"\n";
	std::fstream output(output_name.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	if (!instanceOut.SerializeToOstream(&output)) {
		SessionLogger::err("LEARNIT") << "Failed to write output instances." << std::endl;
		return -1;
    }
	google::protobuf::ShutdownProtobufLibrary();
	
	SessionLogger::info("LEARNIT") << "  Found " << total_seed_matches << " seed matches" << std::endl;
	SessionLogger::info("LEARNIT") << "  Found " << total_pattern_matches << " pattern matches" << std::endl;
	SessionLogger::info("LEARNIT") << "  Found " << total_instance_matches << " instance matches" << std::endl;
	SessionLogger::info("LEARNIT") << "  Found " << total_scored_instance_matches << " scored instance matches" << std::endl;
	SessionLogger::info("LEARNIT") << "  Found " << total_feature_matches << " feature matches" << std::endl;
	//out.close();

#ifdef NDEBUG
	// If something broke, then say what it was.
	} catch (UnrecoverableException &e) {
		std::cerr << "\n" << e.getMessage() << std::endl;
		std::cerr << "Error Source: " << e.getSource() << std::endl;
		return -1;
	}
	catch (std::exception &e) {
		std::cerr << "Uncaught Exception: " << e.what() << std::endl;
		return -1;
	}
#endif

	//ProfilerStop();
}

