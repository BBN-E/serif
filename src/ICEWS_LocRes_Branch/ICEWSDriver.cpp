// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.
//
// Serif HTTP Server command-line interface

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/ParamReader.h"
#include "Generic/common/OutputUtil.h"
#include "ICEWS/ICEWSDriver.h"
#include "ICEWS/Identifiers.h"
#include "ICEWS/Stories.h"
#include "ICEWS/EventMention.h"
#include "ICEWS/EventMentionSet.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/results/SerifXMLResultCollector.h"
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

namespace ICEWS {

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
	SerifXMLResultCollector resultCollector;

	SessionProgram preIcewsSessionProgram(false);
	preIcewsSessionProgram.setStageRange(Stage("START"), Stage("icews-actors").getPrevStage());

	SessionProgram icewsSessionProgram(false);
	icewsSessionProgram.setStageRange(Stage("icews-actors"), Stage(ParamReader::getRequiredParam("end_stage").c_str()));

	if (icewsSessionProgram.hasExperimentDir()) {
		wstring experimentDir(icewsSessionProgram.getExperimentDir());
		size_t index = experimentDir.find_last_of(LSERIF_PATH_SEP);
		OutputUtil::makeDir(experimentDir.substr(0, index).c_str());
		OutputUtil::makeDir(experimentDir.c_str());
		_sessionLogger = _new FileSessionLogger(icewsSessionProgram.getLogFile(), N_CONTEXTS, CONTEXT_NAMES);
		SessionLogger::setGlobalLogger(_sessionLogger);
	}
	SessionLogger::info("ICEWS") << "_________________________________________________\n";
	SessionLogger::info("ICEWS") << "Starting session: \"" << icewsSessionProgram.getSessionName() << "\"";
	SessionLogger::info("ICEWS") << "Parameters:";
	ParamReader::logParams();

	Stories::StoryIDRange stories = Stories::getStoryIds(); 
	BOOST_FOREACH(StoryId storyId, stories) {
		std::cerr << "Processing story " << storyId.getId() << std::endl;
		boost::scoped_ptr<Document> document;
		boost::scoped_ptr<DocTheory> docTheory;

		// Check if the serifxml for this story is cached.
		std::pair<Document*, DocTheory*> docPair = Stories::loadCachedSerifxmlForStory(storyId);
		if (docPair.first != 0) {
			document.reset(docPair.first);
			docTheory.reset(docPair.second);
			if (documentShouldBeSkipped(document.get())) continue;
		}
		// Otherwise, use the pre-icews doc driver to process it.
		else {
			document.reset(Stories::readDocument(storyId));
			if (documentShouldBeSkipped(document.get())) continue;
			docTheory.reset(_new DocTheory(document.get()));
			_docDriver->beginBatch(&preIcewsSessionProgram, 0, false);
			_docDriver->runOnDocTheory(docTheory.get());
			_docDriver->endBatch();
			Stories::saveCachedSerifxmlForStory(storyId, docTheory.get());
		}

		// Now use the icews doc driver to run the icews stages.
		_docDriver->beginBatch(&icewsSessionProgram, 0, false);
		_docDriver->runOnDocTheory(docTheory.get());
		_docDriver->endBatch();

		// Save the results to a file whose name is based on the story id.
		if (_save_final_serifxml_in_experiment_dir) {
			std::wostringstream outfile;
			outfile << L"icews.stories." << storyId.getId() << L".xml";
			resultCollector.loadDocTheory(docTheory.get());
			resultCollector.produceOutput(icewsSessionProgram.getOutputDir(), 
				outfile.str().c_str());
		}
	}
	_docDriver->printTrace();

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


}
