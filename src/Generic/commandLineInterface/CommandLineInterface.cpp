// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/version.h"

#include "Generic/commandLineInterface/CommandLineInterface.h"

#include "Generic/common/cleanup_hooks.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/driver/QueueDriver.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/GenericTimer.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/parse/ParseResultCollector.h"
#include "Generic/apf/APFResultCollector.h"
#include "Generic/apf/APF4ResultCollector.h"
#include "Generic/eeml/EEMLResultCollector.h"
#include "Generic/icews/CAMEOXMLResultCollector.h"
#include "Generic/results/MSAResultCollector.h"
#include "Generic/results/MTResultCollector.h"
#include "Generic/results/SerifXMLResultCollector.h"
#include "Generic/edt/discmodel/featuretypes/WorldKnowledgeFT.h"
#include "Generic/descriptors/CompoundMentionFinder.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "dynamic_includes/common/ProfilingDefinition.h"
#include "dynamic_includes/common/BoostLibrariesFound.h"

#include "Generic/CASerif/driver/CADocumentDriver.h"
#include "Generic/CASerif/results/CAResultCollectorAPF.h"
#include "Generic/CASerif/results/CAResultCollectorAPF4.h"
#include "Generic/CASerif/results/CAResultCollectorEEML.h"

#include "English/timex/en_TemporalNormalizer.h" // For runTests()

#include <iostream>
#include <stdio.h>
#include <exception>
#include <fcntl.h>
#include <iomanip>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#ifdef _WIN32
#include <process.h>
#endif

// If the boost libraries are available, then we generate a much more
// friendly command-line user inferface.
#ifdef Boost_REGEX_FOUND
#ifdef Boost_PROGRAM_OPTIONS_FOUND
#define USE_BOOST_LIBS
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#endif
#endif


using namespace std;

//=======================================================================
// Forward-delcarations for file-local helper functions.
namespace {
	std::vector<ModifyCommandLineHook_ptr> &cliHooks();
	void initializeLeakDetection();
	void checkForLeaks();
	void displayVersionAndCopyright();
	bool parseCommandLine(int argc, const char **argv,
						  // output parameters:
						  string &parfile, map<string,string> &overrides,
						  vector<string> &unsets,
						  string &outdir, int &verbosity);
	void initializeParameters(const string& parfile,
							  const map<string,string> &overrides,
							  const vector<string> &unsets,
							  int verbosity);
	ResultCollector* createResultCollector(string outputFormat, bool use_correct_answers);
    std::vector<ResultCollector*>* createMultipleResultCollectors(bool use_correct_answers);

	int reportError(const char *what, const char *source=0);
	void checkHeap(const char *context);
	void printStages();
	void reportOpenedFiles();
	void restart(int argc, const char **argv);
	void quiet(const std::string &outdir);
	void runSerif(int verbosity, GenericTimer &totalLoadTimer);

	template<class C>
	class DevNullStreamBuff: public std::basic_streambuf<C> {
		typename std::char_traits<C>::int_type overflow(typename std::char_traits<C>::int_type c) { return c; }
	};
	DevNullStreamBuff<char> _CHAR_DEV_NULL;
	DevNullStreamBuff<wchar_t> _WCHAR_DEV_NULL;

	std::ofstream *cout_file = 0;
	std::ofstream *cerr_file = 0;
	std::wofstream *wcout_file = 0;
	std::wofstream *wcerr_file = 0;
}

void addModifyCommandLineHook(ModifyCommandLineHook_ptr hook) {
	cliHooks().push_back(hook);
}

//=======================================================================
/** Run the standard SERIF command-line interface.  This is typically 
  * called by the main() method of each language-specific binary (such
  * as Serif_English/EnglishSerif.cpp). */
