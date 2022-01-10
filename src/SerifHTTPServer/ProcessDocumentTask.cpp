// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "SerifHTTPServer/ProcessDocumentTask.h"
#include "SerifHTTPServer/IncomingHTTPConnection.h"
#include "SerifHTTPServer/SerifWorkQueue.h"

#include "Generic/driver/DocumentDriver.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/driver/Stage.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/TimexUtils.h"

#include "Generic/parse/ParseResultCollector.h"
#include "Generic/apf/APFResultCollector.h"
#include "Generic/apf/APF4ResultCollector.h"
#include "Generic/eeml/EEMLResultCollector.h"
#include "Generic/results/MTResultCollector.h"
#include "Generic/results/SerifXMLResultCollector.h"
#include "Generic/icews/CAMEOXMLResultCollector.h"
#include "Generic/results/MSAResultCollector.h"
#include "Generic/common/version.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

using namespace SerifXML;
using namespace xercesc;

// Forward declarations for private helper functions
namespace {
	void checkLanguageAttribute(XMLTheoryElement docElem);
	void clearTheoryFromStage(DocTheory* docTheory, Stage stage);
	ResultCollector* createResultCollector(const std::string &outputFormat);
}

ProcessDocumentTask::ProcessDocumentTask(xercesc::DOMDocument *document, 
										 OptionMap *optionMap, const std::wstring &sessionId, 
										 bool user_supplied_session_id, boost::asio::io_service &ioService,
										 IncomingHTTPConnection_ptr connection)
: Task(ioService, connection), _document(document), _optionMap(optionMap), 
  _sessionId(sessionId), _user_supplied_session_id(user_supplied_session_id) {}

ProcessDocumentTask::~ProcessDocumentTask() {
	if (_document) _document->release();
	delete _optionMap;
}


