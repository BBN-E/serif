// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/common/en_NationalityRecognizer.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/NodeInfo.h"
#include "English/parse/en_STags.h"

static Symbol one_sym = Symbol(L"one");

bool EnglishNationalityRecognizer::isNamePersonDescriptor(const SynNode *node) {
	// first make sure the head is a nationality word
	if (!isNationalityWord(node->getHeadWord()))
		return false;

	if (isCertainNationalityWord(node->getHeadWord()))
		return true;

	// now make sure that if it's in an NP, there are no other NPs alongside it
	if (node->getParent() != 0) {
		const SynNode *parent = node->getParent();

		// "the American, one American, an American"
		if (parent->getHead() == node &&
			parent->getChild(parent->getNChildren() - 1) == node) 
		{
			const SynNode *firstChild = parent->getChild(0);
			if (firstChild->getHeadPreterm()->getTag() == EnglishSTags::DT ||
				firstChild->getHeadPreterm()->getTag() == EnglishSTags::CD ||
				firstChild->getHeadWord() == one_sym)
			{
				return true;
			}
		}

		bool other_reference_found = false;
		for (int i = 0; i < node->getParent()->getNChildren(); i++) {
			if (parent->getChild(i)->hasMention() ||
				NodeInfo::canBeNPHeadPreterm(parent->getChild(i)) ||
				(parent->getHead() == parent->getChild(i) && parent->hasMention()))
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

bool EnglishNationalityRecognizer::isRegionWord(Symbol word) {
	wchar_t buf[MAX_TOKEN_SIZE + 1];
	wcsncpy(buf, word.to_string(), MAX_TOKEN_SIZE + 1);
	std::wstring wbuf (buf);
	std::transform(wbuf.begin(), wbuf.end(), wbuf.begin(), towlower);

	return wbuf.compare(L"baltic") == 0;
}
