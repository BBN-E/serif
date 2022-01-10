// Copyright 2016 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/reader/ProseDocumentZoner.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/DocTheory.h"

#include "boost/foreach.hpp"
#include "boost/algorithm/string.hpp"

#include <vector>

const Symbol ProseDocumentZoner::PROSE_TAG = Symbol(L"PROSE");
const Symbol ProseDocumentZoner::NON_PROSE_TAG = Symbol(L"NON_PROSE");

ProseDocumentZoner::ProseDocumentZoner()
	: WINDOW_LENGTH(20), THRESHOLD(0.5), MIN_NON_PROSE_SECTION_LENGTH(5)
{ }

bool ProseDocumentZoner::isJunkyToken(std::wstring word) {
	int alpha_count = 0;
	int digit_count = 0;
	int weird_character_count = 0;
	bool found_non_comma = false;

	for (size_t i = 0; i < word.length(); i++) {
		wchar_t c = word.at(i);
		if (iswalpha(c)) {
			alpha_count++;
			continue;
		}
		if (iswdigit(c)) {
			digit_count++;
			continue;
		}
		if (c != L',')
			found_non_comma = true;

		// OK end of word characters
		if (i == word.length() - 1 && 
			(c == L'.' || c == L',' || c == L':' || c == L'"' || c == L';' || c == L'\'' || c == L'?' || c == L'%' || c == L')'))
		{
			continue;
		}
		// OK start of word characters
		if (i == 0 &&
			(c == L'"' || c == L'\'' || c == L'$' || c == L'`' || c == L'('))
		{
			continue;
		}
		// Give periods a pass unless they are at start
		if (c == L'.' && i > 0)
			continue;

		// Dashes OK
		if (c == L'-')
			continue;

		// Common contractions
		if (i == word.length() - 2 && c == L'\'' && (word.at(i+1) == L't' || word.at(i+1) == L's'))
			continue;

		weird_character_count++;
	}
	
	if (word.length() == 1) {
		wchar_t c = word.at(0);
		if (c != L'A' && c != L'a' && c != L'I' && c != L'i' && c != L'-')
			return true;
	}
	
	// only digits and commas OK
	if (digit_count > 0 && alpha_count == 0 && !found_non_comma)
		return false;

	if (alpha_count > 0 && digit_count > 0)
		return true;

	if (weird_character_count > 0)
		return true;

	return false;
}

bool ProseDocumentZoner::isHeaderToken(std::wstring word, const LocatedString *contents, int start_offset, int end_offset) {
	// Check for word preceeded and followed by tab or newline
	bool preceeded = false;
	int i;
	for (i = start_offset - 1; i >= 0; i--) {
		wchar_t c = contents->charAt(i);
		if (c == L'\t' || c == L'\n') {
			preceeded = true;
			break;
		}
		if (iswalnum(c))
			break;
	}
	if (i == -1) 
		preceeded = true;

	bool followed = false;
	for (i = end_offset; i < contents->length(); i++) {
		wchar_t c = contents->charAt(i);
		if (c == L'\t' || c == L'\n') {
			followed = true;
			break;
		}
		if (iswalnum(c))
			break;
	}
	if (i == contents->length())
		followed = true;

	return preceeded && followed;
}

ProseDocumentZoner::Classification ProseDocumentZoner::classifyWord(const LocatedString *contents, int start_offset, int end_offset)
{
	std::wstring word = contents->substringAsWString(start_offset, end_offset);
	if (isJunkyToken(word)) {
		//std::cout << "Junky token: " << UnicodeUtil::toUTF8StdString(word) << "\n";
		return NON_PROSE;
	}
	if (isHeaderToken(word, contents, start_offset, end_offset)) {
		//std::cout << "Header token: " << UnicodeUtil::toUTF8StdString(word) << "\n";
		return NON_PROSE;
	}

	//std::cout << "PROSE token: " << UnicodeUtil::toUTF8StdString(word) << "\n";
	return PROSE;

}

void ProseDocumentZoner::process(DocTheory* docTheory) {
	Document* document = docTheory->getDocument();

	std::vector<Region *> newRegions;
	for (int i = 0; i < document->getNRegions(); i++) {
		const Region *r = document->getRegions()[i];
		processRegion(document, r, newRegions);
	}
	document->takeRegions(newRegions);
}

