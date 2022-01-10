// Copyright 2016 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/results/MSANamesResultCollector.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NameSpan.h"


#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>


MSANamesResultCollector::MSANamesResultCollector() { }

void MSANamesResultCollector::loadDocTheory(DocTheory* theory) {
	_docTheory = theory;
}

std::wstring MSANamesResultCollector::produceOutputHelper() {
	std::wstringstream wss;
	wss << L"<MSANAMESXML>\n";

	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		const SentenceTheory *st = _docTheory->getSentenceTheory(i);
		NameTheory *nt = st->getNameTheory();
		for (int j = 0; j < nt->getNNameSpans(); j++) {
			NameSpan *ns = nt->getNameSpan(j);
			wss << L"<name type=\"" << ns->type.getName() << "\">" << OutputUtil::escapeXML(nt->getNameString(j)) << "</name>\n";	
		}
	}

	wss << L"</MSANAMESXML>\n";
	return wss.str();

}

void MSANamesResultCollector::produceOutput(const wchar_t *output_dir,
									   const wchar_t *doc_filename)
{
	// File name
	std::wstring output_file = std::wstring(output_dir) + LSERIF_PATH_SEP + std::wstring(doc_filename);
	if (!boost::algorithm::ends_with(output_file, L".txt")) {
		output_file += L".msa.names.xml";
	} else {
		output_file = boost::algorithm::replace_last_copy(output_file, ".txt", ".msa.names.xml");
	}

	// Open output stream
	UTF8OutputStream stream;
	stream.open(output_file.c_str());

	std::wstring results = produceOutputHelper();

	// Write out
	stream << results;

	stream.close();
}

void MSANamesResultCollector::produceOutput(std::wstring *results) { *results = produceOutputHelper(); }