int runSerifFromCommandLine(int argc, const char **argv) {
	// If leak detection is enabled, then initialize it.
	initializeLeakDetection();

	UTF8InputStream::startTrackingFileOpens();

	try {
		// Load the parameters and initialize the document driver.  This
		// will load all of Serif's models.  Use a timer to keep track
		// of how long we spend.
		GenericTimer totalLoadTimer;
		totalLoadTimer.startTimer();

		// Parse the command line arguments.
		string parfile;
		map<string,string> overrides;
		vector<string> unsets;
		string outdir;
		int verbosity;
		if (!parseCommandLine(argc, argv, parfile, overrides, unsets, outdir, verbosity))
			return -1;

		// Read the parameter file.
		initializeParameters(parfile, overrides, unsets, verbosity);

		// Set the open-file-retries count, if we have one.
		int open_file_retries = ParamReader::getOptionalIntParamWithDefaultValue("open_file_retries", 0);
		if (open_file_retries > 0)
			UTF8InputStream::setOpenFileRetries(open_file_retries);

		// Load modules specified in the parameter file.
		FeatureModule::load();

		// Be quiet, if requested
		if (verbosity < 0 || ParamReader::isParamTrue("run_subprocess_server"))
			quiet(outdir);

		// Let the user know what verison of serif they're using.
		displayVersionAndCopyright();

		if (!ParamReader::getOptionalTrueFalseParamWithDefaultVal("track_files_read", false))
			UTF8InputStream::stopTrackingFileOpens();

		// Run any command line hooks defined by feature modules.
		BOOST_FOREACH(ModifyCommandLineHook_ptr hook, cliHooks()) {
			switch (hook->run(verbosity)) {
			case ModifyCommandLineHook::CONTINUE: continue;
			case ModifyCommandLineHook::SUCCESS: return 0;
			case ModifyCommandLineHook::FAILURE: return -1;
			case ModifyCommandLineHook::RESTART: restart(argc, argv);
			}
		}

		// Run serif.
		if (ParamReader::isParamTrue("run_tests")) {
			if (runTests() != 0) return -1;
		} else if (ParamReader::hasParam("queue_driver")) {
			std::string driverType = ParamReader::getParam("queue_driver");
			boost::scoped_ptr<QueueDriver> queueDriver(QueueDriver::build(driverType));
			queueDriver->processQueue();
		} else {
			runSerif(verbosity, totalLoadTimer);
		}

		// Run any cleanup defined by feature modules
		BOOST_FOREACH(ModifyCommandLineHook_ptr hook, cliHooks()) {
			hook->cleanup(verbosity);
		}
	}
	catch (UnrecoverableException &e) {
		if (!e.isLogged())
			return reportError(e.getMessage(), e.getSource());
	}
	catch (exception &e) {
		return reportError(e.what());
	}
	catch (...) {
		return reportError("Unknown error");
	}

	// XMLPlatformUtils was initialized in XMLString.cpp
	// Cleanup here, so we don't pollute leak detection output with
	// the objects it creates.
	xercesc::XMLPlatformUtils::Terminate();

	// If leak detection is enabled, then report any leaked memory.
	checkForLeaks();

	reportOpenedFiles();

	checkHeap("main(); About to exit after successful run");

	#ifdef _DEBUG
		cerr << "Press enter to exit..." << endl;
		getchar();
	#endif
	return 0;
}

int runTests() {
	#ifdef NDEBUG
	std::cerr << "WARNING - Unit tests were not run because you have NDEBUG defined, which turns off asserts.\n";
	return -1;
	#else
	EnglishTemporalNormalizer::test();
	std::cerr << "All unit tests passed successfully.\n";
	return 0;
	#endif
}

//=======================================================================
// Helper Functions
//=======================================================================
namespace {

	std::vector<ModifyCommandLineHook_ptr> &cliHooks() {
		static std::vector<ModifyCommandLineHook_ptr> hooks;
		return hooks;
	}

