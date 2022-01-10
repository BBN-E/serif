// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SERIF_XML_RESULT_COLLECTOR_H
#define SERIF_XML_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include <boost/algorithm/string/predicate.hpp>

class SerifXMLResultCollector : public ResultCollector
{
public:
	typedef std::map<SerifXML::xstring, SerifXML::xstring> OptionMap;
private:
	DocTheory *_docTheory; // we don't own this.
	OptionMap *_optionMap; // we don't own this.
public:
	SerifXMLResultCollector(OptionMap* optionMap=0): _docTheory(0), _optionMap(optionMap) {}
	virtual ~SerifXMLResultCollector() {}
	virtual void finalize() {}

	virtual void loadDocTheory(DocTheory* docTheory) { 
		_docTheory = docTheory; 
	}	
	virtual void produceOutput(const wchar_t *output_dir, const wchar_t* document_filename) {
		std::wstring filename = std::wstring(output_dir) + LSERIF_PATH_SEP + std::wstring(document_filename);
		if (!boost::algorithm::ends_with(filename, L".xml"))
			filename += L".xml";
		SerifXML::XMLSerializedDocTheory(_docTheory).save(filename);
	}
	virtual void produceOutput(std::wstring *results) {
		std::stringstream result_stream;
		if (_optionMap) {
		    SerifXML::XMLSerializedDocTheory(_docTheory, *_optionMap).save(result_stream);
		} else {
		    SerifXML::XMLSerializedDocTheory(_docTheory).save(result_stream);
		}
		// Convert utf8->unicode.  There are better ways to do this, but this works for now.
		*results = UnicodeUtil::toUTF16StdString(result_stream.str());
	}
};

#endif
