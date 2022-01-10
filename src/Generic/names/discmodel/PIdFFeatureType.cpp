// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

Symbol PIdFFeatureType::modeltype = Symbol(L"PIdF");

std::set<Symbol> PIdFFeatureType::_unigramVocab = std::set<Symbol>();
std::map<Symbol,std::set<Symbol> > PIdFFeatureType::_bigramVocab = std::map<Symbol,std::set<Symbol> >();

void PIdFFeatureType::setUnigramVocab(const std::string vocabFile) {
	if (!_unigramVocab.empty()) {
		throw InternalInconsistencyException(
			"PIdFFeatureType::setUnigramVocab()",
			"Caller tried to load vocabulary after it has already been loaded.");
	}
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(vocabFile));
	UTF8InputStream& stream(*stream_scoped_ptr);
	if (stream.fail()) {
		std::ostringstream err;
		err << "Problem opening " << vocabFile;
		throw UnexpectedInputException("PIdFFeatureType::setUnigramVocab()", err.str().c_str());
	}
	while (!(stream.eof() || stream.fail())) {
		std::wstring line;
		std::getline(stream, line);
		boost::trim(line);
		if (!line.empty() && line.find_first_of(L'(') == 0) {
			line = line.substr(2, line.length()-3); // Remove leading/trailing parens
			if (boost::lexical_cast<int>(line.substr(line.find_last_of(L' ')+1)) > VOCAB_MIN_FREQUENCY) {
				_unigramVocab.insert(Symbol(line.substr(0,line.find_first_of(L')'))));
			}
		}
	}
}
void PIdFFeatureType::setBigramVocab(const std::string bigramVocabFile) {
	if (!_bigramVocab.empty()) {
		throw InternalInconsistencyException(
			"PIdFFeatureType::setBigramVocab()",
			"Caller tried to load vocabulary after it has already been loaded.");
	}
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(bigramVocabFile));
	UTF8InputStream& stream(*stream_scoped_ptr);
	if (stream.fail()) {
		std::ostringstream err;
		err << "Problem opening " << bigramVocabFile;
		throw UnexpectedInputException("PIdFFeatureType::setUnigramVocab()", err.str().c_str());
	}
	while (!(stream.eof() || stream.fail())) {
		std::wstring line;
		std::getline(stream, line);
		boost::trim(line);
		if (!line.empty() && line.find_first_of(L'(') == 0) {
			line = line.substr(2, line.length()-3); // Remove leading/trailing parens
			if (boost::lexical_cast<int>(line.substr(line.find_last_of(L' ')+1)) > VOCAB_MIN_FREQUENCY) {
				size_t space = line.find_first_of(L' ');
				_bigramVocab[Symbol(line.substr(0,space))].insert(
					Symbol(line.substr(space+1,line.find_first_of(L')')-space-1)));
			}
		}
	}
}

bool PIdFFeatureType::isVocabWord(const Symbol word) {
	return _unigramVocab.empty() || _unigramVocab.find(word) != _unigramVocab.end();
}
bool PIdFFeatureType::isVocabBigram(const Symbol word1, const Symbol word2) {
	return _bigramVocab.empty() || (_bigramVocab.find(word1) != _bigramVocab.end() &&
									_bigramVocab[word1].find(word2) != _bigramVocab[word1].end());
}
