// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CAMEOXML_RESULT_COLLECTOR_H
#define CAMEOXML_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/common/OutputStream.h"
#include "Generic/theories/ActorMention.h"

class DocTheory;

class CAMEOXMLResultCollector : public ResultCollector {
public:
	CAMEOXMLResultCollector() {}
	virtual ~CAMEOXMLResultCollector() {}
	virtual void finalize() {}

	virtual void loadDocTheory(DocTheory* docTheory);
	
	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t* document_filename);
	virtual void produceOutput(std::wstring *results);

private:
	DocTheory *_docTheory;
	ActorInfo_ptr _actorInfo;
	std::wstring produceOutputHelper();
	void addActorElement(SerifXML::XMLElement& eventElem, ActorMention_ptr am, std::string role);
};

#endif