// ProcessRegion will delete r, and replace it with a new Region in newRegions
void ProseDocumentZoner::processRegion(Document *document, const Region *r, std::vector<Region *> &newRegions) {
	const LocatedString* contents = r->getString();

	std::vector<Region*> regions;
	int word_start = -1;
	int i = 0;
	bool in_word = false;
	std::vector<ClassifiedWord> words;

	// Cycle over contents one at a time finding words based on whitespace
	// and classifying them when found
	while (true) {
		if (i >= contents->length()) {
			if (word_start > -1) {
				Classification classification = classifyWord(contents, word_start, i);
				words.push_back(ClassifiedWord(word_start, i, classification));
			}
			break;
		}

		wchar_t c = contents->charAt(i);

		if (!in_word) {
			if (iswspace(c)) {
				;
			} else {
				in_word = true;
				word_start = i;
			}
		}

		if (in_word) {
			if (iswspace(c)) {
				Classification classification = classifyWord(contents, word_start, i);
				words.push_back(ClassifiedWord(word_start, i, classification));
				in_word = false;
				word_start = -1;
			} else {
				;
			}
		}

		i++;
		continue;
	}

	// words vector is filled with all words in the document and they have all been classified
	// Find non-prose sections
	size_t current_word = 0;
	std::vector<OffsetPair> nonProseOffsets;
	while (true) {
		if (current_word >= words.size())
			break;
		int last_non_prose_index = -1;
		if (words[current_word].classification == NON_PROSE) {
			//std::cout << "Looking at: " << UnicodeUtil::toUTF8StdString(contents->substring(words[current_word].start_offset, words[current_word].end_offset)->toString()) << "\n";
			last_non_prose_index = getIndexOfLastNonProse(words, current_word);
			//std::cout << "Last non prose index : " << last_non_prose_index << "\n";
		}
		if (last_non_prose_index == -1) { // Didn't find non-prose section
			current_word++;
			continue;
		}
		// Found non-prose section
		nonProseOffsets.push_back(
			OffsetPair(words[current_word].start_offset, words[last_non_prose_index].end_offset));
		current_word = last_non_prose_index + 1;
	}

	int start_of_region = 0;
	BOOST_FOREACH (OffsetPair offsetPair, nonProseOffsets) {
		if (offsetPair.first > start_of_region) {
			// create PROSE region
			LocatedString *newRegionString = contents->substring(start_of_region, offsetPair.first);
			Region *proseRegion = new Region(document, PROSE_TAG, static_cast<int>(newRegions.size()), newRegionString);
			delete newRegionString;
			newRegions.push_back(proseRegion);
			//std::cout << "\n---------------------\nPROSE: \n";
			//std::cout << UnicodeUtil::toUTF8StdString(proseRegion->getString()->toString());
		}
		
		// create NON_PROSE region
		LocatedString *newRegionString = contents->substring(offsetPair.first, offsetPair.second);
		Region *nonProseRegion = new Region(document, NON_PROSE_TAG, static_cast<int>(newRegions.size()), newRegionString);
		delete newRegionString;
		newRegions.push_back(nonProseRegion);
		//std::cout << "\n---------------------\nNON PROSE: \n";
		//std::cout << UnicodeUtil::toUTF8StdString(nonProseRegion->getString()->toString());
		start_of_region = offsetPair.second;
	}
	if (start_of_region < contents->length()) {
		// final PROSE region
		LocatedString *newRegionString = contents->substring(start_of_region);
		Region *proseRegion = new Region(document, PROSE_TAG, static_cast<int>(newRegions.size()), newRegionString);
		delete newRegionString;
		std::wstring regionString = proseRegion->getString()->toString();
		boost::algorithm::trim(regionString);
		if (regionString.length() > 0) {
			newRegions.push_back(proseRegion);
			//std::cout << "\n---------------------\nPROSE: \n";
			//std::cout << UnicodeUtil::toUTF8StdString(proseRegion->getString()->toString());
		} else {
			delete proseRegion;
		}		
	}

	delete r;
}

int ProseDocumentZoner::getIndexOfLastNonProse(std::vector<ClassifiedWord> &words, size_t start_index) {
	std::deque<Classification> lastClassifications;
	int num_prose = 0;
	int num_non_prose = 0;
	size_t i = 0;

	for (i = start_index; i < words.size(); i++) {
		Classification c = words[i].classification;
		lastClassifications.push_back(c);
		if (c == NON_PROSE)
			num_non_prose++;
		else
			num_prose++;

		// Only consider last WINDOW_LENGTH classifications
		if (lastClassifications.size() > WINDOW_LENGTH) {
			Classification poppedClassification = lastClassifications[0];
			lastClassifications.pop_front();
			if (poppedClassification == NON_PROSE)
				num_non_prose--;
			else
				num_prose--;
		}

		// If the percent of non prose classifications in the window 
		// drop below a threshold, end the potential non-prose section
		if ((float)num_non_prose / (num_non_prose + num_prose) < THRESHOLD) {
			if (i - start_index < MIN_NON_PROSE_SECTION_LENGTH - 1)
				return -1;
			else
				break;
		}
	}

	// Back up i to the last NON_PROSE word also preceeded by a NON_PROSE
	if (i == words.size()) i--;
	size_t end_non_prose;
	for (end_non_prose = i; end_non_prose >= 1 && end_non_prose > start_index; end_non_prose--) {
		if (words[end_non_prose].classification == NON_PROSE && 
			words[end_non_prose - 1].classification == NON_PROSE)
		{
			break;
		}
	}

	if (end_non_prose - start_index < MIN_NON_PROSE_SECTION_LENGTH - 1)
		return -1;

	return static_cast<int>(end_non_prose);
}
