// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENTENCE_H
#define SENTENCE_H

#define SENTENCE_BLOCK_SIZE 1000

#include "Generic/theories/Theory.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/LocatedString.h"

#include <wchar.h>

class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;
class Document;
class Region;
class Zone;

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED Sentence : public Theory {
private:
	// memory stuff
    static const size_t _block_size;
	static Sentence* _freeList;
	bool _isAnnotated;

public:
	Sentence(const Document *doc, const Region *region, int sent_no, const LocatedString *content);
	//Sentence(const Document *doc, const Zone *zone, int sent_no, const LocatedString *content);
	~Sentence();

	const Document *getDocument() const { return _doc; }
	const Region *getRegion() const { return _region; }
	//const Zone *getZone() const { return _zone; }
	
	int getSentNumber() const { return _sent_no; }
	EDTOffset getStartEDTOffset() const { return _content->start<EDTOffset>(); }
	EDTOffset getEndEDTOffset() const { return _content->end<EDTOffset>(); }
	CharOffset getStartCharOffset() const { return _content->start<CharOffset>(); } 
	CharOffset getEndCharOffset() const { return _content->end<CharOffset>(); } 
	int getNChars() const { return _n_chars; }
	const LocatedString *getString() const { return _content; }
	void setAnnotationFlag(bool flag);
	bool isAnnotated() const {return _isAnnotated;}

	enum { MIXED, UPPER, LOWER };
	int getSentenceCase() const;

	// Don't use this
	void reorderAs(int i) { _sent_no = i; }

	void dump(std::ostream &out, int indent = 0);
	friend std::ostream &operator <<(std::ostream &out, Sentence &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	void resolvePointers(StateLoader * stateLoader);
	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Sentence(SerifXML::XMLTheoryElement elem, const Document *doc, int sent_no);
	const wchar_t* XMLIdentifierPrefix() const;

protected:
	const Document *_doc;
	const Region *_region;
	//const Zone *_zone;
	int _sent_no;
	int _n_chars;
	const LocatedString *_content;
};

#endif
