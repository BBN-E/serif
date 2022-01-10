// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "Generic/theories/Theory.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Segment.h"
#include "Generic/common/Attribute.h"
#include "Generic/common/version.h"
#include <wchar.h>
#include <boost/optional.hpp>
#pragma warning(push, 0)
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#pragma warning(pop)
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/optional.hpp>


class Metadata;
class Region;
class Zoning;
class LocatedString;
typedef boost::shared_ptr<const LocatedString> LocatedString_ptr;

// TODO: At some point in the future we should add an array of "region"
//		 objects to keep track of properties or labels for a given block 
//		 of text from start_offset to end_offset (paragraph, italics, etc.).
//		 These regions should be retrievable by offset (i.e. what paragraph
//		 does offset x belong to?).
class Document : public Theory {
public:
	/** Constructs a new document object with the given name, number
	  * of text regions, and region text strings.
	  *
	  * @param name the document id
	  * @param n_regions the number of non-overlapping text regions
	  * @param regions the region text strings
	  */
	Document(Symbol name, int n_regions, LocatedString **regionStrings, LanguageAttribute language=SerifVersion::getSerifLanguage());

	/** Constructs a new document object with the given name, number
	  * of text regions, and Region objects.
	  *
	  * @param name the document id
	  * @param n_regions the number of non-overlapping text regions
	  * @param regions the Region objects
	  */
	Document(Symbol name, int n_regions, Region **regions, LanguageAttribute language=SerifVersion::getSerifLanguage());
	Document(Symbol name, LanguageAttribute language=SerifVersion::getSerifLanguage());

	Document(std::vector<Document*> splitDocuments);

	~Document();
	
	Symbol getName() const { return _name; }
	int getNRegions() const { return _n_regions; }
	const Region* const*  getRegions() const { return _regions; }

	
	// Segments and segment boundary offsets read by MTDocumentReader
	//  Kept with Document object for pass-through to MTResultCollector
	int getNSegments() const { return static_cast<int>(_segments.size()); }
	std::vector<WSegment>& getSegments() { return _segments; }
	void takeSegments(std::vector<WSegment>& segments) {
		_segments = segments;
	}

	void dump(std::ostream &out, int indent = 0);
	friend std::ostream &operator <<(std::ostream &out, Document &it)
		{ it.dump(out, 0); return out; }

	// For saving state:
	void updateObjectIDTable() const;
	void saveState(StateSaver *stateSaver) const;
	// For loading state:
	void resolvePointers(StateLoader * stateLoader);

	Metadata * getMetadata() const { return _metadata; }

	/// Sets the document's metadata.
	void setMetadata(Metadata *metadata);
	Symbol getSourceType() const { return _sourceType; };

	/// Sets the document's metadata.
	void setSourceType(Symbol sourcetype);

	void freeRegions(); // Call this to save memory when you know you won't be using the regions

	/// Don't call this more than once or the old regions will be leaked.
	void takeRegions(std::vector<Region*> regions);

	void takeZoning (Zoning* zoning);
	Zoning* getZoning() const {return _zoning;}

	void setIsDowncased(bool is_downcased) { DOWNCASED = is_downcased; }
	bool isDowncased() const { return DOWNCASED; }

	// The language used by the document
	LanguageAttribute getLanguage() const { return _language; }

	void copyOriginalText(Document *other);
	void setOriginalText(const LocatedString *text);
	void setDateTimeField(const LocatedString *datetime);
	const LocatedString* getOriginalText() const { return _originalText.get(); }
	const LocatedString* getDateTimeField() const { return _dateTimeField; }

	// For XML serialization:
	void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit Document(SerifXML::XMLTheoryElement elem);
	const wchar_t* XMLIdentifierPrefix() const;

	void setDocumentTimePeriod(boost::posix_time::ptime documentTimeStart, boost::posix_time::ptime documentTimeEnd);
	void setDocumentTimePeriod(boost::posix_time::ptime documentTimeStart, boost::posix_time::time_duration duration);
	boost::optional<boost::posix_time::time_period> getDocumentTimePeriod() const;
	/* If document time period is within one day, return that day, otherwise return boost::none */
	boost::optional<boost::gregorian::date> getDocumentDate() const;
	void setDocumentTimeZone(boost::local_time::posix_time_zone timeZone);
	boost::optional<boost::local_time::posix_time_zone> getDocumentTimeZone() const;

	std::wstring normalizeDocumentDate() const;

#if 0 // I removed this for now for purposes of leak detection
	// memory stuff
	Document* next;
	static void* operator new(size_t n, int, char *, int) { return operator new(n); }
	static void* operator new(size_t);
    static void operator delete(void* object);
#endif

	const std::wstring &getURL() const { return _url; }
	void setURL(const std::wstring &url) { _url = url; }
protected:
	Symbol _name;
	int _n_regions;
	Region **_regions;
	Zoning *_zoning;
	Metadata * _metadata;
	static int downcaseAllUppercaseDocs();
	Symbol _sourceType;
	bool DOWNCASED;
	LocatedString_ptr _originalText;
	const LocatedString *_dateTimeField;
	boost::optional<boost::posix_time::time_period> _documentTimePeriod;
	boost::optional<boost::local_time::posix_time_zone> _documentTimeZone;

	/** The language used by this document.  When a new Document is created,
	  * this is initialized to the current build language (unless otherwise
	  * specified). */
	LanguageAttribute _language;

	// added 11/9/06 by JSG for pass-support of the segment file format (see common/Segment.h)
	std::vector<WSegment> _segments;

	int getCaseFromRegions(int n_regions, Region **regions);

	int getCaseFromRegions(std::vector<Region*> regions);

	// Helper function
	void addTimeZone(std::stringstream &ss, const boost::local_time::posix_time_zone &timeZone) const;


	enum { MIXED, UPPER, LOWER };

	/** A file://... or http://... URL indicating the location of the
	 * original text for this document.  When a DocTheory is
	 * serialized as XML, and this value is defined (non-empty), a
	 * serialization option (save_original_text_as_url) can be used to
	 * indicate that the original text should be saved as a URL,
	 * rather than including it inline.  If this option is used, then
	 * the serialized file may only be read if the original text is
	 * still available at the same location. */
	std::wstring _url;
};

#endif
