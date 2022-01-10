// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "ICEWS/EventMentionSet.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"
#include <xercesc/util/XMLString.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

namespace ICEWS {

ICEWSEventMentionSet::ICEWSEventMentionSet() {}
ICEWSEventMentionSet::~ICEWSEventMentionSet() {}


void ICEWSEventMentionSet::addEventMention(ICEWSEventMention_ptr emention) {
	_event_mentions.push_back(emention);
}

void ICEWSEventMentionSet::removeEventMentions(std::set<ICEWSEventMention_ptr> ementions) {
	if (ementions.empty()) return;
	std::vector<ICEWSEventMention_ptr> filtered;
	BOOST_FOREACH(ICEWSEventMention_ptr emention, _event_mentions) {
		if (ementions.find(emention) == ementions.end())
			filtered.push_back(emention);
	}
	_event_mentions.swap(filtered);
}



void ICEWSEventMentionSet::updateObjectIDTable() const {
	throw InternalInconsistencyException("ICEWSEventMentionSet::updateObjectIDTable",
		"ICEWSEventMentionSet does not currently have state file support");
}
void ICEWSEventMentionSet::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("ICEWSEventMentionSet::saveState",
		"ICEWSEventMentionSet does not currently have state file support");
}
void ICEWSEventMentionSet::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("ICEWSEventMentionSet::resolvePointers",
		"ICEWSEventMentionSet does not currently have state file support");
}

ICEWSEventMentionSet::ICEWSEventMentionSet(SerifXML::XMLTheoryElement elem, const DocTheory* theory) {
	static const XMLCh* X_ICEWSEventMention = xercesc::XMLString::transcode("ICEWSEventMention");

	using namespace SerifXML;
	elem.loadId(this);
	XMLTheoryElementList emElems = elem.getChildElementsByTagName(X_ICEWSEventMention);
	size_t n_ementions = emElems.size();
	for (size_t i=0; i<n_ementions; ++i) {
		_event_mentions.push_back(boost::make_shared<ICEWSEventMention>(emElems[i]));
	}
}

void ICEWSEventMentionSet::saveXML(SerifXML::XMLTheoryElement elem, const Theory *context) const {
	static const XMLCh* X_ICEWSEventMention = xercesc::XMLString::transcode("ICEWSEventMention");

	size_t n_ementions = _event_mentions.size();
	for (size_t i=0; i<n_ementions; ++i) {
		elem.saveChildTheory(X_ICEWSEventMention, _event_mentions[i].get());
	}
}


} // end of ICEWS namespace
