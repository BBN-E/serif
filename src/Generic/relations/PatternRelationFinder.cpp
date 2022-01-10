// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/PatternRelationFinder.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Sexp.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/RelMention.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

PatternRelationFinder::PatternRelationFinder() : _docTheory(0), _sentence_number(-1) {

	// This code directly echoes PatternEventFinder
	std::string pattern_set_list = ParamReader::getRequiredParam("relation_pattern_set_list");
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(pattern_set_list));
	UTF8InputStream& stream(*stream_scoped_ptr);
	std::wstring line;
	while (stream) {
		stream.getLine(line);
		if (!stream) 
			break;
		if (line.size()>0 && line[0]!=L'#') {
			std::string s = UnicodeUtil::toUTF8StdString(line);
			s = ParamReader::expand(s);
			PatternSet_ptr ps = boost::make_shared<PatternSet>(s.c_str());
			_patternSets.push_back(ps);
		}
	}
	stream.close();
}

PatternRelationFinder::~PatternRelationFinder() {}

void PatternRelationFinder::resetForNewSentence(DocTheory *docTheory, int sentence_num) {

	_docTheory = docTheory;
	_sentence_number = sentence_num;
}

RelMentionSet *PatternRelationFinder::getRelationTheory() {

	// This code directly echoes PatternEventFinder

	// This should have gotten set...
	if (_docTheory == 0)
		return _new RelMentionSet();

	RelMentionSet *rmSet = _new RelMentionSet();
	SentenceTheory *sTheory = _docTheory->getSentenceTheory(_sentence_number);

	// Find all results in this sentence
	// Sadly, it is inefficient to do this for each sentence, but for now that's how it'll have to be
	std::vector<PatternFeatureSet_ptr> all_results;
	BOOST_FOREACH(PatternSet_ptr ps, _patternSets) {
		PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(_docTheory, ps);
		std::vector<PatternFeatureSet_ptr> results = pm->getSentenceSnippets(sTheory, 0, true);
		all_results.insert(all_results.end(), results.begin(), results.end());
	}
	SessionLogger::dbg("pattern_relation_finder") << "Sent " << _sentence_number << ": " << all_results.size() << " pattern feature sets\n";

	// For each result, turn it into a RelationMention
	// We expect a (relation_type MY_RELATION_TYPE) label somewhere
	// We expect a (role left) and (role right) label on two mentions
	// We expect optionally a (role MY_TIME_ROLE) label on value mentions

	// NOTE: We could consider using the name of the pattern set as the relation type
	//       However, I think it's easier this way. It would also allow us to have two
	//       related relations in the same pattenr set.

	BOOST_FOREACH(PatternFeatureSet_ptr pfs, all_results) {
		
		std::vector<const Mention *> leftMentions;
		std::vector<const Mention *> rightMentions;
		std::vector<const ValueMention *> timeValueMentions;
		std::vector<std::wstring> time_roles;
		std::wstring relation_type = L"";

		for (size_t f = 0; f < pfs->getNFeatures(); f++) {
			PatternFeature_ptr feat = pfs->getFeature(f);
			if (ReturnPatternFeature_ptr rf=boost::dynamic_pointer_cast<ReturnPatternFeature>(feat)) {
				std::map<std::wstring, std::wstring> ret_values_map = rf->getPatternReturn()->getCopyOfReturnValuesMap();
				typedef std::pair<std::wstring, std::wstring> str_pair;
				BOOST_FOREACH(str_pair key_val, ret_values_map) {
					if (key_val.first == L"relation_type")
						relation_type = key_val.second;
					else if (key_val.first == L"role") {
						if (MentionReturnPFeature_ptr mrf=boost::dynamic_pointer_cast<MentionReturnPFeature>(rf)) {
							if (key_val.second == L"left")
								leftMentions.push_back(mrf->getMention());
							else if (key_val.second == L"right")
								rightMentions.push_back(mrf->getMention());
							else SessionLogger::warn("pattern_relation_finder") << "Relation mention argument label (" << key_val.first << " is neither 'left' nor 'right'.";
						} else if (ValueMentionReturnPFeature_ptr vmrf=boost::dynamic_pointer_cast<ValueMentionReturnPFeature>(rf)) {
							timeValueMentions.push_back(vmrf->getValueMention());
							time_roles.push_back(key_val.second);
						} else {
							SessionLogger::warn("pattern_relation_finder") << "Tried to use non-Mention as relation argument.";
						}
					}
				}
			}  
		}

		if (leftMentions.size() > 0 && rightMentions.size() > 0 && relation_type != L"") {
			for (size_t l = 0; l < leftMentions.size(); l++) {
				for (size_t r = 0; r < rightMentions.size(); r++) {
					if (timeValueMentions.size() == 0) {
						RelMention *rm = _new RelMention(leftMentions.at(l), rightMentions.at(r), Symbol(relation_type), _sentence_number, rmSet->getNRelMentions(), 0);
						rmSet->takeRelMention(rm);
					} else {
						for (size_t t = 0; t < timeValueMentions.size(); t++) {
							RelMention *rm = _new RelMention(leftMentions.at(l), rightMentions.at(r), Symbol(relation_type), _sentence_number, rmSet->getNRelMentions(), 0);
							rm->setTimeArgument(Symbol(time_roles.at(t)), timeValueMentions.at(t), 0);
							rmSet->takeRelMention(rm);
						}
					}
				}
			}
		} else SessionLogger::warn("pattern_relation_finder") << "Incomplete relation found by pattern relation finder.";
	}
	SessionLogger::dbg("pattern_relation_finder") << "Sent " << _sentence_number << ": " << rmSet->getNRelMentions() << " relation mentions\n";
	
	return rmSet;
}
