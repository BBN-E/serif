// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/descriptors/DescriptorClassifierTrainer/en_PartitiveFinder.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/common/ParamReader.h"
#include "English/parse/en_STags.h"
#include "English/common/en_WordConstants.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/SymbolListMap.h"
#include <string.h>
#include <boost/scoped_ptr.hpp>

int EnglishPartitiveFinder::_n_partitive_headwords = 0;
Symbol EnglishPartitiveFinder::_partitiveHeadwords[1000];


const SynNode *EnglishPartitiveFinder::getPartitiveHead(const SynNode *node)
{
	if (_n_partitive_headwords == 0)
		initializeSymbols();

	// see if head node is considered a partitive headword
	if (!isPartitiveHeadWord(node->getHeadWord()))
		return 0;

	// find a PP child
	const SynNode *ppNode = 0;
	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->getTag() == EnglishSTags::PP) {
			if (ppNode != 0) {
				// no good -- only one pp allowed
				return 0;
			}
			else {
				ppNode = child;
			}
		}
	}
	if (ppNode == 0)
		return 0;

	if (ppNode->getHeadWord() != EnglishWordConstants::OF)
		return 0;

	const SynNode *lastChild = ppNode->getChild(ppNode->getNChildren() - 1);
// TODO: not sure about this one!
//	if (!lastChild->hasMention())
//		return 0;

	return lastChild;
}

bool EnglishPartitiveFinder::isPartitiveHeadWord(Symbol sym) {
	for (int i = 0; i < _n_partitive_headwords; i++) {
		if (sym == _partitiveHeadwords[i])
			return true;
	}
	return false;
}




void EnglishPartitiveFinder::initializeSymbols() {

	// now read in partitive head-words
	std::string ph_list = ParamReader::getRequiredParam("partitive_headword_list");
	boost::scoped_ptr<UTF8InputStream> wordStream_scoped_ptr(UTF8InputStream::build(ph_list.c_str()));
	UTF8InputStream& wordStream(*wordStream_scoped_ptr);

	_n_partitive_headwords = 0;
	while (!wordStream.eof() && _n_partitive_headwords < 1000) {
		wchar_t line[100];
		wordStream.getLine(line, 100);
		if (line[0] != '\0')
			_partitiveHeadwords[_n_partitive_headwords++] = Symbol(line);
	}

	wordStream.close();
}




