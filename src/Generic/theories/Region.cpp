// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/Region.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <wchar.h>
#include "Generic/reader/DefaultDocumentReader.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLTheoryElement.h"


// Note: Region formerly kept a pointer to the document that contained it.
// But it was never used, and made things a little more difficult for 
// serialization, so I removed the pointer.  If you want to add it back, then
// be sure to modify Document::Document(SerifXML::XMLTheoryElement documentElem)
// appropriately (in particular, the new Regions created during zoning will
// need to have their document pointers adjusted when they are "stolen").
Region::Region(const Document* document, Symbol tagName, int region_no, LocatedString *content)
	: _tagName(tagName), _region_no(region_no), 
	_is_speaker_region(false), _is_receiver_region(false), _content_flags(0x00) 
{
	if(content != 0) {
		_content = _new LocatedString(*content);
	}
	else {
		_content = 0;
	}
}

Region::~Region() {
	delete _content;
}

void Region::toLowerCase() {
	if (_content != 0) 
		_content->toLowerCase();
}

void Region::dump(std::ostream &out, int indent) {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "(region " << _region_no << "):";
	if (_content != 0) {
		out << newline;
		_content->dump(out, indent + 2);
	}
	delete[] newline;
}

void Region::updateObjectIDTable() const {
	throw InternalInconsistencyException("Region::updateObjectIDTable()",
										"Using unimplemented method.");
}

void Region::saveState(StateSaver *stateSaver) const {
	throw InternalInconsistencyException("Region::saveState()",
										"Using unimplemented method.");

}

void Region::resolvePointers(StateLoader * stateLoader) {
	throw InternalInconsistencyException("Region::resolvePointers()",
										"Using unimplemented method.");

}

const wchar_t* Region::XMLIdentifierPrefix() const {
	return L"region";
}

void Region::saveXML(SerifXML::XMLTheoryElement regionElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("Region::saveXML", "Expected context to be NULL");
	if (!getRegionTag().is_null()) {
		regionElem.setAttribute(X_tag, getRegionTag());
	}
	regionElem.setAttribute(X_is_speaker, isSpeakerRegion());
	regionElem.setAttribute(X_is_receiver, isReceiverRegion());
	getString()->saveXML(regionElem);
	// Note: we don't serialize getRegionNumber(), since it's redundant -- 
	// in particular, the i-th region in a document will be given region
	// number i.
}

Region::Region(SerifXML::XMLTheoryElement regionElem, size_t region_no)
: _region_no(static_cast<int>(region_no)), _tagName(), 
  _is_speaker_region(false), _is_receiver_region(false), _content(0)
{
	using namespace SerifXML;
	regionElem.loadId(this);
	_tagName = regionElem.getAttribute<Symbol>(X_tag, Symbol());
	_is_speaker_region = regionElem.getAttribute<bool>(X_is_speaker, false);
	_is_receiver_region = regionElem.getAttribute<bool>(X_is_receiver, false);
	_content = _new LocatedString(regionElem);
}
