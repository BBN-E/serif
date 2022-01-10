// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/MentionGroups/extractors/EnglishPersonNameVariationsExtractor.h"
#include "Generic/edt/MentionGroups/MentionGroupUtils.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/AttributeValuePair.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

const Symbol EnglishPersonNameVariationsExtractor::FULLEST = Symbol(L"fullest");
const Symbol EnglishPersonNameVariationsExtractor::FIRST_MI_LAST = Symbol(L"first-mi-last");
const Symbol EnglishPersonNameVariationsExtractor::FIRST_LAST = Symbol(L"first-last");
const Symbol EnglishPersonNameVariationsExtractor::FIRST = Symbol(L"first");
const Symbol EnglishPersonNameVariationsExtractor::LAST = Symbol(L"last");
const Symbol EnglishPersonNameVariationsExtractor::MI = Symbol(L"middle-initial");
const Symbol EnglishPersonNameVariationsExtractor::MIDDLE_TOKEN = Symbol(L"middle-token");
const Symbol EnglishPersonNameVariationsExtractor::MIDDLE = Symbol(L"middle");
const Symbol EnglishPersonNameVariationsExtractor::SUFFIX = Symbol(L"suffix");

namespace {
	Symbol COMMA(L",");
	Symbol DASH(L"-");

	// Check for names of the sort "Smith, John"
	bool isReversedCommaName(std::vector<Symbol>& words) {
		return words.size() > 2 && words[1] == COMMA;
	}

	std::wstring getTokenString(std::vector<Symbol>& words, int start, int end) {
		std::wstring result = L"";
		for (int i = start; i <= end; i++) {
			if (result.size() != 0)
				result += L' ';
			result += words[i].to_string();
		}
		return result;
	}
}

EnglishPersonNameVariationsExtractor::EnglishPersonNameVariationsExtractor() 
	: AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"person-name-variations")),
	_suffixes(_new SymbolHash(ParamReader::getRequiredParam("linker_suffixes").c_str()))
{
	validateRequiredParameters();
}

void EnglishPersonNameVariationsExtractor::validateRequiredParameters() {
	ParamReader::getRequiredParam("linker_suffixes");
}


