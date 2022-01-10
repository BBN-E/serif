#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/nestedNames/DefaultNestedNameRecognizer.h"
#include "Generic/theories/NestedNameTheory.h"
#include "Generic/theories/NestedNameSpan.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Token.h"

#include <boost/foreach.hpp> 
#include <boost/algorithm/string/predicate.hpp>

DefaultNestedNameRecognizer::DefaultNestedNameRecognizer() :
	_debug_flag(false),
	_num_names(0), _sent_no(0), _tokenSequence(0), _docTheory(0)
{
	_skip_all_namefinding = ParamReader::isParamTrue("skip_all_namefinding");
	_do_nested_names = ParamReader::isParamTrue("do_nested_names");
	if (!_skip_all_namefinding && _do_nested_names) 
	{
		_nameList = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("nested_names_gpe_list"), 
			true, false); 
	}
	
}

DefaultNestedNameRecognizer::~DefaultNestedNameRecognizer()
{
}

int DefaultNestedNameRecognizer::getNestedNameTheories(NestedNameTheory **results, int max_theories, 
												TokenSequence *tokenSequence, NameTheory *nameTheory) 
{
	NestedNameTheory *nestedNameTheory = _new NestedNameTheory(tokenSequence, nameTheory, 0);
	_tokenSequence = tokenSequence;
	_sent_no = tokenSequence->getSentenceNumber();
	if (_skip_all_namefinding || !_do_nested_names) {
		results[0] = nestedNameTheory;
		return 1;
	}
	
	// don't do nested name recognition on POSTDATE region (turned into a timex elsewhere)
	if (_docTheory != 0 && _docTheory->isPostdateSentence(_sent_no)) {
		results[0] = nestedNameTheory;
		return 1;
	}

	int n_name_spans = nameTheory->getNNameSpans();
	std::vector<NestedNameSpan*> nestedNameSpans;
	for (int i = 0; i < n_name_spans; i++) 
	{
		// skip any name spans that aren't ORGs
		if (nameTheory->getNameSpan(i)->type != EntityType::getORGType()) continue;

		int name_start_token = nameTheory->getNameSpan(i)->start;
		int name_end_token = nameTheory->getNameSpan(i)->end;

		// skip any single token name spans
		if (name_start_token == name_end_token) continue;

		std::wstring full_name_string = nameTheory->getNameString(i);

		// skip names that look like street names
//		if (isStreetName(full_name_string)) continue;

		// iterate over each name in the name list, pick only a single name that covers the
		// largest span of tokens in the current name (this is to prevent issues with large 
		// geonames list where "New York" and "York" both match in a name like "New York Yankees")
		int best_start_token_idx = -1;
		int best_end_token_idx = -1;
		for (std::set<std::wstring>::iterator name = _nameList.begin(); name != _nameList.end(); ++name) 
		{
			// skip any list name that is longer or equal to the full name span
			if (name->length() >= full_name_string.length()) continue;
			size_t index = full_name_string.find(*name);
			if (index != std::wstring::npos) {
				// find start and end token for nested name
				const Token * st = _tokenSequence->getToken(name_start_token);
				int nested_name_start_index = st->getStartCharOffset().value() + static_cast<int>(index);
				int nested_name_end_index = st->getStartCharOffset().value() + static_cast<int>(index + name->length()) - 1;
				int nested_name_start_token = findMatchingStartToken(nested_name_start_index, 
																	 name_start_token,
																	 name_end_token);
			 	int nested_name_end_token = findMatchingEndToken(nested_name_end_index, 
																	 name_start_token,
																	 name_end_token);
				if (nested_name_start_token == -1 || nested_name_end_token == -1) {
					// found nested name doesn't align properly to tokens, ignoring it
					SessionLogger::info("nested_names") << "nested name alignment issue:" << std::endl
						<< "   full name: " << full_name_string << std::endl
						<< "   nested name match: " << *name << std:: endl;
					continue;
				}

				// only change best nested name span if nested name spans more tokens
				if (best_start_token_idx == -1 && best_end_token_idx == -1) 
				{
					best_start_token_idx = nested_name_start_token;
					best_end_token_idx = nested_name_end_token;
				} else if (nested_name_end_token - nested_name_start_token > 
					best_end_token_idx - best_start_token_idx) 
				{
					best_start_token_idx = nested_name_start_token;
					best_end_token_idx = nested_name_end_token;
				}
			}
		}
		// if no matching best nested name found, continue
		if (best_start_token_idx == -1 || best_end_token_idx == -1) continue;

		// create a nested name span, add it to nested name theory
		NestedNameSpan *nestedNameSpan = _new NestedNameSpan(best_start_token_idx, 
									   best_end_token_idx, 
									   EntityType::getGPEType(), 
		// above line: currently only loading GPE's into the name list, may change in the future
									   nameTheory->getNameSpan(i));
		nestedNameSpans.push_back(nestedNameSpan);

	}
				
	results[0] = _new NestedNameTheory(tokenSequence, nameTheory, nestedNameSpans);
	return 1;
}

int DefaultNestedNameRecognizer::findMatchingStartToken(int index, int start_token, int end_token) {
	for (int i = start_token; i <= end_token; i++) 
	{
		const Token * token = _tokenSequence->getToken(i);
		if (_debug_flag) 
		{
			const char * token_text = token->getSymbol().to_debug_string();
			int token_start_index = token->getStartCharOffset().value();
		}
		if (token->getStartCharOffset().value() == index) return i;
	}
	return -1;
}

int DefaultNestedNameRecognizer::findMatchingEndToken(int index, int start_token, int end_token) {
	for (int i = end_token; i >= start_token; i--) 
	{
		const Token * token = _tokenSequence->getToken(i);
		if (_debug_flag) 
		{
			const char * token_text = token->getSymbol().to_debug_string();
			int token_end_index = token->getEndCharOffset().value();
		}
		if (token->getEndCharOffset().value() == index) return i;
	}
	return -1;
}
