// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEFAULT_DOCUMENT_READER_H
#define DEFAULT_DOCUMENT_READER_H

#include "Generic/theories/Document.h"
#include "Generic/common/limits.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/limits.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/reader/SGMLTag.h"
#include "Generic/theories/Zone.h"
#include <time.h>
#include <iostream>

typedef std::basic_istream<wchar_t> InputStream;

#define MAX_DOC_REGION_TYPES 100
#define MAX_DOC_REGION_BREAKS 100
#define MAX_DOC_ID_TYPES 10
#define MAX_DOC_DATETIME_TYPES 10
#define MAX_DOC_SPEAKER_TYPES 10
#define MAX_DOC_RECEIVER_TYPES 10

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED DefaultDocumentReader: public DocumentReader {
public:

	DefaultDocumentReader();

	/**
	  * Reads from strm until it reaches the end of 
	  * file and returns a pointer to a document object
	  * containing the located raw text of the file, 
	  * ignoring only text surrounded by '<' and '>'.
	  * The client is responsible for deleting the
	  * document.
	  *
	  * @param stream the input stream to read from.
	  */
	virtual Document* readDocument(InputStream &stream, const wchar_t * filename);
	virtual Document* readDocument(const LocatedString *source, const wchar_t * filename);
	virtual Document* readDocument(const LocatedString *source, const wchar_t * filename, Symbol inputType);
	virtual Document* readDocument(InputStream &stream, const wchar_t * filename, Symbol inputType);

	static Symbol TEXT_SYM;
	static Symbol SGM_SYM;
	static Symbol AUTO_SYM;
	static Symbol OSCTEXT_SYM;

	static Symbol ERE_WKSP_AUTO_SYM;
	static Symbol PROXY_SYM;

	static Symbol DF_SYM;

//	static Symbol 
	/// Removes annotations and unrecognized tags and translates SGML entities.
	virtual void cleanRegion(LocatedString *region, Symbol inputType);
	
protected:

	typedef struct {
		int start;
		int end;
		Symbol tag;
		int open_tag_length;
		int close_tag_length;
	} RegionInfo;
	
	typedef struct {
		int start;
		int end;
		Symbol tag;
		SGMLTag realTag;
		std::vector<Zone*> children;
		LocatedString* pure_content;
	} ZoneInfo;

	std::vector<Region*> _regions;
	Symbol _inputType;
	int _rawtext_doc_count;
	bool _use_filename_as_docid;
	int _docid_includes_directory_levels;
	bool _calculate_edt_offsets;
	bool _offsets_start_at_first_tag;
	bool _doc_reader_replace_invalid_xml_characters;

	Symbol _regionTypes[MAX_DOC_REGION_TYPES];
	int _n_region_types;

	std::set<Symbol> _zoneTypes;
	std::set<Symbol> _authorAttributes;
	std::set<Symbol> _datetimeAttributes;

	Symbol _regionBreaks[MAX_DOC_REGION_BREAKS];
	int _n_region_breaks;

	Symbol _speakerTypes[MAX_DOC_SPEAKER_TYPES];
	int _n_speaker_types;

	Symbol _receiverTypes[MAX_DOC_RECEIVER_TYPES];
	int _n_receiver_types;
	
	Symbol _docIDTypes[MAX_DOC_ID_TYPES];
	int _n_id_types;

	Symbol _docDateTimeTypes[MAX_DOC_DATETIME_TYPES];
	int _n_datetime_types;

	// collected tags that must be parsable if present;
	Symbol _relevantTags[MAX_DOC_DATETIME_TYPES+MAX_DOC_REGION_TYPES+MAX_DOC_ID_TYPES];
	int _n_relevantTags;

	/// Identifies regions in string and assigns ownership to document
	void identifyRegions(Document *document, const LocatedString *string, Symbol inputType);

	/// Searches through regions for datetime candidate
	LocatedString* findDateTimeTag(Document *document) const;
	
	/// Turns raw text into something that can be processed by Serif
	void processRawText(LocatedString *document);

	// Check for invalid XML chars; based on _doc_reader_replace_invalid_xml_characters either crash or replace them
	// Called by DefaultDocumentReader::readDocument()
	void handleInvalidXmlCharacters(LocatedString *locatedString);

	// Return a name Symbol containing a name string for the next doc
	Symbol getNextRawTextDocName();

	// tests whether the relevant SGML/XML tags parse okay
	bool checkRelevantTagsParsable(const LocatedString& source, const wchar_t* filename,
		bool throwExceptionIfNotParsable);

	// tests whether the given tag is in the _speakerTypes list
	bool isSpeakerTag(Symbol tag);

	// tests whether the given tag is in the _receiverTypes list
	bool isReceiverTag(Symbol tag);

	// tests whether text contains newline-justified text ("fixed"-width)
	bool isJustifiedText(const LocatedString* text);

	// tests whether text contains lines separated by multiple newlines (double spaced)
	bool isDoubleSpacedText(const LocatedString* text);
	
	/// Removes path from file name and returns as Symbol
	Symbol cleanFileName(const wchar_t* file) const;
	Symbol removeFileExtension(const wchar_t* file) const;
	Symbol keepDirectoryLevels(Symbol name, const wchar_t* file) const;

	/// Converts a comma-delimiter parameter value to a Symbol array
	int convertCommaDelimitedListToSymbolArray(const char *str, Symbol *result, int max_results);

	/// Converts a comma-delimiter parameter value to a Symbol set
	void convertCommaDelimitedListToSymbolSet(const char *str, std::set<Symbol>& result);

	// Creates a LocatedString for the next region of text that were annotated in LDC-released FBIS Data
	int getNextFBISRegion(const LocatedString* source, int startoffset, LocatedString*& resgionSubstring);
	int getNextBracektRegionToRemove(const LocatedString* source, int searchstart, int& start, int& end);

	// tests whether it is a XML file with <text></text> region
	bool checkXmlHasText(const LocatedString* source);

	// creates a LocatedString for the next region of text that were annotated in LDC-released Proxy data
	int getNextProxyRegion(const LocatedString* source, int startoffset, LocatedString*& resgionSubstring);

	// creates a LocatedString for the next region of text that were annotated in KBP pilot discussion_forum data
	void identifyDFZones(Document *document, const LocatedString *string, Symbol inputType);


	void printAllRegexMatches();

	void removeOtherTags(LocatedString *region);
	void cleanDFPostQuote(LocatedString *region);


};

#endif