	/** Initialize the leak detection system (if enabled). */
	void initializeLeakDetection() {
		#ifdef ENABLE_LEAK_DETECTION
		// Tell CRT to report memory leaks on exit:
		//_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
		//_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF );

		// Display memory leak information to stdout:
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

		// To track down a specific memory leak, check the allocation identifier that's printed
		// when the memory leak is dumped.  Then uncomment the following line, and set <NNNN> to
		// be the value of that identifier.
		//_crtBreakAlloc = <NNNN>;
		// In the watch window (for Visual Studio 9.0), you can edit this value using:
		//    {,,msvcr90d.dll}_crtBreakAlloc
		#endif
	}

	/** Check if any memory has been leaked since the leak detection 
	  * system was enabled.  If so, display a summary.*/
	void checkForLeaks() {
		#ifdef ENABLE_LEAK_DETECTION
		// Clean up static resources.
		ParamReader::finalize();
		SessionLexicon::destroy();
		Retokenizer::destroy();
		ObjectIDTable::finalize();
		runCleanupHooks();
		WordNet::deleteInstance();
		CompoundMentionFinder::deleteInstance();
		Symbol::defaultSymbolTable()->discardUnusedSymbols();
		Symbol::defaultSymbolTable()->freeAllDebugStrings();
		#endif
	}

	/** Display the current Serif version number, including a list
	  * of loaded feature modules; and a copyright message. */
	void displayVersionAndCopyright() {
		const char *os;
		#if defined(_WIN32)
			os = "Windows";
		#elif defined(UNIX)
			os = "UNIX";
		#else
			#error "Unknown operating system."
		#endif
			cout << "This is " << os << " " 
				 << SerifVersion::getProductName() << ", version " 
				 << SerifVersion::getVersionString() << ".\n"
				 << "  Build date: " << __DATE__ << " " << __TIME__ << "\n";
		std::vector<std::string> modules = FeatureModule::loadedFeatureModules();
		if (!modules.empty()) {
			cout << "  Using Modules:";
			BOOST_FOREACH(std::string name, modules) {
				cout << " " << name;
			}
			cout << "\n";
		}
		cout << SerifVersion::getCopyrightMessage() << endl;

	}

	void printModuleList() {
		std::vector<std::string> modules = FeatureModule::registeredFeatureModules();
		cout << "Statically-linked feature modules:\n";
		BOOST_FOREACH(std::string name, modules)
			cout << "  * " << name <<endl;
	}

	boost::regex OverrideParameterOption("(\\w+)=(.*)", boost::regex::perl);

	void usage(const char* progname, const boost::program_options::options_description &opts_spec) {
		boost::filesystem::path path(progname);
		cout << "Usage: " << path.filename()
			 << " [OPTIONS] PARFILE [INPUT_FILES...]\n\n"
			 << "    PARFILE: The " << SerifVersion::getProductName()
			 << " parameter file, specifying where models are located.\n"
			 << "    INPUT_FILES: An optional list of input files.  This will override the\n"
			 << "        \"batch\" parameter from the parameter file\n\n"
			 << opts_spec << endl;
	}

