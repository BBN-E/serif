// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/SynNode.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/MentionSet.h"
#include "English/descriptors/en_TemporalIdentifier.h"
#include "English/common/en_WordConstants.h"
#include "English/parse/en_STags.h"

#include <string>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

using namespace std;

Symbol::HashSet *TemporalIdentifier::_temporalWords = 0;

void TemporalIdentifier::freeTemporalHeadwordList() {
	delete _temporalWords;
	_temporalWords = 0;
}

void TemporalIdentifier::loadTemporalHeadwordList() {
	if (_temporalWords != 0)
		return;

	std::string temp_head_file = ParamReader::getRequiredParam("temporal_headword_list");

	boost::scoped_ptr<UTF8InputStream> tempHeadStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& tempHeadStream(*tempHeadStream_scoped_ptr);
	tempHeadStream.open(temp_head_file.c_str());
	if (tempHeadStream.fail()) {
		std::string error = "Unable to open file specified by parameter 'temporal_headword_list':\n" + temp_head_file;
		throw UnexpectedInputException("EnglishDescriptorClassifier::EnglishDescriptorClassifier()", error.c_str());
	}

	_temporalWords = _new Symbol::HashSet(1000);
	while (!tempHeadStream.eof()) {
		wstring word;
		tempHeadStream.getLine(word);

		size_t pos = word.find(L'\n');
		if (pos != wstring::npos)
			word.resize(pos);
		pos = word.find(L'\r');
		if (pos != wstring::npos)
			word.resize(pos);

		Symbol sym = Symbol(word.c_str());
		_temporalWords->insert(sym);
	}

	tempHeadStream.close();
}

bool TemporalIdentifier::looksLikeTemporal(const SynNode *node, const MentionSet* mentionSet, bool no_embedded) {
	if (_temporalWords == 0) {
		loadTemporalHeadwordList();
	}
		
	if (no_embedded && node->getParent() != 0 &&
		node->getParent()->hasMention() &&
		looksLikeTemporal(node->getParent(), mentionSet)) {
		return false;
	}

	if (node->getTag() == EnglishSTags::DATE) {
		return true;
	}

	Symbol word = node->getHeadWord();

	if (word == EnglishWordConstants::BEGINNING ||
		word == EnglishWordConstants::START ||
		word == EnglishWordConstants::END ||
		word == EnglishWordConstants::CLOSE ||
		word == EnglishWordConstants::MIDDLE) {
		const SynNode *PP = 0;
		for (int i = 0; i < node->getNChildren(); i++) {
			if (node->getChild(i)->getTag() == EnglishSTags::PP) {
				PP = node->getChild(i);
				break;
			}
		}
		if (PP != 0 &&
			PP->getHeadWord() == EnglishWordConstants::OF &&
			PP->getNChildren() >= 2 &&
			looksLikeTemporal(PP->getChild(1), mentionSet)) {
			return true;
		}
	}

	return _temporalWords->find(word) != _temporalWords->end();
}
