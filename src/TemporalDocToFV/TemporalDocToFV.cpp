#include "Generic/common/leak_detection.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <set>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/archive/text_woarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/edt/CorefUtilities.h"
#include "learnit/LearnIt2Matcher.h"
#include "learnit/Target.h"
#include "learnit/db/LearnItDB.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/common/ElfMultiDoc.h"
#include "learnit/MainUtilities.h"
#include "learnit/ProgramOptionsUtils.h"
#include "learnit/util/FileUtilities.h"
#include "temporal/TemporalInstanceGenerator.h"
#include "temporal/ManualTemporalInstanceGenerator.h"
#include "temporal/TemporalInstanceSerialization.h"
#include "temporal/TemporalFeatureGenerator.h"
#include "temporal/FeatureMap.h"
#include "temporal/TemporalDB.h"
#include "temporal/TemporalTypeTable.h"

using std::string; using std::wstring;
using std::cerr;
using std::wofstream;
using std::endl;
using boost::filesystem::path;
using boost::make_shared;
using boost::archive::text_woarchive;

void process_command_line(int argc, char** argv, string& param_file,
		string& doclist_filename, string& temporal_db_file, string& fv_file,
		string& preview_strings_file)
{
	boost::program_options::options_description desc("Options");
	desc.add_options()
		("param-file,P", boost::program_options::value<string>(&param_file))
		("doc-list,D", boost::program_options::value<string>(&doclist_filename))
		("db-file,T", boost::program_options::value<string>(&temporal_db_file))
		("preview-file,P", boost::program_options::value<string>(&preview_strings_file))
		("fv-file,F", boost::program_options::value<string>(&fv_file));

	boost::program_options::positional_options_description pos;
	pos.add("param-file", 1).add("db-file", 1).add("doc-list", 1)
		.add("fv-file",1).add("preview-file", 1);

	boost::program_options::variables_map var_map;

	try {
		boost::program_options::store(
			boost::program_options::command_line_parser(argc,argv).
			options(desc).positional(pos).run(), var_map);
	} catch (exception& exc) {
		cerr << "Comamnd-line parsing exception: " << exc.what();
		exit(1);
	}

	boost::program_options::notify(var_map);

	validate_mandatory_unique_cmd_line_arg(desc, var_map, "param-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "doc-list");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "db-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "fv-file");
	validate_mandatory_unique_cmd_line_arg(desc, var_map, "preview-file");
}

void PredFinderishInitialization() {
	CorefUtilities::initializeWKLists();
	ElfMultiDoc::load_xdoc_maps(UnicodeUtil::toUTF16StdString(
		ParamReader::getRequiredParam("string_to_cluster_filename")));
}