	bool parseCommandLine(int argc, const char **argv,
						  // output parameters:
						  string &parfile, 
						  map<string,string> &overrides,
						  vector<string> &unset_params,
						  string &outdir, int &verbosity) 
	{
		namespace opts = boost::program_options;
		opts::command_line_parser optparser(argc, const_cast<char **>(argv));
		opts::options_description arg_spec; // Argument specification.

		// Destination variables for arguments.
		vector<string> override_strings;
		vector<string> input_files;
		vector<string> modules;
		string http_server_port;
		string http_server_logdir;

		// Define the list of (non-positional) options we accept.
		opts::options_description options_arg_spec("Options");
		std::string productName = SerifVersion::getProductName();
		options_arg_spec.add_options()
			("help,h", "Describe the allowed options")
			("verbose,v", "Generate verbose output")
			("quiet,q", "Do not write messages to stdout/stderr")
			("outdir,o", opts::value(&outdir), 
			 "The output directory (aka the experiement_dir)")
			("stages", ("Print a list of "+productName+"'s stages").c_str())
			("param,p", opts::value(&override_strings), 
			 "Override a specific parameter: \"-p param=value\"")
			("unset,u", opts::value(&unset_params),
			 "Unset a specific parameter: \"-u param\"")
			("http-server", ("Run "+productName+" in HTTP server mode").c_str())
			("http-server-port", opts::value(&http_server_port),
			 "The port number that the HTTP server should use to listen "
			 "for incoming connections.")
			("http-server-logdir", opts::value(&http_server_logdir),
			 "If specified, then log all HTTP POST requests and their "
			 "responses to this directory.")
			("subprocess-server", ("Run "+productName+" in subprocess server mode.").c_str())
			("module", opts::value(&modules), "Use a feature module.")
			("list-modules", "List available static feature modules.")
			("run_tests", "Run unit tests.")
			("version,V", "Print version information")
			;
		arg_spec.add(options_arg_spec);
		// Map positional arguments to the hidden "__parfile" 
		// and "__input_files" options.
		opts::options_description positional_arg_spec("Positional Arguments");
		positional_arg_spec.add_options()
			("__parfile", opts::value(&parfile), "Parameter file");
		positional_arg_spec.add_options()
			("__input_files", opts::value(&input_files), "Input files");
		opts::positional_options_description positional_opt_spec;
		positional_opt_spec.add("__parfile", 1);
		positional_opt_spec.add("__input_files", -1);
		optparser.positional(positional_opt_spec);
		arg_spec.add(positional_arg_spec);

		// Read the options.
		optparser.options(arg_spec);
		opts::variables_map option_values;
		opts::store(optparser.run(), option_values);
		opts::notify(option_values);

		verbosity = (static_cast<int>(option_values.count("verbose")) -
					 static_cast<int>(option_values.count("quiet")));

		// Display the help message, if requested.
		if (option_values.count("help")) {
			displayVersionAndCopyright();
			usage(argv[0], options_arg_spec);
			return false;
		}

		// Display version, if requested
		if (option_values.count("version")) {
			displayVersionAndCopyright();
			return false;
		}

		// Get the dirname part of the serif executable (argv[0]).
		boost::filesystem::path cmdpath(argv[0]);
		std::string cmd_dir = cmdpath.parent_path().string();
		if (cmd_dir.empty()) cmd_dir += ".";

		// Command-line support for the http server.
		if ((option_values.find("http-server") != option_values.end()) ||
			(option_values.find("subprocess-server") != option_values.end()))
		{
			modules.push_back("SerifHTTPServer");

			if (option_values.find("http-server-port") != option_values.end()) {
				override_strings.push_back("http_server_port=" + 
					boost::lexical_cast<string>(http_server_port));
			}
			if (option_values.find("http-server-logdir") != option_values.end())
				override_strings.push_back("http_server_logdir=" + http_server_logdir);

			if (option_values.find("subprocess-server") != option_values.end()) {
				override_strings.push_back("run_subprocess_server=true");
				verbosity = -1;
			}
			if (option_values.find("http-server") != option_values.end())
				override_strings.push_back("run_http_server=true");
		}

		// Load any requested modules
		FeatureModule::addToPath(cmd_dir.c_str());
		if (!modules.empty()) {
			if (verbosity > 0)
				cout << "Loading feature modules..." << std::endl;
			BOOST_FOREACH(string module, modules) {
				if (verbosity > 0)
					std::cout << "  * " << module << std::endl;
				FeatureModule::load(module.c_str());
			}
		}

		// Display a stage list, if requested.
		if (option_values.count("stages")) {
			displayVersionAndCopyright();
			printStages();
			return false;
		}

		if (option_values.count("list-modules")) {
			printModuleList();
			return false;
		}

		// Require a parameter file.
		if (parfile.empty()) {
			displayVersionAndCopyright();
			usage(argv[0], options_arg_spec);
			cout << "Error: expected a parameter file" << endl;
			return false;
		}

		// Handle command-line options that translate into parameters.
		if (!outdir.empty())
			override_strings.push_back("experiment_dir="+outdir);
		if (!input_files.empty()) {
			override_strings.insert(override_strings.begin(), "batch_file=");
			override_strings.push_back("input_files="+boost::algorithm::join(input_files, ",")); 
		}
		if (option_values.find("run_tests") != option_values.end()) {
			override_strings.push_back("run_tests=true");
		}

		// Convert override strings into override map.
		for (size_t i=0; i<override_strings.size(); ++i) {
			boost::smatch match;
			if (boost::regex_search(override_strings[i], match, OverrideParameterOption)) {
				if (match.str(1) == string("experiment_dir") && !outdir.empty() && match.str(2) != outdir) {
					cout << "Error: Use \"--outdir\" or \"-p experiment_dir=path\", not both!" << endl;
					displayVersionAndCopyright();
					return false;
				}
				if (match.str(1) == string("batch_file") && !input_files.empty() && !match.str(2).empty()) {
					cout << "Warning: batch_file and input_files were both specified; "
						 << SerifVersion::getProductName() 
						 << " will process both." << endl;
				}
				overrides[match.str(1)] = match.str(2);
			} else {
				cout << "Badly formatted parameter override: \"" << override_strings[i] << "\"" << endl;
				cout << "Expected: -p param=value" << endl;
			}
		}

		return true;
	}
	
