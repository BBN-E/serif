// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <boost/filesystem.hpp>
#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/theories/Document.h"
#include "Generic/preprocessors/DocumentInputStream.h"
#include "Generic/preprocessors/EntityTypes.h"
#include "Generic/preprocessors/ResultsCollector.h"
#include "Generic/common/FeatureModule.h"
#include "DocumentReader.h"
#include "DocumentWriter.h"

using namespace DataPreprocessor;
using namespace std;

const wchar_t *context_name = L"Preprocess";

/**
 * Prints utility usage information to standard error.
 */
void print_usage()
{
	cerr << "usage: IdfTrainerPreprocessor config (-n entities | -d | -b entities) (-o output) input ..." << endl;
	cerr << "    config     name of Serif configuration file" << endl;
	cerr << "    entities   name of file containing entity names" << endl;
    cerr << "    output     name of path to direct output files to" << endl;
	cerr << "    input      name of input file to preprocess" <<  endl << endl;

	cerr << "options:" << endl;
	cerr << "    -n         use only the entity names from the given file" << endl;
	cerr << "    -d         use the default Serif entity names" << endl;
	cerr << "    -b         use both default entity names and names from the given file" << endl;
	cerr << "               (redefinition of entity names is not allowed)" << endl << endl;
    cerr << "    -o         path for the output file" << endl << endl;
    
	cerr << "The IdfTrainerPreprocessor takes as input one or more SGML input files and" << endl;
	cerr << "outputs them in a sexp format compatible with the IdfTrainer." << endl << endl;

	cerr << "Each input file `input' will be output to `input.sexp'. If an output_path" << endl;
    cerr << "is specified, then output goes to `output_path/input.sexp'. Input files must" << endl;
	cerr << "contain one or more <DOCUMENT> or <DOC> tags. <ENAMEX> and <TIMEX> tags are" << endl;
	cerr << "recognized and their meta-data preserved and output in the sexp output files." << endl;
	cerr << "<SENT> tags are respected as sentence boundaries." << endl << endl;

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
void preprocess(const EntityTypes *entityTypes, char *input_file, char *output_path)
{
	ResultsCollector results;
	DocumentWriter writer(results);

	char output_file[_MAX_PATH];
    if (output_path == NULL)
      sprintf(output_file, "%s.sexp", input_file);

    else {
      boost::filesystem::path input_path(input_file);
      const char *filename = input_path.filename().c_str();
      sprintf(output_file, "%s/%s.sexp", output_path, filename);
    }

	cerr << "Reading input file `" << input_file << "'" << endl;
    cerr << "Printing to output file `" << output_file << "'" << endl;
	DocumentInputStream in(input_file);
	DocumentReader reader(in);

	Document *doc = NULL;
	int index = 0;

	// Read all the documents from the input file.
	int doc_number = 1;
	while ((doc = reader.readNextDocument()) != NULL) {
		cerr << "\rParsing document " << doc_number;
		// Once we've found a document, open the output file.
		results.open(output_file);
		writer.writeNextDocument(entityTypes, doc);
		delete doc;
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
    int nextarg;
    char* output_path = NULL;

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
		
		FeatureModule::load();

		// Set up the session logger.
		SessionProgram sessionProgram;
		SessionLogger::logger = _new FileSessionLogger(sessionProgram.getLogFile(), 1, &context_name);
	}
	catch (UnrecoverableException e) {
		cerr << "error: " << e.getMessage() << endl;
		return -1;
	}

	// Initialize the entity names.
	if (strcmp(argv[2], "-d") == 0) {
		cerr << "Initializing with SERIF entity names." << endl;
		entityTypes = _new EntityTypes();
		nextarg = 3;
	}
	else if (strcmp(argv[2], "-b") == 0) {
		cerr << "Initializing with SERIF entity names." << endl;
		cerr << "Adding extension entity names from " << argv[3] << endl;
		entityTypes = _new EntityTypes(argv[3], true);
		nextarg = 4;
	}
	else if (strcmp(argv[2], "-n") == 0) {
		cerr << "Initializing with entity names from " << argv[3] << endl;
		entityTypes = _new EntityTypes(argv[3], false);
		nextarg = 4;
	}
	else {
		print_usage();
		cerr << endl;
		cerr << "unrecognized option: " << argv[2] << endl;
		return -1;
	}

    if (strcmp(argv[nextarg], "-o") == 0) {
        output_path = argv[nextarg+1];
        first = nextarg+2;
      }
    else
      first = nextarg;

	// Preprocess each file.
	for (int i = first; i < argc; i++) {
		try {
          preprocess(entityTypes, argv[i], output_path);
		}
		catch (UnrecoverableException e) {
			cerr << "error: " << e.getMessage() << endl;
			return -1;
		}
	}

	delete entityTypes;

#ifdef ENABLE_LEAK_DETECTION
	ParamReader::finalize();
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	//_CrtDumpMemoryLeaks();
#endif

	return 0;
}
