#ifndef _AL_EVENT_UTILITIES_H_
#define _AL_EVENT_UTILITIES_H_

#include <string>
#include "Generic/common/Symbol.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Proposition.h"

//class EventMention;

class ALEventUtilities {
public:
	static std::wstring anchorArg(const EventMention* event) {
		if (const SynNode* node = event->getAnchorNode()) {
			std::wstring ret = L"parseHW=";
			ret += node->getHeadWord().to_string();
			return ret;
		} else if (const Proposition* prop = event->getAnchorProp()) {
			const SynNode* node = prop->getPredHead();
			std::wstring ret = L"propHw=";
			ret += node->getHeadWord().to_string();
			return ret;
		}
		return L"";
}
};

#endif