	void initializeParameters(const string& parfile,
							  const map<string,string> &overrides,
							  const vector<string> &unsets,
							  int verbosity) {
		// First, read the parameter file
		if (verbosity > 0)
			cout << "Reading parameter file " << parfile << "..." << endl;
		ParamReader::readParamFile(parfile, overrides);

		// Then override any parameters specified on the command-line.
		typedef std::pair<string,string> string_pair;
		BOOST_FOREACH(string_pair entry, overrides) {
			if (verbosity > 0)
				cout << "Overriding parameter " << entry.first << endl
					 << "  Old value: " << ParamReader::getParam(entry.first) << endl
					 << "  New value: " << entry.second << endl;
			ParamReader::setParam(entry.first.c_str(), entry.second.c_str());
		}

		BOOST_FOREACH(string unset, unsets) {
			if (verbosity > 0)
				cout << "Unsetting parameter " << unset << endl;
			ParamReader::unsetParam(unset.c_str());
		}
	}

	/** Print a list of Serif's stages */
	void printStages() {
		cout << SerifVersion::getProductName()
			 << " processing stages:" << endl;
		for (Stage s=Stage::getStartStage(); s<=Stage::getEndStage(); ++s)
			cout << setw(4) << (s.getSequenceNumber()+1) << ") " << s.getName() << endl;
	}

    /** Create and return a vector of new result collectors,
      one for each type listed as an alternate output format. */
    std::vector<ResultCollector*>* createMultipleResultCollectors(bool use_correct_answers) {
      std::vector<ResultCollector*> *resultCollectors = _new std::vector<ResultCollector*>;
      std::vector<std::string> outputFormats = ParamReader::getStringVectorParam("output_format");
      if (outputFormats.empty()) {
		  if (use_correct_answers) {
			  resultCollectors->push_back(_new APFResultCollector());
		  } else {
			  resultCollectors->push_back(_new CAResultCollectorAPF());
		  }
		  return resultCollectors;
      }
      else {
        std::string outputFormat;
        for (size_t i=0; i< outputFormats.size(); i++) {
          outputFormat = outputFormats[i];
          boost::trim(outputFormat);
          resultCollectors->push_back(createResultCollector(outputFormat, use_correct_answers));
        }
        return resultCollectors;
      }
    }