bool ProcessDocumentTask::run(DocumentDriver *documentDriver) {
	bool success = false; // This will be our return value.
	std::pair<Document*, DocTheory*> docPair(0,0);
	ResultCollector *resultCollector = 0;

	try {
		// Create an XMLSerializedDocTheory from the document.  Note: this doesn't
		// actually deserialize a DocTheory from the XML yet -- that's done when we 
		// call generateDocTheory(), below.  The serializedDocTheory takes ownership
		// of the DOMDocument object (_document).
		XMLSerializedDocTheory serializedDocTheory(_document);
		_document = 0; // serializedDocTheory took over ownership.
		serializedDocTheory.setOptions(*_optionMap);

		// If the user supplied a session id, then don't delete the experiment 
		// directory -- they may want to send further queries about it.
		bool persistent_session = _user_supplied_session_id;

		std::wstring experimentDir;

		// This throws an exception if the language attribute is missing or
		// does not match the language we're compiled to handle:
		checkLanguageAttribute(serializedDocTheory.getDocumentElement());

		// Deserialize the document
		docPair = serializedDocTheory.generateDocTheory();
		Document *document = docPair.first;
		DocTheory* docTheory = docPair.second;

		// Create an initial session program.  NOTE: depending on the parameter
		// file, this may swap the NP Chunking stage with the Parsing stage.
		SessionProgram sessionProgram(false);

		// Use a separate subdirectory for each session.
		if (sessionProgram.hasExperimentDir()) {
			std::wstring rootExptDir(sessionProgram.getExperimentDir());
			experimentDir = rootExptDir+LSERIF_PATH_SEP+_sessionId;
			sessionProgram.setExperimentDir(experimentDir);
		}

		// Should we keep the experiment dir after we run?
		if (XMLString::compareIString(X_TRUE, (*_optionMap)[X_persistent_session].c_str()) == 0)
			persistent_session = true;

		// Get the start & end stages.
		Stage startStage, endStage;
		getStageRange(startStage, endStage, docTheory);

		std::string outputFormat = transcodeToStdString((*_optionMap)[X_output_format].c_str());

		std::wstring result_string; // Destination for result collector

		sessionProgram.setStageRange(startStage, endStage);
		resultCollector = createResultCollector(outputFormat);

		// docname is used to save the results to disk -- only bother with 
		// this for persistent sessions.
		const wchar_t* docname = 0;
		if (persistent_session) 
			document->getName().to_string();

		// Set document date if specified
		if (_optionMap->find(X_document_date) != _optionMap->end()) {
			std::wstring dateString = SerifXML::transcodeToStdWString((*_optionMap)[X_document_date].c_str());
			boost::optional<boost::gregorian::date> date;
			boost::optional<boost::posix_time::time_duration> timeOfDay;
			boost::optional<boost::local_time::posix_time_zone> timeZone;
			TimexUtils::parseDateTime(dateString, date, timeOfDay, timeZone);

			if (date) {
				boost::posix_time::ptime start(*date);
				document->setDocumentTimePeriod(start, boost::posix_time::seconds(86399)); // start of day, end of day
			} else {
				std::stringstream errStr; 
				errStr << "Could not parse document date: " << UnicodeUtil::toUTF8StdString(dateString);
				throw UnexpectedInputException("ProcessDocumentTask::run", errStr.str().c_str());
			}
		} 

		// This is where SERIF actually gets run.
		documentDriver->beginBatch(&sessionProgram, resultCollector, false);
		if (resultCollector)
			documentDriver->runOnDocTheory(docTheory, docname, &result_string);
		else
			documentDriver->runOnDocTheory(docTheory, docname);
		documentDriver->endBatch();

		// Generate our response.
		if (resultCollector) {
			sendResponse(OutputUtil::convertToUTF8BitString(result_string.c_str()));
		} else {
			// Serialize the output.  For now, we serialize from scratch, but later
			// it would be good to try to preserve the input document to the extent
			// possible (i.e., modify the input DOM, rather than generating a new one)
			XMLSerializedDocTheory result(docTheory, *_optionMap);
			sendResponse(result.adoptXercesXMLDoc());
		}
		success = true;

		if (!persistent_session  && !experimentDir.empty()) {
			try {
				boost::filesystem::remove_all(experimentDir.c_str());
			} catch (...) {
				std::cerr << "Warning: unable to delete " << experimentDir << std::endl;
			}
		}
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
	catch (const std::exception &exc) {
		reportError(500, exc.what()); // 500 = server error 
	}

	// Clean up
	if (documentDriver->inBatch()) 
		documentDriver->endBatch(); 
	delete docPair.first;  // the Document (or NULL)
	delete docPair.second; // the DocTheory (or NULL)
	delete resultCollector; // Might be NULL.

	Symbol::discardUnusedSymbols();
	return success;
}

void ProcessDocumentTask::getStageRange(Stage& startStage, Stage& endStage, DocTheory *docTheory) {
	// Start stage.
	std::string startStageName = transcodeToStdString((*_optionMap)[X_start_stage].c_str());
	Stage maxStartStage = docTheory->getMaxStartStage();
	if (startStageName.empty())
		startStage = maxStartStage;
	else
		startStage = Stage(startStageName.c_str());

	// End stage.
	std::string endStageName = transcodeToStdString((*_optionMap)[X_end_stage].c_str());
	if (endStageName.empty())
		endStage = Stage("output");
	else
		endStage = Stage(endStageName.c_str());

	// Sanity checks
	if (startStage > endStage) {
		std::ostringstream err;
		err << "Requested start stage (" << startStage.getName()
			<< ") comes after requested end stage (" << endStage.getName() << ")";
		throw UnexpectedInputException("ProcessDocumentTask::getStageRange", err.str().c_str());
	}
	if (startStage > maxStartStage) {
		std::ostringstream err;
		err << "Requested start stage (" << startStage.getName() 
			<< ") requires that the following stages be run first:";
		for (Stage s=maxStartStage; s<startStage; ++s)
			err << " " << s.getName();
		throw UnexpectedInputException("ProcessDocumentTask::getStageRange", err.str().c_str());
	}
}


namespace {
	// #if defined(ENGLISH_LANGUAGE)
	// #define X_LANGUAGE_NAME X_English
	// #elif defined(ARABIC_LANGUAGE)
	// #define X_LANGUAGE_NAME X_Arabic
	// #elif defined(CHINESE_LANGUAGE)
	// #define X_LANGUAGE_NAME X_Chinese
	// #elif defined(UNSPEC_LANGUAGE)
	// #define X_LANGUAGE_NAME X_Unspecified
// #elif defined(FARSI_LANGUAGE)
  // #define X_LANGUAGE_NAME X_Farsi
	// #else
	// #error "Add a case for your langauge here!" 
	// #endif

	// Check to make sure the language attribute matches our configuration.
	void checkLanguageAttribute(XMLTheoryElement docElem) {
		using namespace SerifXML;
		xstring language_lib = transcodeToXString(SerifVersion::getSerifLanguage().toString());
		const XMLCh* language = docElem.getAttribute<const XMLCh*>(X_language, 0);
		if (!language)
			throw UnexpectedInputException("checkLanguageAttribute", 
				"The \"language\" attribute is required in the input document"); 
		if (XMLString::compareIString(language, language_lib.c_str()) != 0)
			throw UnexpectedInputException("checkLanguageAttribute", 
				("This server only supports language=\"" +  
				 transcodeToStdString(language_lib.c_str())+"\"").c_str());
	}

	// Erase all theory information that is generated at or after the given stage.
	// (under development...)
	void clearDataFromStage(DocTheory* docTheory, Stage stage) {
		/*if (stage <= Stage("sent-break"))
			docTheory->setSentences(0);
		if (stage <= Stage("tokens")*/
	}


	ResultCollector* createResultCollector(const std::string &outputFormat_) {
		std::string outputFormat = outputFormat_;
		if (outputFormat.empty()) // default: specified in parameter file.
			outputFormat = ParamReader::getParam("output_format", "SERIFXML");

		if (boost::iequals(outputFormat, "EEML"))
			return _new EEMLResultCollector();
		else if (boost::iequals(outputFormat, "APF"))
			return _new APFResultCollector();
		else if (boost::iequals(outputFormat, "APF4"))
			return _new APF4ResultCollector(APF4ResultCollector::APF2004);
		else if (boost::iequals(outputFormat, "APF5"))
			return _new APF4ResultCollector(APF4ResultCollector::APF2005);
		else if (boost::iequals(outputFormat, "APF7"))
			return _new APF4ResultCollector(APF4ResultCollector::APF2007);
		else if (boost::iequals(outputFormat, "APF8"))
			return _new APF4ResultCollector(APF4ResultCollector::APF2008);
		else if (boost::iequals(outputFormat, "PARSE"))
			return _new ParseResultCollector();
		else if (boost::iequals(outputFormat, "MT"))
			return _new MTResultCollector();
		else if (boost::iequals(outputFormat, "CAMEOXML"))
			return _new CAMEOXMLResultCollector();
		else if (boost::iequals(outputFormat, "MSAXML"))
			return _new MSAResultCollector();
		else if (boost::iequals(outputFormat, "SERIFXML"))
			return 0;
		else
			throw UnexpectedInputException("runSerifFromCommandLine()",
				"Invalid output_format: must be APF, APF4, APF5, APF7, APF8, EEML, PARSE, SERIFXML, CAMEOXML, MSAXML or MT");
	}
}
