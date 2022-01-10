// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/NationalityRecognizer.h"
#include "Generic/common/xx_NationalityRecognizer.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

bool NationalityRecognizer::_initialized = false;

hash_set<Symbol,
		 NationalityRecognizer::HashKey,
		 NationalityRecognizer::EqualKey>
	NationalityRecognizer::_natNames(500);

hash_set<Symbol,
		 NationalityRecognizer::HashKey,
		 NationalityRecognizer::EqualKey>
	NationalityRecognizer::_certainNatNames(500);


void NationalityRecognizer::initialize() {

	if (_initialized)
		return;

	std::string nat_file = ParamReader::getParam("nationality_certain_list");
	if (!nat_file.empty()) {
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(nat_file.c_str());
		if (in.fail()) {
			std::stringstream errMsg;
			errMsg << "Unable to open " << nat_file << " specified by parameter 'nationality_certain_list'";
			throw UnexpectedInputException(
				"NationalityRecognizer::initialize()",
				errMsg.str().c_str());
		}

		while (!in.eof()) {
			wchar_t line[100];
			in.getLine(line, 100);
			if (line[0] != L'\0' && line[0] != L'#') {
				_natNames.insert(Symbol(line));
				_certainNatNames.insert(Symbol(line));
			}
		}
	}

	nat_file = ParamReader::getParam("nationality_likely_list");
	if (!nat_file.empty()) {
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(nat_file.c_str());
		if (in.fail()) {
			std::stringstream errMsg;
			errMsg << "Unable to open " << nat_file << " specified by parameter 'nationality_likely_list'";
			throw UnexpectedInputException(
				"NationalityRecognizer::initialize()",
				errMsg.str().c_str());
		}

		while (!in.eof()) {
			wchar_t line[100];
			in.getLine(line, 100);
			if (line[0] != L'\0' && line[0] != L'#') {
				_natNames.insert(Symbol(line));
			}
		}
	}

	_initialized = true;
}


bool NationalityRecognizer::isNationalityWord(Symbol word) {
	if (!_initialized)
		initialize();

	// look for lower-cased version of token symbol in nat list
	wchar_t buf[MAX_TOKEN_SIZE + 1];
	wcsncpy(buf, word.to_string(), MAX_TOKEN_SIZE + 1);
	std::wstring wbuf (buf);
	std::transform(wbuf.begin(), wbuf.end(), wbuf.begin(), towlower);

	return _natNames.find(Symbol(wbuf.c_str())) != _natNames.end();
}

bool NationalityRecognizer::isCertainNationalityWord(Symbol word) {

	if (!_initialized)
		initialize();

	// look for lower-cased version of token symbol in certain-nat list
	wchar_t buf[MAX_TOKEN_SIZE + 1];
	wcsncpy(buf, word.to_string(), MAX_TOKEN_SIZE + 1);
	std::wstring wbuf (buf);
	std::transform(wbuf.begin(), wbuf.end(), wbuf.begin(), towlower);

	return _certainNatNames.find(Symbol(wbuf.c_str())) != _natNames.end();
}

bool NationalityRecognizer::isLowercaseNationalityWord(Symbol word) {
	if (!_initialized)
		initialize();

	return _natNames.find(word) != _natNames.end();
}

bool NationalityRecognizer::isLowercaseCertainNationalityWord(Symbol word) {

	if (!_initialized)
		initialize();

	return _certainNatNames.find(word) != _natNames.end();
}


/*
bool NationalityRecognizer::isNamePersonDescriptor(const SynNode *node) {
	// first make sure the head is a nationality word
	if (!isNationalityWord(node->getHeadWord()))
		return false;

	if (isCertainNationalityWord(node->getHeadWord()))
		return true;

	// now make sure that if it's in an NP, there are no other NPs
	// alongside it
	if (node->getParent() != 0) {
		const SynNode *parent = node->getParent();

		bool other_reference_found = false;
		for (int i = 0; i < node->getParent()->getNChildren(); i++) {
			if (parent->getChild(i)->hasMention() ||
				NodeInfo::canBeNPHeadPreterm(parent->getChild(i)))
			{
				if (parent->getChild(i) != node) {
					other_reference_found = true;
					break;
				}
			}
		}

		if (other_reference_found)
			return false;
	}

	return true;
}



*/


boost::shared_ptr<NationalityRecognizer::Factory> &NationalityRecognizer::_factory() {
	static boost::shared_ptr<NationalityRecognizer::Factory> factory(new GenericNationalityRecognizerFactory());
	return factory;
}

