// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/preprocessors/VIKTRSSentenceSpan.h"
#include <xercesc/util/XMLString.hpp>

REGISTER_SPAN_CLASS(, VIKTRSSentenceSpan);

void VIKTRSSentenceSpan::loadXML(SerifXML::XMLTheoryElement spanElem) {
	using namespace SerifXML;
	static const XMLCh* X_original_sentence_index = xercesc::XMLString::transcode("pID");
	static const XMLCh* X_step_pos_id = xercesc::XMLString::transcode("sID");
	Span::loadXML(spanElem);
	_original_sentence_index = spanElem.getAttribute<int>(X_original_sentence_index);
	_step_pos_id = spanElem.getAttribute<int>(X_step_pos_id);
}

void VIKTRSSentenceSpan::saveXML(SerifXML::XMLTheoryElement spanElem, const Theory *context) const {
	using namespace SerifXML;
	static const XMLCh* X_original_sentence_index = xercesc::XMLString::transcode("pID");
	static const XMLCh* X_step_pos_id = xercesc::XMLString::transcode("sID");
	Span::saveXML(spanElem);
	spanElem.setAttribute(X_original_sentence_index, _original_sentence_index);
	spanElem.setAttribute(X_step_pos_id, _step_pos_id);
}
