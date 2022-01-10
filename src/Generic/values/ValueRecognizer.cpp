// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <boost/foreach.hpp> 

#include "Generic/common/ParamReader.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Generic/theories/ValueType.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/values/ValueRecognizer.h"
#include "Generic/values/xx_ValueRecognizer.h"

Symbol ValueRecognizer::NONE_ST = Symbol(L"NONE-ST");
Symbol ValueRecognizer::POSTDATE_SYM = Symbol(L"POSTDATE");



boost::shared_ptr<ValueRecognizer::Factory> &ValueRecognizer::_factory() {
	static boost::shared_ptr<ValueRecognizer::Factory> factory(new XXValueRecognizerFactory());
	return factory;
}

ValueMentionSet *ValueRecognizer::createValueMentionSet(TokenSequence *tokenSequence,
                                                               SpanList valueSpans) 
{
	std::sort(valueSpans.begin(), valueSpans.end());
	SpanList finalSpans = filterOverlappingAndEmptySpans(valueSpans);

	if (static_cast<int>(finalSpans.size()) > MAX_SENTENCE_VALUES) {
		finalSpans.resize(static_cast<size_t>(MAX_SENTENCE_VALUES));
		std::stringstream errMsg;
		errMsg << "Sentence has too many 'value' objects (dates, numbers, etc.). "
			   << "The limit is " << MAX_SENTENCE_VALUES << "; all further values will be discarded.";
		SessionLogger::err("createValueMentionSet") << errMsg.str().c_str();
	}

	int sent_no = tokenSequence->getSentenceNumber();
	ValueMentionSet *valueSet = _new ValueMentionSet(tokenSequence, static_cast<int>(finalSpans.size()));
	for (int val = 0; val < static_cast<int>(finalSpans.size()); val++) {
		ValueSpan v = finalSpans.at(val);
		ValueMentionUID uid = ValueMention::makeUID(sent_no, val);
		ValueMention *valMention = _new ValueMention(sent_no, uid, v.start, v.end, v.tag);
		valueSet->takeValueMention(val, valMention);
	}	
	return valueSet;
}

ValueRecognizer::SpanList ValueRecognizer::collectValueSpans(const PIdFSentence &sentence,
                                                                           const DTTagSet *tagSet) 
{
	int NONE_ST_tag = tagSet->getTagIndex(NONE_ST);
	SpanList results;

	for (int j = 0; j < sentence.getLength(); j++) {
		if (sentence.getTag(j) != NONE_ST_tag &&
			tagSet->isSTTag(sentence.getTag(j)))
		{
			ValueSpan span;
			span.tag = tagSet->getReducedTagSymbol(sentence.getTag(j));
			span.start = j;
			span.end = span.start;
			while (span.end+1 < sentence.getLength() &&
				tagSet->isCOTag(sentence.getTag(span.end+1)))
				span.end++;
			
			results.push_back(span);
			j = span.end;
		} 
	}

	return results;
}

ValueRecognizer::SpanList ValueRecognizer::filterOverlappingAndEmptySpans(const ValueRecognizer::SpanList &spans) {
	SpanList results;
	int last_value_end = -1;
	BOOST_FOREACH (ValueSpan span, spans) {
		if (span.start <= last_value_end ||
			span.start > span.end)
		{
			continue;
		}
		last_value_end = span.end;
		results.push_back(span);
	}
	return results;
}

ValueRecognizer::SpanList ValueRecognizer::identifyRuleRepositoryPhoneNumbers(TokenSequence *tokenSequence) {
	SpanList results;

	std::list<ValueRuleRepository::index_pair_t> phoneNumbers;
	ValueType phoneType = ValueType::getUndetType();
	try {
		phoneType = ValueType(Symbol(L"PHONE"));
		_valueRuleRepository.getPhoneNumbers(tokenSequence, phoneNumbers);
	} catch (UnexpectedInputException &e) {
		// do nothing but use variable to avoid error
		e.getMessage();
	}

	for (int j = 0; j < tokenSequence->getNTokens(); j++) {
		for (std::list<ValueRuleRepository::index_pair_t>::const_iterator iter = phoneNumbers.begin(); iter != phoneNumbers.end(); ++iter) {
			ValueRuleRepository::index_pair_t my_pair = (*iter);
			if (my_pair.first == j) {
				ValueSpan span;
				span.start = my_pair.first;
				span.end = my_pair.second;
				span.tag = phoneType.getNameSymbol();
				results.push_back(span);
				break;
			}
		}
	}

	return results;
}

ValueRecognizer::SpanList ValueRecognizer::identifyRuleRepositoryValues(TokenSequence *tokenSequence) {
	SpanList results;

	std::list<ValueRuleRepository::index_pair_t> phoneNumbers;
	ValueType phoneType = ValueType::getUndetType();
	try {
		phoneType = ValueType(Symbol(L"PHONE"));
		_valueRuleRepository.getPhoneNumbers(tokenSequence, phoneNumbers);
	} catch (UnexpectedInputException &e) {
		// do nothing but use variable to avoid error
		e.getMessage();
	}

	std::list<ValueRuleRepository::index_pair_t> emailAddresses;
	ValueType emailType = ValueType::getUndetType();
	try {
		emailType = ValueType(Symbol(L"EMAIL"));
		_valueRuleRepository.getEmails(tokenSequence, emailAddresses);
	} catch (UnexpectedInputException &e) {
		// do nothing but use variable to avoid error
		e.getMessage();
	}

	std::list<ValueRuleRepository::index_pair_t> urls;
	ValueType urlType = ValueType::getUndetType();
	try {
		urlType = ValueType(Symbol(L"URL"));
		_valueRuleRepository.getURLs(tokenSequence, urls);
	} catch (UnexpectedInputException &e) {
		// do nothing but use variable to avoid error
		e.getMessage();
	}

	for (int j = 0; j < tokenSequence->getNTokens(); j++) {
		bool value_found = false;

		if (!value_found) {
			for (std::list<ValueRuleRepository::index_pair_t>::const_iterator iter = phoneNumbers.begin(); iter != phoneNumbers.end(); ++iter) {
				ValueRuleRepository::index_pair_t my_pair = (*iter);
				if (my_pair.first == j) {
					ValueSpan span(my_pair.first, my_pair.second, phoneType.getNameSymbol());
					results.push_back(span);
					value_found = true;
					break;
				}
			}
		}

		if (!value_found) {
			for (std::list<ValueRuleRepository::index_pair_t>::const_iterator iter = emailAddresses.begin(); iter != emailAddresses.end(); ++iter) {
				ValueRuleRepository::index_pair_t my_pair = (*iter);
				if (my_pair.first == j) {
					ValueSpan span(my_pair.first, my_pair.second, emailType.getNameSymbol());
					results.push_back(span);
					value_found = true;
					break;
				}
			}
		}

		if (!value_found) {
			for (std::list<ValueRuleRepository::index_pair_t>::const_iterator iter = urls.begin(); iter != urls.end(); ++iter) {
				ValueRuleRepository::index_pair_t my_pair = (*iter);
				if (my_pair.first == j) {
					ValueSpan span(my_pair.first, my_pair.second, urlType.getNameSymbol());
					results.push_back(span);
					value_found = true;
					break;
				}
			}
		}
	}

	return results;
}