    ResultCollector* createResultCollector(std::string outputFormat,
										   bool use_correct_answers) {
		if (use_correct_answers) {
			if (boost::iequals(outputFormat, "EEML"))
				return _new CAResultCollectorEEML();
			else if (boost::iequals(outputFormat, "APF"))
				return _new CAResultCollectorAPF();
			else if (boost::iequals(outputFormat, "APF4"))
				return _new CAResultCollectorAPF4(CAResultCollectorAPF4::APF2004);
			else if (boost::iequals(outputFormat, "APF5"))
				return _new CAResultCollectorAPF4(CAResultCollectorAPF4::APF2005);
			else if (boost::iequals(outputFormat, "APF7"))
				return _new CAResultCollectorAPF4(CAResultCollectorAPF4::APF2007);
			else if (boost::iequals(outputFormat, "APF8"))
				return _new CAResultCollectorAPF4(CAResultCollectorAPF4::APF2008);
			else if (boost::iequals(outputFormat, "SERIFXML"))
				return _new SerifXMLResultCollector();
			else
				throw UnexpectedInputException("runSerifFromCommandLine()",
					"Parameter 'output_format' must be set to 'APF', 'APF4', 'APF5', 'APF7', 'APF8', 'SERIFXML' or 'EEML'");
		} else {
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
			else if (boost::iequals(outputFormat, "SERIFXML"))
				return _new SerifXMLResultCollector();
			else if (boost::iequals(outputFormat, "CAMEOXML"))
				return _new CAMEOXMLResultCollector();
			else if (boost::iequals(outputFormat, "MSAXML"))
				return _new MSAResultCollector();
			else
				throw UnexpectedInputException("runSerifFromCommandLine()",
					"Parameter 'output_format' must be set to 'APF', 'APF4', 'APF5', 'APF7', 'APF8', 'EEML', 'PARSE', 'SERIFXML', 'CAMEOXML', 'MSAXML', or 'MT'");
		}
	}
}

	/** Display a message indicating how much time we spent loading models
	  * and processing documents. */
	void reportProfilingInformation(const GenericTimer &totalLoadTimer, 
									const GenericTimer &totalProcessTimer,
									const DocumentDriver &documentDriver
									) {
		std::ostringstream ostr;
		ostr << "Load time: " << (int) (totalLoadTimer.getTime() / 1000) << " seconds\n";
		ostr << "Load\t" << totalLoadTimer.getTime() << " msec" << endl;
		ostr << "Process\t" << totalProcessTimer.getTime() << " msec" << endl;
		ostr << "Load(exc.score)\t" << totalLoadTimer.getTime() - documentDriver.stageLoadTimer[Stage("score")].getTime() << " msec" << endl;
		ostr << "Process(exc.score)\t" << totalProcessTimer.getTime() - documentDriver.stageProcessTimer[Stage("score")].getTime() << " msec" << endl;
		ostr << endl;
		if (documentDriver.getSessionProgram())
			SessionLogger::updateContext(SESSION_CONTEXT, documentDriver.getSessionProgram()->getSessionName());
		SessionLogger::info("profiling") << ostr.str();
		documentDriver.logTrace();
	}

namespace {
	void checkHeap(const char *context) {
		#if defined(_WIN32)
			HeapChecker::checkHeap(context);
		#endif
	}

	/** Report an unhandled error, and return -1. */
	int reportError(const char *what, const char *source) {
		cerr << "\n" << what << endl;
		if (source)
			cerr << "Error Source: " << source << endl;
		checkHeap("main(); About to exit due to error");

		#if defined(_DEBUG) || defined(_UNOPTIMIZED)
			cerr << "Press enter to exit..." << endl;
			getchar();
		#endif
		return -1;
	}

