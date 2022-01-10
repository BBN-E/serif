// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/theories/Document.h"
#include "Generic/preprocessors/DocumentInputStream.h"
#include "Generic/preprocessors/EntityTypes.h"
#include "Generic/preprocessors/ResultsCollector.h"
#include "Generic/trainers/EnamexDocumentReader.h"
#include "Generic/linuxPort/serif_port.h"
#include "AnnotatedParsePreprocessor/DocumentProcessor.h"

using namespace DataPreprocessor;
using namespace std;

const wchar_t *context_name = L"Preprocess";

// session logging stuff
const wchar_t *APP_CONTEXT_NAMES[] = {L"Session",
								  L"Document",
								  L"Sentence",
								  L"Stage"};
const int SESSION_CONTEXT = 0;
const int DOCUMENT_CONTEXT = 1;
const int SENTENCE_CONTEXT = 2;
const int STAGE_CONTEXT = 3;
const int N_CONTEXTS = 4;

/**
 * Prints utility usage information to standard error.
 */
void print_usage()
{
	cerr << "usage: AnnotatedParsePreprocessor config (-n entities | -d | -b entities) input ..." << endl;
	cerr << "    config     name of Serif configuration file" << endl;
	cerr << "    entities   name of file containing entity names" << endl;
	cerr << "    input      name of input file to preprocess" << endl << endl;

	cerr << "options:" << endl;
	cerr << "    -n         use only the entity names from the given file" << endl;
	cerr << "    -d         use the default Serif entity names" << endl;
	cerr << "    -b         use both default entity names and names from the given file" << endl;
	cerr << "               (redefinition of entity names is not allowed)" << endl << endl;

	cerr << "The AnnotatedParsePreprocessor takes as input one or more SGML input files and" << endl;
	cerr << "outputs them in a sexp format compatible with the DescriptorClassifierTrainer," << endl;
	cerr << "the NameLinkerTrainer and the PronounLinkerTrainer. " << endl << endl;

	cerr << "Each input file `input' will be output to `input.sexp'. Input files must" << endl;
	cerr << "contain one or more <DOCUMENT> or <DOC> tags. <ENAMEX>, <TIMEX>, <NUMEX>," << endl;
	cerr << "<PRONOUN>, <HEAD>, and <LIST> tags are recognized and their meta-data" << endl;
	cerr << "preserved and output in the sexp output files. <SENT> tags are respected" << endl;
	cerr << "as sentence boundaries." << endl;
	cerr << "Note: Word and phrase annotation tags should not contain inital or final spaces." << endl << endl;

	cerr << "Entity names files have the following format:" << endl << endl;

	cerr << "    n" << endl;
	cerr << "    NAME(1)[=REPLACE(1)]" << endl;
	cerr << "    ." << endl;
	cerr << "    ." << endl;
	cerr << "    NAME(n)[=REPLACE(n)]" << endl << endl;

	cerr << "where `n' is the number of entity names in the file, NAME(k) is the kth" << endl;
	cerr << "recognized entity name, and REPLACE(k) is the optional replacement text for its" << endl;
	cerr << "respective entity name." << endl;
}

/**
 * Preprocesses the file at the given location.
 *
 * @param entityTypes the recognized entity types.
 * @param input_file the name of the file to preprocess.
 */
void preprocess(DocumentProcessor& processor, char *input_file)
{
	cerr << "Reading input file `" << input_file << "'" << endl;
	*SessionLogger::logger << "Reading input file '" << input_file << "'\n";

	DocumentInputStream in(input_file);
	EnamexDocumentReader reader(in);

	char output_file[_MAX_PATH];
	sprintf(output_file, "%s.sexp", input_file);
	ResultsCollector results;
	results.open(output_file);
	processor.setResultsCollector(results);

	Document *doc = NULL;
	int index = 0;

	// Read all the documents from the input file.
	int doc_number = 1;
	while ((doc = reader.readNextDocument()) != NULL) {
		cerr << "\rProcessing document " << doc_number;
		SessionLogger::logger->updateContext(DOCUMENT_CONTEXT, doc->getName().to_string());
		processor.processNextDocument(doc);
		delete doc;
		SessionLogger::logger->updateContext(DOCUMENT_CONTEXT, "");
		doc_number++;
	}
	cerr << endl;
}

/**
 * Program entry point.
 *
 * @param argc the number of command-line arguments.
 * @param argv the command-line arguments.
 */
int main(int argc, char* argv[])
{
	EntityTypes *entityTypes;
	int first;

	cerr << endl;

	// If they didn't supply enough arguments, print the usage information.
	int minusDee = (argc >= 3) ? strcmp(argv[2], "-d") : -1;
	if (((minusDee == 0) && (argc < 4)) ||
		((minusDee != 0) && (argc < 5))) {
		print_usage();
		return -1;
	}

	try {
		// Read the configuration file.
		ParamReader::readParamFile(argv[1]);

		// Set up the session logger.
		SessionProgram sessionProgram;
		SessionLogger::logger = _new FileSessionLogger(sessionProgram.getLogFile(), N_CONTEXTS, APP_CONTEXT_NAMES);
		SessionLogger::logger->updateContext(SESSION_CONTEXT, sessionProgram.getSessionName());
		SessionLogger::logger->beginMessage();
		*SessionLogger::logger << "_________________________________________________\n"
							   << "Starting session: \""
					           << sessionProgram.getSessionName() << "\"\n"
							   << "Parameters:\n";
	
		ParamReader::logParams();
		*SessionLogger::logger << "\n";
	}
	catch (UnrecoverableException e) {
		cerr << "error: " << e.getMessage() << endl;
		return -1;
	}

	// Initialize the entity names.
	if (strcmp(argv[2], "-d") == 0) {
		cerr << "Initializing with SERIF entity names." << endl;
		*SessionLogger::logger << "Initializing with SERIF entity names.\n\n";
		entityTypes = _new EntityTypes();
		first = 3;
	}
	else if (strcmp(argv[2], "-b") == 0) {
		cerr << "Initializing with SERIF entity names." << endl;
		cerr << "Adding extension entity names from " << argv[3] << endl;
		*SessionLogger::logger << "Initializing with SERIF entity names.\n"
							   << "Adding extension entity names from " << argv[3] << ".\n\n";
		entityTypes = _new EntityTypes(argv[3], true);
		first = 4;
	}
	else if (strcmp(argv[2], "-n") == 0) {
		cerr << "Initializing with entity names from " << argv[3] << endl;
		*SessionLogger::logger << "Initializing with entity names from " << argv[3] << ".\n\n";
		entityTypes = _new EntityTypes(argv[3], false);
		first = 4;
	}
	else {
		print_usage();
		cerr << endl;
		cerr << "unrecognized option: " << argv[2] << endl;
		return -1;
	}

	try {
		DocumentProcessor processor(entityTypes);
	
		// Preprocess each file.
		for (int i = first; i < argc; i++) 
			preprocess(processor, argv[i]);

		cerr << endl;
		SessionLogger::logger->displaySummary();
		cerr << endl;

		delete entityTypes;
		delete SessionLogger::logger;
	}
	catch (UnrecoverableException e) {
		cerr << "\n" << e.getMessage() << "\n";
		return -1;
	}
	

#ifdef ENABLE_LEAK_DETECTION
	ParamReader::finalize();
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
