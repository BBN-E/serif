// Copyright 2011 BBN Technologies
// All rights reserved.

#include "Generic/common/leak_detection.h"
#include <boost/foreach.hpp> 
#include "Generic/common/ParamReader.h"
#include "Generic/values/DefaultValueRecognizer.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/values/PatternValueFinder.h"

/*
	A PIdF model may be trained for the Unspecified Language and used here.
	Otherwise, a rule-based model will be used to collect some TIMEX values.
*/	

DefaultValueRecognizer::DefaultValueRecognizer()
	: DO_VALUES(false), _pidfDecoder(0), _docTheory(0),
	_value_finder(RULE_VALUE_FINDER), _patternValueFinder(0)
{
	DO_VALUES = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_values_stage", false);
	if (!DO_VALUES) return;

	std::string type = ParamReader::getRequiredParam("value_finder_type");
	if (type.compare("pidf") == 0) {
		_value_finder = PIDF_VALUE_FINDER;
	} else if (type.compare("rule") == 0) { 
		_value_finder = RULE_VALUE_FINDER;
	} else if (type.compare("both") == 0) {
		_value_finder = BOTH;
	} else {
		throw UnexpectedInputException("DefaultValueRecognizer::DefaultValueRecognizer()",
										"value_finder_type must be set to 'pidf', 'rule', or 'both'");
	}
	
	if (_value_finder == PIDF_VALUE_FINDER || _value_finder == BOTH) {
		// Read parameters
		std::string tag_set_file = ParamReader::getRequiredParam("values_tag_set_file");
		std::string features_file = ParamReader::getRequiredParam("values_features_file");
		std::string model_file = ParamReader::getRequiredParam("values_model_file");
		
		_pidfDecoder = _new PIdFModel(PIdFModel::DECODE, tag_set_file.c_str(),
									  features_file.c_str(), model_file.c_str(), 0);
		
	}
	
	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_value_finding_patterns", false))
		_patternValueFinder = _new PatternValueFinder();
}

DefaultValueRecognizer::~DefaultValueRecognizer()
{
	if (_value_finder == PIDF_VALUE_FINDER)
		delete _pidfDecoder;
	delete _patternValueFinder; 
	_patternValueFinder = 0;
}

void DefaultValueRecognizer::resetForNewSentence()
{
}

void DefaultValueRecognizer::resetForNewDocument(DocTheory* docTheory)
{	
	_docTheory = docTheory;

	if (_patternValueFinder)
		_patternValueFinder->resetForNewDocument(docTheory);

	if (!DO_VALUES || _value_finder != PIDF_VALUE_FINDER) return;

	_pidfDecoder->resetForNewDocument(docTheory);

}

int DefaultValueRecognizer::getValueTheories(ValueMentionSet **results, int max_theories,
											 TokenSequence *tokenSequence)
{
	if (!DO_VALUES) {
		results[0] = _new ValueMentionSet(tokenSequence, 0);
		return 1;
	}

	// make the contents of POSTDATE regions automatically into dates
	if (_docTheory != NULL) {
		int sent_no = tokenSequence->getSentenceNumber();
		EDTOffset sent_offset = _docTheory->getSentence(sent_no)->getStartEDTOffset();
		if (_docTheory->getMetadata() != 0 &&
			_docTheory->getMetadata()->getCoveringSpan(sent_offset, POSTDATE_SYM) != 0 &&
			tokenSequence->getNTokens() > 0) {
			try {
				ValueMentionUID uid = ValueMention::makeUID(sent_no, 0);
				// T0 DO: replace TIMEX2 with something that isn't hard coded
				ValueMention *vm = _new ValueMention(sent_no, uid, 0, 
					tokenSequence->getNTokens() - 1, Symbol(L"TIMEX2"));
				results[0] = _new ValueMentionSet(tokenSequence, 1);
				results[0]->takeValueMention(0, vm);
				return 1;
			} catch (InternalInconsistencyException &e) {
				// Should never happen unless max # of value mentions is strangely set to 0
				SessionLogger::err("value_mention_uid") << e;
			}
		}
	}
	
	SpanList valueSpans;

	if (_value_finder == PIDF_VALUE_FINDER){
		valueSpans = getPIdFValues(tokenSequence);
	}
	else if (_value_finder == RULE_VALUE_FINDER) {
		valueSpans = identifyRuleRepositoryValues(tokenSequence);
	}
	else {// _value_finder == BOTH
		SpanList resultsPidf = getPIdFValues(tokenSequence);
		SpanList resultsRule = identifyRuleRepositoryValues(tokenSequence);
		valueSpans = combineSpanLists(resultsPidf, resultsRule);
	}

	if (_patternValueFinder) {
		SpanList patternValueSpans = _patternValueFinder->findValueMentions(tokenSequence->getSentenceNumber());
		valueSpans.insert(valueSpans.end(), patternValueSpans.begin(), patternValueSpans.end());
	}

	results[0] = createValueMentionSet(tokenSequence, valueSpans);
	return 1;

}

std::vector<ValueRecognizer::ValueSpan> DefaultValueRecognizer::getPIdFValues(TokenSequence *tokenSequence) {
	
	PIdFSentence sentence(_pidfDecoder->getTagSet(), *tokenSequence);	
	_pidfDecoder->decode(sentence);
	return collectValueSpans(sentence, _pidfDecoder->getTagSet());

}

ValueRecognizer::SpanList DefaultValueRecognizer::combineSpanLists(SpanList resultsPidf, SpanList resultsRule)
{
	SpanList resultsCombined;

	// assume the two spanlists are already sorted
	// don't take two with the same start & end offset

	size_t pidf_index = 0;
	size_t rule_index = 0;

	if (resultsPidf.size() > 0 &&
		resultsRule.size() > 0){
		int pidfOffset;
		int ruleOffset;
		while (pidf_index < resultsPidf.size() &&
			   rule_index < resultsRule.size()) {
		
			pidfOffset = resultsPidf[pidf_index].start;
			ruleOffset = resultsRule[rule_index].start;
		
			if (pidfOffset < ruleOffset){
				resultsCombined.push_back(resultsPidf[pidf_index++]);
			} else if (pidfOffset > ruleOffset) {
				resultsCombined.push_back(resultsRule[rule_index++]);
			} else {
				if (resultsPidf[pidf_index].end !=
					resultsRule[rule_index].end) {
						resultsCombined.push_back(resultsPidf[pidf_index++]);
						resultsCombined.push_back(resultsRule[rule_index++]);
				} else {
					resultsCombined.push_back(resultsPidf[pidf_index++]);
					rule_index++;
				}
			}
		}
	}

	while (pidf_index < resultsPidf.size())
		resultsCombined.push_back(resultsPidf[pidf_index++]);
	while (rule_index < resultsRule.size())
		resultsCombined.push_back(resultsRule[rule_index++]);
	
	return resultsCombined;
}
