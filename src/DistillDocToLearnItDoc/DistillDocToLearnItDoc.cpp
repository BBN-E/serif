#include "common/leak_detection.h"

#pragma warning(disable: 4996)

#include <vector>
#include "string.h"
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include "Generic/common/ParamReader.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"

#include "LearnIt/MainUtilities.h"
#include "LearnIt/util/FileUtilities.h"
#include "SeedIRDocConverter.h"
#include "PatternIRDocConverter.h"
#include "PropIRDocConverter.h"

#include "Generic/common/FeatureModule.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;

typedef pair<boost::regex, string> PathConversion;
typedef vector<PathConversion> PathConversions;

const path& ensurePathToFile(const path& filename) {
	if (!exists(filename)) {
		create_directories(filename.parent_path());
	}
	return filename;
}

string transformDocIDToFilename(const string& doc, const PathConversions& pathConversions) {
    static boost::regex wikipedia_regex("^wikipedia/(..)(.*)$");

	string ret = doc;

	BOOST_FOREACH (const PathConversion& pathConversion, pathConversions) {
		ret = regex_replace(ret, pathConversion.first, pathConversion.second);
	}

	// Wikipedia needs to be handled as a special case because it's weird
	if (ret.find("wikipedia")==0 && ret.length()<12) {
		SessionLogger::warn("LEARNIT") << "Wikipedia document name doesn't have at least two letters!" <<endl;
	}
	boost::smatch m;
	if (regex_match(ret, m, wikipedia_regex)) {
		string first_two_letters(m[2].first, m[2].second);;
		string rest(m[3].first, m[3].second);
		string folder_name(first_two_letters);

		erase_all(folder_name, "_");

		ret = "wikipedia/"+folder_name+"/"+first_two_letters+rest;
	}

	return ret;
}

PathConversions load_path_conversions() {
	PathConversions ret;

	const string path_conversion_file = ParamReader::getRequiredParam("path_conversions");
	

	if (!boost::filesystem::exists(path_conversion_file)) {
		wstringstream msg;
		msg << "Cannot open path conversion file " 
			<< UnicodeUtil::toUTF16StdString(path_conversion_file);
		throw UnexpectedInputException("load_path_conversions", msg);
	}

	ifstream inp(path_conversion_file.c_str());
	string line;
	while(getline(inp, line)) {
		vector<string> parts;
		split(parts, line, is_any_of("\t"));
		if (line.size() > 0) {
			if (parts.size() == 2) {
				ret.push_back(make_pair(boost::regex(parts[0]), parts[1]));
			} else {
				wstringstream msg;
				msg << L"Cannot understand path conversion line:\n" 
					<< UnicodeUtil::toUTF16StdString(line);
				throw UnexpectedInputException("load_path_conversions", msg);
			}
		}
	}

	return ret;
}