	void reportOpenedFiles() {
		if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("track_files_read", false)) {
			std::string experiment_dir = ParamReader::getParam("experiment_dir");
			if (experiment_dir.empty()) return;
			UTF8OutputStream file_read_log((experiment_dir + SERIF_PATH_SEP + "files_read.log").c_str());
			const std::vector<std::string> &openedFiles = UTF8InputStream::getOpenedFiles();
			for (size_t i=0; i<openedFiles.size(); ++i) {
				file_read_log << openedFiles[i] << "\n";
			}
		}
	}

	void quiet(const std::string &outdir) {
		OutputUtil::makeDir(outdir.c_str());
		string stdoutFile(outdir+SERIF_PATH_SEP+"stdout.txt");
		string stderrFile(outdir+SERIF_PATH_SEP+"stderr.txt");
		string wstdoutFile(outdir+SERIF_PATH_SEP+"wstdout.txt");
		string wstderrFile(outdir+SERIF_PATH_SEP+"wstderr.txt");
		#ifdef _WIN32
			if (outdir.empty()) {
				std::cout.rdbuf(&_CHAR_DEV_NULL);
				std::cerr.rdbuf(&_CHAR_DEV_NULL);
				std::wcout.rdbuf(&_WCHAR_DEV_NULL);
				std::wcerr.rdbuf(&_WCHAR_DEV_NULL);
			} else {
				cout_file = new std::ofstream(stdoutFile.c_str());
				cerr_file = new std::ofstream(stderrFile.c_str());
				wcout_file = new std::wofstream(wstdoutFile.c_str());
				wcerr_file = new std::wofstream(wstderrFile.c_str());
				std::cout.rdbuf(cout_file->rdbuf());
				std::cerr.rdbuf(cerr_file->rdbuf());
				std::wcout.rdbuf(wcout_file->rdbuf());
				std::wcerr.rdbuf(wcerr_file->rdbuf());
			}
		#else
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			if (!outdir.empty()) {
				mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
				dup2(creat(stdoutFile.c_str(), mode), STDOUT_FILENO);
				dup2(creat(stderrFile.c_str(), mode), STDERR_FILENO);
			}
		#endif
	}

	void runSerif(int verbosity, GenericTimer &totalLoadTimer) {
		bool use_correct_answers = ParamReader::isParamTrue("use_correct_answers");

		SessionProgram sessionProgram;
        std::vector<ResultCollector*>* resultCollectors = createMultipleResultCollectors(use_correct_answers);
        ResultCollector* firstResultCollector = resultCollectors->at(0);

		boost::scoped_ptr<DocumentDriver> documentDriver;
		if (use_correct_answers) {
			documentDriver.reset(_new CADocumentDriver(&sessionProgram, firstResultCollector));
		} else {
			documentDriver.reset(_new DocumentDriver(&sessionProgram, firstResultCollector));
		}


        vector<ResultCollector*>* alternateCollectors = _new vector<ResultCollector*>;
        if (resultCollectors->size() > 1) {
          vector<ResultCollector*>::iterator vit = resultCollectors->begin();
          vit++;
          alternateCollectors->assign(vit, resultCollectors->end());
          documentDriver->addAlternateResultCollectors(alternateCollectors);
        }

		totalLoadTimer.stopTimer();

		// Call documentDriver.run() to process the documents specified
		// by the parameter file.  If we're checking memory use, then 
		// we may want to run twice (to check the memory delta).
		GenericTimer totalProcessTimer;
		totalProcessTimer.startTimer();
		documentDriver->run();
		if (ParamReader::isParamTrue("run_serif_twice"))
			documentDriver->run();
        for (size_t i=0; i<resultCollectors->size();i++)
          delete resultCollectors->at(i);
        delete resultCollectors;
        delete alternateCollectors;
		totalProcessTimer.stopTimer();

		// If SERIF_PROFILING is enabled, then let the user know how
		// long we spent loading models and processing the documents.
		// Governed by "profiling" SessionLogger keyword.
		reportProfilingInformation(totalLoadTimer, totalProcessTimer, *documentDriver);
	}

	void restart(int argc, const char **argv) {
		// Make a copy of argv with a terminating NULL.
		std::vector<const char*> args(argv, argv+argc);
		args.push_back(NULL);
		char** arglist = const_cast<char**>(&(*(args.begin())));

		// Use exec to restart the server.
		#ifdef _WIN32
		_execv(arglist[0], arglist);
		#else
		execv(arglist[0], arglist);
		#endif
	}
}
