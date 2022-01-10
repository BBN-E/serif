/*
#include "Generic/common/leak_detection.h"
#include "EventUtilities.h"

#include "Generic/common/Symbol.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"

using std::wstring;

wstring ALEventUtilities::anchorArg(const EventMention* event) {
	if (const SynNode* node = event->getAnchorNode()) {
		wstring ret = L"parseHW=";
		ret += node->getHeadWord().to_string();
		return ret;
	} else if (const Proposition* prop = event->getAnchorProp()) {
		const SynNode* node = prop->getPredHead();
		wstring ret = L"propHw=";
		ret += node->getHeadWord().to_string();
		return ret;
	}
	return L"";
}
*/
