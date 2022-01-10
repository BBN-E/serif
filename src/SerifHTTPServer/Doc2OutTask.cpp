// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "SerifHTTPServer/Doc2OutTask.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/apf/APF4ResultCollector.h"
#include "Generic/results/SerifXMLResultCollector.h"
#include "Generic/state/XMLStrings.h"

bool Doc2OutTask::run(DocumentDriver *documentDriver) {
	ResultCollector *resultCollector = 0;

	// utf8 -> wchar_t.  There are better ways to do this, but this works for now.
	std::wstring document = UnicodeUtil::toUTF16StdString(_document);

	try {
		documentDriver->giveDocumentReader(DocumentReader::build("sgm"));

		// Choose what output format to use.
		switch(_outputFormat) {
			case APF:
				 resultCollector = _new APF4ResultCollector(APF4ResultCollector::APF2008);
				 break;
			case SERIFXML:
				 resultCollector = _new SerifXMLResultCollector();
				 break;
		}				 

		// Create an initial session program.  NOTE: depending on the parameter
		// file, this may swap the NP Chunking stage with the Parsing stage.
		SessionProgram sessionProgram;
		sessionProgram.setStageRange(Stage::getStartStage(), Stage("output"));

		// Run serif on the given document.
		std::wstring results;
		documentDriver->beginBatch(&sessionProgram, resultCollector);
		documentDriver->runOnString(document.c_str(), &results);
		documentDriver->endBatch();

		// wchar_t -> utf8.  There are better ways to do this, but this works for now.
		std::string results_utf8 = UnicodeUtil::toUTF8StdString(results);
		sendResponse(results_utf8);

		// Restore the default document reader.
		documentDriver->giveDocumentReader(DocumentReader::build());

		delete resultCollector;
		return true; // = success!
	}
	catch (const UnexpectedInputException &exc) {
		reportError(400, exc.getMessage()); // 400 = client error
	}
	catch (const InternalInconsistencyException &exc) {
		reportError(500, exc.getMessage()); // 500 = server error
	}
	catch (const UnrecoverableException &exc) {
		reportError(500, exc.getMessage()); // 500 = server error 
	}

	// Clean up after ourselves.
	documentDriver->endBatch();
	documentDriver->giveDocumentReader(DocumentReader::build());
	delete resultCollector;

	// Report the fact that we encountered an error.
	return false;
}