/* This method is a direct adaptation of EnglishRuleNameLinker::generatePERVariations() */
std::vector<AttributeValuePair_ptr> EnglishPersonNameVariationsExtractor::extractFeatures(const Mention& context,
																					LinkInfoCache& cache,
																					const DocTheory *docTheory) 
{
	std::vector<AttributeValuePair_ptr> results;

	if (context.getEntityType() == EntityType::getPERType() && 
		(context.getMentionType() == Mention::NAME || context.getMentionType()  == Mention::NEST)) 
	{
		std::vector<Symbol> words = context.getHead()->getTerminalSymbols();
		int word_count = static_cast<int>(words.size());

		// if ITEA_LINKING block goes here

		std::wstring buffer;
		std::wstring middleNameBuffer;
		std::wstring lastNameBuffer;
		bool has_suffix = _suffixes->lookup(words.back());

		// Determine which word(s) in the word vector corresponds to which name
		int first_name = -1;
		int middle_name_start = -1;
		int middle_name_end = -1;
		int last_name_start = -1;
		int last_name_end = -1;
		int suffix = -1;
		int last_name_dash = -1;
		
		if (word_count > 2 && words[word_count - 2] == DASH)
			last_name_dash = word_count - 2;

		if (isReversedCommaName(words)) { // 'smith, john william'
			last_name_start = 0;
			last_name_end = 0;
			first_name = 2;
			if (word_count >= 4) middle_name_start = 3;
			if (word_count >= 4) middle_name_end = word_count - 1;
		} else if (word_count == 1) { // 'john' or 'smith'
			// we don't know if this is a first or last name, so do nothing 
		} else if (word_count == 2 && !has_suffix) { // john smith
			first_name = 0;
			last_name_start = 1;
			last_name_end = 1;
		} else if (word_count == 2 && has_suffix) { // 'john jr.'
			first_name = 0;
			suffix = 1;
		} else if (word_count == 3 && last_name_dash != -1) { // smith - jones
			last_name_start = 0;
			last_name_end = 2;
		} else if (word_count == 4 && !has_suffix && last_name_dash != -1) { // john smith - jones
			first_name = 0;
			last_name_start = 1;
			last_name_end = 3;
		} else if (word_count == 5 && has_suffix && last_name_dash != -1) { // john smith - jones jr.
			first_name = 0;
			last_name_start = 1;
			last_name_end = 3;
			suffix = 4;
		} else if (word_count > 4 && !has_suffix && last_name_dash != -1) { // john william edward smith - jones
			first_name = 0;
			last_name_start = word_count - 3;
			last_name_end = word_count - 1;
			middle_name_start = 1;
			middle_name_end = word_count - 4;
		} else if (word_count > 4 && has_suffix && last_name_dash != -1) { // john william edward smith - jones jr. 
			first_name = 0;
			last_name_start = word_count - 4;
			last_name_end = word_count - 2;
			if (word_count > 5) middle_name_start = 1;
			if (word_count > 5) middle_name_end = word_count - 5;
	    } else if (word_count > 2 && !has_suffix) { // 'john william edward smith'
			first_name = 0;
			last_name_start = word_count - 1;
			last_name_end = word_count - 1;
			middle_name_start = 1;
			middle_name_end = word_count - 2;
		} else if (word_count > 2 && has_suffix) { // 'john william edward smith jr.'
			first_name = 0;
			last_name_start = word_count - 2;
			last_name_end = word_count - 2;
			suffix = word_count - 1;
			if (word_count > 3) middle_name_start = 1;
			if (word_count > 3) middle_name_end = word_count - 3;
		}

		if (last_name_start != -1 && last_name_end != -1) {
			lastNameBuffer = getTokenString(words, last_name_start, last_name_end);
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), LAST, lastNameBuffer.c_str(), cache);
		}
		if (first_name != -1) {
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), FIRST, words[first_name].to_string(), cache);
		}
		if (first_name != -1 && last_name_start != -1 && last_name_end != -1) {
			buffer = words[first_name].to_string();
			buffer += L' ';
			buffer += getTokenString(words, last_name_start, last_name_end);
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), FIRST_LAST, buffer, cache);
		}
		if (middle_name_start != -1) {
			std::wstring middleName = words[middle_name_start].to_string();
			std::wstring middleInitial = middleName.substr(0, 1);
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), MI, middleInitial, cache);
		}
		if (first_name != -1 && middle_name_start != -1 && last_name_start != -1 && last_name_end != -1) {
			std::wstring middleName = words[middle_name_start].to_string();
			std::wstring middleInitial = middleName.substr(0, 1);
			buffer = words[first_name].to_string();
			buffer += L' ';
			buffer += middleInitial;
			buffer += L' ';
			buffer += getTokenString(words, last_name_start, last_name_end);
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), FIRST_MI_LAST, buffer, cache);
		}
		if (middle_name_start != -1 && middle_name_end != -1) {
			middleNameBuffer = getTokenString(words, middle_name_start, middle_name_end);
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), MIDDLE, middleNameBuffer, cache);
		}
		if (middle_name_start != middle_name_end && middle_name_start != -1 && middle_name_end != -1) {
			for (int i = middle_name_start; i <= middle_name_end; i++) {
				MentionGroupUtils::addSymbolFeatures(results, getFullName(), MIDDLE_TOKEN, words[i].to_string(), cache);
			}
		}
		if (suffix != -1) {
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), SUFFIX, words[suffix].to_string(), cache);
		}

		// Generate fullest possible name
		buffer = std::wstring();
		if (first_name != -1) 
			buffer += words[first_name].to_string();
		if (middle_name_start != -1 && middle_name_end != -1) {
			if (buffer.length() != 0) buffer += L' ';
			buffer += middleNameBuffer;
		}
		if (last_name_start != -1 && last_name_end != -1) {
			if (buffer.length() != 0) buffer += L' ';
			buffer += lastNameBuffer;
		}
		if (suffix != -1) {
			if (buffer.length() != 0) buffer += L' ';
			buffer += words[suffix].to_string();
		}	
		if (buffer.length() != 0) {
			MentionGroupUtils::addSymbolFeatures(results, getFullName(), FULLEST, buffer, cache);
		}

	}

	return results;
}

