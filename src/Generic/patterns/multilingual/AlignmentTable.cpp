// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "AlignmentTable.h"

#include "LanguageVariant.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "Generic/common/UnexpectedInputException.h"


void AlignmentTable::loadAlignmentFile(std::string& alignmentFilename, std::string& alignmentString) {
	//this takes a string in the form sourcelanguage:sourcevariant=targetlanguage:targetvariant and makes language variant objects for it
	
	std::vector<std::string> alignment_split;
	boost::split(alignment_split, alignmentString, boost::is_any_of("="));

	if (alignment_split.size() != 2)return;

	std::vector<std::string> lang_var_split;
	Symbol language;
	Symbol variant;

	boost::split(lang_var_split, alignment_split[0], boost::is_any_of(":"));
	if (lang_var_split.size() != 2) return;
	language = Symbol(std::wstring(lang_var_split[0].begin(),lang_var_split[0].end()));
	variant = Symbol(std::wstring(lang_var_split[1].begin(),lang_var_split[1].end()));
	LanguageVariant_ptr sourceLanguageVariant = LanguageVariant::getLanguageVariant(language,variant);
																						

	boost::split(lang_var_split, alignment_split[1], boost::is_any_of(":"));
	if (lang_var_split.size() != 2) return;
	language = Symbol(std::wstring(lang_var_split[0].begin(),lang_var_split[0].end()));
	variant = Symbol(std::wstring(lang_var_split[1].begin(),lang_var_split[1].end()));
	LanguageVariant_ptr targetLanguageVariant = LanguageVariant::getLanguageVariant(language,variant);

	loadAlignmentFile(alignmentFilename, sourceLanguageVariant, targetLanguageVariant);
}

void AlignmentTable::loadAlignmentFile(std::string& alignmentFilename, const LanguageVariant_ptr& sourceLanguageVariant, 
	const LanguageVariant_ptr& targetLanguageVariant) {

		_alignmentFiles[sourceLanguageVariant][targetLanguageVariant] = alignmentFilename;
}

//Takes a file with sentences separated into lines and 1:2 3:1 token alignments separated by space for each sentence. 
//-1 is null token alignment
void AlignmentTable::_loadAlignments(const LanguageVariant_ptr& sourceLanguageVariant, const LanguageVariant_ptr& targetLanguageVariant) {

	if (_alignmentFiles.find(sourceLanguageVariant) != _alignmentFiles.end()) {
	
		std::string alignmentFilename = _alignmentFiles[sourceLanguageVariant][targetLanguageVariant];
		std::ifstream alignmentFile(alignmentFilename.c_str());

		if (alignmentFile.is_open()) {
			std::string line;
			int sent_no = 0;
			while (alignmentFile.good()) {
				getline(alignmentFile,line);

				std::vector<std::string> alignments;
				boost::split(alignments, line, boost::is_any_of(" "));

				BOOST_FOREACH(std::string alignment, alignments) {
					std::vector<std::string> alignPair;
					boost::split(alignPair, alignment, boost::is_any_of(":"));
					
					if (alignPair.size() == 2) {
						int sourceTokenIdx = boost::lexical_cast<int>(alignPair[0]);
						int targetTokenIdx = boost::lexical_cast<int>(alignPair[1]);
						_alignments[sourceLanguageVariant][targetLanguageVariant][sent_no][sourceTokenIdx].push_back(targetTokenIdx);
						_alignments[targetLanguageVariant][sourceLanguageVariant][sent_no][targetTokenIdx].push_back(sourceTokenIdx);
					}
				}

				sent_no++;
			}
		} else {
			std::wstringstream err;
			err << "Error reading alignments " 
				<< std::wstring(alignmentFilename.begin(), alignmentFilename.end());
			throw UnexpectedInputException("AlignmentTable::loadAlignmentFile", err);
		}
	} else if (_alignmentFiles.find(targetLanguageVariant) != _alignmentFiles.end()) {
		_loadAlignments(targetLanguageVariant,sourceLanguageVariant);
	}
}

//provide functions to support retrieving aligned elements
std::vector<int> AlignmentTable::getAlignment(const LanguageVariant_ptr& sourceLanguageVariant, 
	const LanguageVariant_ptr& targetLanguageVariant, int sentenceNo, int tokenIndex) {
	
		if (_alignments.find(sourceLanguageVariant) == _alignments.end() || 
			_alignments[sourceLanguageVariant].find(targetLanguageVariant) == _alignments[sourceLanguageVariant].end()) {

				_loadAlignments(sourceLanguageVariant,targetLanguageVariant);
		}
		return _alignments[sourceLanguageVariant][targetLanguageVariant][sentenceNo][tokenIndex];
}

std::vector<LanguageVariant_ptr> AlignmentTable::getAlignedLanguageVariants(const LanguageVariant_ptr& languageVariant) const {
	std::vector<LanguageVariant_ptr> result;

	for (AlignmentFileMap::const_iterator it = _alignmentFiles.begin(); it != _alignmentFiles.end(); ++it) {
		LanguageVariant_ptr source = it->first;
		AlignedLanguageFileMap langMap = _alignmentFiles.at(it->first);
		for (AlignedLanguageFileMap::const_iterator it2 = langMap.begin(); it2 != langMap.end(); ++it2) {
			LanguageVariant_ptr target = it2->first;
			if (*source == *languageVariant) {
				result.push_back(target);
			} else if (*target == *languageVariant) {
				result.push_back(source);
			}
		}
	}
	
	return result;
}

bool AlignmentTable::areAligned(const LanguageVariant_ptr& sourceLanguageVariant, const LanguageVariant_ptr& targetLanguageVariant) const {
	if (_alignmentFiles.find(sourceLanguageVariant) == _alignmentFiles.end()) {
		return areAligned(targetLanguageVariant,sourceLanguageVariant);
	} else { 
		return _alignmentFiles.at(sourceLanguageVariant).find(targetLanguageVariant) != _alignmentFiles.at(sourceLanguageVariant).end();
	}
}