int main(int argc, char** argv) {
#ifdef NDEBUG
	try {
#endif
		string param_file;
		string temporal_db_file;
		string doclist_filename;
		string feature_vectors_file;
		string preview_strings_file;
		
		process_command_line(argc, argv, param_file, doclist_filename
			,temporal_db_file, feature_vectors_file, preview_strings_file);
		ParamReader::readParamFile(param_file);
		ParamReader::logParams();
		ConsoleSessionLogger logger(std::vector<wstring>(), L"[TempD2FV]");
		SessionLogger::setGlobalLogger(&logger);
		SessionLoggerUnsetter unsetter;
		SessionLogger::info("read_params") << "Read parameters from " << param_file;
		
		vector<LearnIt2Matcher_ptr> matchers;
		
		string learnit2_dir_path = ParamReader::getRequiredParam("learnit2_dbs");
		if (!learnit2_dir_path.empty()) {
			std::set<path> db_paths = 
				FileUtilities::getLearnItDatabasePaths(learnit2_dir_path);
			BOOST_FOREACH(path p, db_paths) {
				SessionLogger::info("load_l2_db") << "Loading LearnIt2 database " 
					<< p.string();
				matchers.push_back(make_shared<LearnIt2Matcher>(
					make_shared<LearnItDB>(p.string())));
			}
		} else {
			throw UnexpectedInputException("TemporalDocToFV::main",
				"LearnIt2 database path cannot be empty");
		}

		if (!boost::filesystem::exists(temporal_db_file)) {
			SessionLogger::warn("create_db") << "Specified temporal db file "
				<< temporal_db_file << " does not exist; it will be created.";
		}
		TemporalDB_ptr db = make_shared<TemporalDB>(temporal_db_file);
		TemporalTypeTable_ptr typeTable = db->makeTypeTable();

		std::vector<DocIDName> w_doc_files =
			FileUtilities::readDocumentList(doclist_filename);

		TemporalInstances instances;
		TemporalInstanceGenerator_ptr instanceGenerator = 
			boost::make_shared<ManualTemporalInstanceGenerator>(typeTable);
		FeatureMap_ptr featureMap = db->createFeatureMap();
		TemporalFeatureVectorGenerator featureGenerator(featureMap);

		PredFinderishInitialization();

		std::vector<Symbol> eligibleRelations =
			ParamReader::getSymbolVectorParam("apply_temporal_module_to");

		wofstream fvs_stream(feature_vectors_file.c_str(), std::ios::binary);
		text_woarchive fvs(fvs_stream);
		wofstream previewStrings(preview_strings_file.c_str());

		/*for (unsigned int doc_index = 0; doc_index < w_doc_files.size(); ++doc_index) {
			DistillationDoc_ptr doc = FileUtilities::load_doc(doc_index, w_doc_files);

			Symbol docid = wstring(w_doc_files[doc_index].first.begin(),
									w_doc_files[doc_index].first.end());
			if (doc) {
				const DocTheory* dt = doc->getTheory();
				MentionToEntityMap_ptr mentionToEntityMap =
					MentionToEntityMapper::getMentionToEntityMap(dt);

				BOOST_FOREACH(LearnIt2Matcher_ptr matcher, matchers) {
					if (find(eligibleRelations.begin(), eligibleRelations.end(),
								matcher->target()->getELFOntologyType()) !=
							eligibleRelations.end())
					{
						for (int sent_index = 0; sent_index < dt->getNSentences(); ++sent_index) {
							const SentenceTheory* sent_theory = dt->getSentenceTheory(sent_index);
							MatchInfo::PatternMatches matches =
								matcher->match(dt, docid, sent_theory, mentionToEntityMap);
							BOOST_FOREACH(MatchInfo::PatternMatch match, matches) {
								ElfRelation_ptr rel = make_shared<ElfRelation>(dt,
										sent_theory, matcher->target(), match, 
										match.score, true);
								instances.clear();
								instanceGenerator->instances(docid, sent_theory, rel,
										instances);

								BOOST_FOREACH(const TemporalInstance_ptr inst, instances) {
									featureGenerator.observe(*inst, sent_theory, dt);
								}
							}
						}
					}
				}
			}
		}

		featureGenerator.finishObservations();*/

		int totalInstances = 0;
		BOOST_FOREACH(DocIDName w_doc_file, w_doc_files) {
			// Load the SerifXML and get the DocTheory
			std::pair<Document*, DocTheory*> doc_pair = SerifXML::XMLSerializedDocTheory(w_doc_file.second.c_str()).generateDocTheory();

			const DocTheory* dt = doc_pair.second;
			MentionToEntityMap_ptr mentionToEntityMap =
				MentionToEntityMapper::getMentionToEntityMap(dt);
			int instCount = 0;

			BOOST_FOREACH(LearnIt2Matcher_ptr matcher, matchers) {
				if (find(eligibleRelations.begin(), eligibleRelations.end(),
							matcher->target()->getELFOntologyType()) !=
						eligibleRelations.end()) 
				{
					for (int sent_index = 0; sent_index < dt->getNSentences(); ++sent_index) {
						const SentenceTheory* sent_theory = dt->getSentenceTheory(sent_index);
						MatchInfo::PatternMatches matches =
							matcher->match(dt, doc_pair.first->getName(), sent_theory, mentionToEntityMap);
						BOOST_FOREACH(MatchInfo::PatternMatch match, matches) {
							ElfRelation_ptr rel = make_shared<ElfRelation>(
									dt, sent_theory, matcher->target(), match, 
									match.score, true);
							instances.clear();
							instanceGenerator->instances(doc_pair.first->getName(), sent_theory, rel,
									/*match.provenance,*/ instances);
							instCount += instances.size();

							BOOST_FOREACH(const TemporalInstance_ptr inst, instances) {
								// does nothing for now, just throws away results
								const TemporalFV fv = *featureGenerator.fv(dt, sent_index, *inst);
								const TemporalInstanceData tid = 
									*TemporalInstanceData::create(*inst, fv);
								fvs << tid;
								previewStrings << inst->previewString() << endl;
							}
						}
					}
				}
			}

			if (instCount > 0) {
				SessionLogger::info("instances_found_in_file") <<
					L"Generated " << instCount << " instances";
			}
			totalInstances += instCount;

			// Clean up
			delete doc_pair.second;
		}
		SessionLogger::info("total_instances_found") << L"Generated total of "
			<< totalInstances << " instances";

		db->syncFeatures(*featureMap);
#ifdef NDEBUG
	}  catch (std::exception &e) {
		std::cerr << "Uncaught std::exception: " << e.what() << std::endl;
        exit(-1);
    }
	catch (int n) {
		std::cerr << "Caught exception with error code: " << n << std::endl;
		exit(n);
	} catch (UnrecoverableException &e) {
		std::cerr << "\n" << e.getMessage() << std::endl;
		std::cerr << "Error Source: " << e.getSource() << std::endl;
		return -1;
	} catch (...) {
		std::cerr << "Uncaught unidentifiable exception" << std::endl;
        exit(-1);
    }
#endif
	return 0;
}
