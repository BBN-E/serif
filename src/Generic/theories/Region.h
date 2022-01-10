// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef REGION_H
#define REGION_H

#include "Generic/theories/Theory.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/LocatedString.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class Document;

class SERIF_EXPORTED Region : public Theory {
public:
	Region(const Document* document, Symbol tagName, int region_no, LocatedString *content);
	~Region();

	Symbol getRegionTag() const { return _tagName; }
	int getRegionNumber() const { return _region_no; }
	EDTOffset getStartEDTOffset() const { return _content->start<EDTOffset>(); }
	EDTOffset getEndEDTOffset() const { return _content->end<EDTOffset>(); }
	const LocatedString *getString() const { return _content; }

	bool isSpeakerRegion() const { return _is_speaker_region; }
	void setSpeakerRegion(bool flag) { _is_speaker_region = flag; }

	bool isReceiverRegion() const { return _is_receiver_region; }
	void setReceiverRegion(bool flag) { _is_receiver_region = flag; }

	void toLowerCase();

	void dump(std::ostream &out, int indent = 0);
	friend std::ostream &operator <<(std::ostream &out, Region &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	const wchar_t* XMLIdentifierPrefix() const;
	explicit Region(SerifXML::XMLTheoryElement elem, size_t region_no);

	typedef enum {
		JUSTIFIED = 0x01,
		DOUBLE_SPACED = 0x02,
	} ContentFlag;

	void addContentFlag(ContentFlag flag) { _content_flags |= flag; }
	void removeContentFlag(ContentFlag flag) { _content_flags &= ~flag; }
	bool hasContentFlag(ContentFlag flag) const { return ((_content_flags & flag) > 0x00); }

private:
	Symbol _tagName;
	int _region_no;
	bool _is_speaker_region;
	bool _is_receiver_region;
	int _content_flags;
	LocatedString *_content;

private: // Deprecated -- these were poorly named.
	int getStartCharNum(); // Use getStartEDTOffset() instead.
	int getEndCharNum(); // Use getStartEDTOffset() instead.
};

#endif