int main(int argc, char* argv[])
{	
	if (argc!=2) {
		cerr << "Expects a LearnIt parameter file.\n";
	}

	try {
		ParamReader::readParamFile(argv[1]);
		//REMOVE PARALLEL string parallel = argv[2];
        ConsoleSessionLogger logger(std::vector<wstring>(), L"[DD2LD]");
		SessionLogger::setGlobalLogger(&logger);
		// When the unsetter goes out of scope, due either to normal termination or an exception, it will unset the logger ptr for us.
		SessionLoggerUnsetter unsetter;

		FeatureModule::load();

		string input_corpus_list=ParamReader::getRequiredParam("learnitdoc_input_corpus_list");
		//REMOVE PARALLEL boost::replace_all(input_corpus_list, "+parallel+", parallel);
		SessionLogger::info("LEARNIT") << "Using corpus list " << input_corpus_list << endl;
		string prefix="";

		//prefix=ParamReader::getRequiredParam("learnitdoc_input_corpus_directory");	
		//SessionLogger::info("LEARNIT") << "Using corpus in " << prefix << " as input." << endl;

		PathConversions path_conversions = load_path_conversions();

		SessionLogger::info("LEARNIT") << "Loading files from the list in " << input_corpus_list; //<< " with prefix " << prefix << "\n";
		bool no_coref = ParamReader::getOptionalTrueFalseParamWithDefaultVal("no_coref", false);

		//Bilingual LearnIt, we want the option of loading multiple languages into a single LearnItDoc by sentence
		vector<string> variations=ParamReader::getStringVectorParam("learnitdoc_language_variations");	

		if (no_coref) {
			SessionLogger::info("LEARNIT") << "Not outputting coreference information" << endl;
		}

		string outFile=ParamReader::getRequiredParam("learnitdoc_output_file");
		//REMOVE PARALLEL boost::replace_all(outFile, "+parallel+", parallel);

		ensurePathToFile(outFile);
		UTF8OutputStream out(outFile.c_str());
		
		LearnItDocConverter converter(out);
		
		if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("prop_mode",
																false))
		{
			SessionLogger::info("LEARNIT") << "Running in prop mode" << endl;
			converter.addSentenceProcessor(
				make_shared<PropIRDocConverter>());
		} else {
			SessionLogger::info("LEARNIT") << "Running in normal mode" << endl;		
			converter.addSentenceProcessor(
				make_shared<PatternIRDocConverter>());
			converter.addSentenceProcessor( 
				make_shared<SeedIRDocConverter>());
		}

		// Read the list of documents to process.
		std::vector<DocIDName> w_doc_files = FileUtilities::readDocumentList(input_corpus_list);

		BOOST_FOREACH(DocIDName w_doc_file, w_doc_files) {
			try {
				if (!variations.empty()) {
					vector<const DocTheory*> docs;
					string doc_id;
					BOOST_FOREACH(string variation, variations) {
						string doc_filename = w_doc_file.second.c_str();
						boost::replace_all(doc_filename, "+variation+", variation);
						SerifXML::XMLSerializedDocTheory xml_doc(doc_filename.c_str());
						//MPS: I know this is scary, but I see no better way
						LanguageAttribute _language = LanguageAttribute::getFromString(
							xml_doc.getDocumentElement().getAttribute<std::wstring>(SerifXML::X_language).c_str());
						SerifVersion::setSerifLanguage(_language); //cross your fingers and hope it's loaded

						std::pair<Document*, DocTheory*> doc_pair = xml_doc.generateDocTheory();
						docs.push_back(doc_pair.second);
						doc_id = doc_pair.first->getName().to_debug_string();
					}
					if (!docs.empty()) {
						converter.process(docs, doc_id, variations);
					} else {
						cout << "Warning: DistillDocToLearnItDocConverter: got bad doc theory for " << w_doc_file.second << "\n";
					}
				} else {
					std::pair<Document*, DocTheory*> doc_pair = SerifXML::XMLSerializedDocTheory(w_doc_file.second.c_str()).generateDocTheory();
					if (doc_pair.second) {
						converter.process(doc_pair.second, doc_pair.first->getName().to_debug_string());
					} else {
						cout << "Warning: DistillDocToLearnItDocConverter: got bad doc theory for " << w_doc_file.second << "\n";
					}
				}
			} catch (const UnrecoverableException &) {
				SessionLogger::warn("LEARNIT") << "DistillDocToLearnItDocConverter: Unable to open " << w_doc_file.second << "\n";
			} 
		}
	} catch (const boost::exception& e) {
		cerr << diagnostic_information(e) << endl;
		return 1;
	} catch (UnexpectedInputException &e) {
        std::cerr << "\n" << e.getMessage() << std::endl;
        std::cerr << "Error Source: " << e.getSource() << std::endl;
        return 1;
	} catch (UnrecoverableException &e) {
        std::cerr << "\n" << e.getMessage() << std::endl;
        std::cerr << "Error Source: " << e.getSource() << std::endl;
        return 1;
    } catch (std::exception &e) {
		std::cerr << "Uncaught std::exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
		std::cerr << "Uncaught unidentifiable exception" << std::endl;
        return 1;
    } 

	return 0;
}

