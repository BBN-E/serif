// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.
//
// Serif HTTP Server command-line interface

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/icews/ICEWSDriver.h"
#include "Generic/icews/Stories.h"
#include "Generic/icews/ICEWSEventMention.h"
#include "Generic/icews/ICEWSEventMentionSet.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/results/SerifXMLResultCollector.h"
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

ICEWSDriver::ICEWSDriver(): 
		_save_final_serifxml_in_experiment_dir(false),
		_docDriver(_new DocumentDriver()),
		_sessionLogger(0)
{
	_save_final_serifxml_in_experiment_dir = ParamReader::isParamTrue("icews_save_final_serifxml");
	Symbol documentTextRegex(ParamReader::getParam(Symbol(L"icews_document_text_filter")));
	if (!documentTextRegex.is_null())
		_documentTextFilter.reset(_new boost::wregex(documentTextRegex.to_string()));
}

ICEWSDriver::~ICEWSDriver() {
	// Shut down the document driver, and *then* the session logger.
	_docDriver.reset();
	if (_sessionLogger) {
		SessionLogger::setGlobalLogger(0);
		delete _sessionLogger;
	}
}

void ICEWSDriver::processStories() {

	// SERIFXML is the only-and-always result collector for ICEWS
	SerifXMLResultCollector resultCollector;

	Stories::StoryIDRange stories = Stories::getStoryIds();

	bool run_icews_stages = true;

	Stage startStage = Stage(ParamReader::getRequiredParam("start_stage").c_str());
	Stage endStage = Stage(ParamReader::getRequiredParam("end_stage").c_str());

	if (startStage != Stage("START")) {
		throw UnrecoverableException("ICEWSDriver::processStories", "When processing files from the database, parameter start_stage must be START");
	}

	SessionProgram sessionProgram(false);
	sessionProgram.setStageRange(startStage, endStage);

	_docDriver->beginBatch(&sessionProgram, &resultCollector, false);
	
	SessionLogger::info("ICEWS") << "_________________________________________________\n";
	SessionLogger::info("ICEWS") << "Starting session: \"" << sessionProgram.getSessionName() << "\"";
	SessionLogger::info("ICEWS") << "Parameters:";
	ParamReader::logParams();

	std::vector<StoryId> storyIds;
	BOOST_FOREACH(StoryId storyId, stories) {
		storyIds.push_back(storyId);
	}

	SessionLogger::info("ICEWS") << "Processing " << storyIds.size() << " stories from the database\n";	

	BOOST_FOREACH(StoryId storyId, storyIds) {
		std::cerr << "Processing story " << storyId.getId() << std::endl;

		boost::scoped_ptr<Document> document;
		boost::scoped_ptr<DocTheory> docTheory;

		document.reset(Stories::readDocument(storyId));
		if (documentShouldBeSkipped(document.get())) continue;
		docTheory.reset(_new DocTheory(document.get()));

		std::wstringstream idStream;
		idStream << storyId.getId();		
		_docDriver->runOnDocTheory(docTheory.get(), idStream.str().c_str());
	}

	_docDriver->endBatch();
	_docDriver->logTrace();
}

bool ICEWSDriver::documentShouldBeSkipped(Document *document) {
	const wchar_t* docText = document->getOriginalText()->toString();
	if (_documentTextFilter && !boost::regex_search(docText, *_documentTextFilter)) {
		SessionLogger::info("ICEWS") << "Skipping document \"" << document->getName()
			<< "\" because its text does not match icews_document_text_filter.";
		return true;
	}
	return false;
}
