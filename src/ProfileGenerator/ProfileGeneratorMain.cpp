// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <ctime>

#include "boost/algorithm/string.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/foreach.hpp"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/UnicodeUtil.h"
#include "ProfileGenerator/PGActorInfo.h"
#include "ProfileGenerator/PGDatabaseManager.h"
#include "ProfileGenerator/ProfileGenerator.h"
#include "ProfileGenerator/ProfileConfidence.h"

#include "ProfileGenerator/EmploymentHypothesis.h"
#include "ProfileGenerator/DescriptionHypothesis.h"
#include "ProfileGenerator/NameHypothesis.h"
#include <boost/scoped_ptr.hpp>

#define FACT_XML_FILE "C:\\default.facts.xml"
#define EQUIV_NAMES_FILE "C:\\batch_equiv_names.list"


void printUsage() {
	SessionLogger::info("PG") << "USAGE:\n";
	SessionLogger::info("PG") << "   ./ProfileGenerator.exe <param file>\n";
	SessionLogger::info("PG") << "   ./ProfileGenerator.exe <param file> -n My Entity Name\n";
}

int main(int argc, char **argv) {
	try{

#if !defined(WIN32) && !defined(WIN64)
		std::cerr << "\n\n********WARNING********\n\n";
		std::cerr << "ProfileGenerator has not been ported to Linux. It can be run but currently\n";
		std::cerr << "will experience a segmentation fault. To compile a version that works,\n";
		std::cerr << "look for the statements with\n";
		std::cerr << "   \"Uncomment this statement to avoid segmentation fault in Linux Release mode\"\n";
		std::cerr << "and uncomment them. If this does not prevent the segmentation fault, consider\n";
		std::cerr << "adding additional print statements. This is a SHORT TERM solution but until we\n";
		std::cerr << "are funded to port this to Linux, it will remain this way, since the cause of\n";
		std::cerr << "the segmentation fault appears to be deeply buried (possibly in BOOST).\n\n";
		std::cerr << "********************************\n\n";
#endif

		
		std::cerr << "Start ProfileGenerator\n";
		std::cerr.flush();

		std::string commandLineEntityName = "";
		if (argc > 3) {
			std::string param(argv[2]);
			if (param.compare("-n") == 0) {
				for (int i = 3; i < argc; i++) {
					commandLineEntityName += std::string(argv[i]);
					if (i + 1 != argc)
						commandLineEntityName += " ";
				}
		 }
		}
		if (commandLineEntityName.size() == 0 && argc != 2) {
			printUsage();
			return -1;
		}

		ParamReader::readParamFile(argv[1]);
		FeatureModule::load();

		const std::string mode = ParamReader::getRequiredParam("mode");
		// mode = { make_profile | get_epoch_actor_idso }
		if (mode.compare("make_profile") != 0 && mode.compare("get_epoch_actor_ids") != 0) {
				SessionLogger::info("PG") << "Unrecognized mode parameter found: '" << mode << "', exiting." << std::endl;
				return -1;
		}
		// do_batch = true for multiples, false for a singleton
		const bool do_batch = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_batch", true);
		
		// force_update = true causes profile to be created; false skips if already up to date
		const bool forceUpdateFlag = ParamReader::getOptionalTrueFalseParamWithDefaultVal("force_update", false);

		bool allowFailure = ParamReader::getOptionalTrueFalseParamWithDefaultVal("allow_individual_profile_failure", false);
		
		bool verbose = ParamReader::getOptionalTrueFalseParamWithDefaultVal("verbose", false);

		if (commandLineEntityName.size() != 0 && (do_batch || mode.compare("make_profile") != 0)) {
			SessionLogger::info("PG") << "Command line name specification legal only in make_profile mode with do_batch false, exiting." << std::endl;
			return -1;
		}

		// Initialize the profile slot confidence model
		ProfileConfidence_ptr confidenceModel = boost::make_shared<ProfileConfidence>();

		// Initialize database interfaces		
		const std::string conn_str = "";

		PGDatabaseManager_ptr pgdm = PGDatabaseManager_ptr();
		pgdm = boost::make_shared<PGDatabaseManager>(ParamReader::getRequiredParam("pg_database_connection"));

		ProfileGenerator_ptr profGen = boost::make_shared<ProfileGenerator>(pgdm, confidenceModel);

		bool all_successful = true;

		if (mode.compare("get_epoch_actor_ids") == 0) {
			// In get_epoch_actor_ids mode, we collect the ids for this epoch and write
			// them into a simple UTF8 file for re-use and parallel splitting

			int epoch_id = ParamReader::getRequiredIntParam("epoch");
			int min_count = ParamReader::getOptionalIntParamWithDefaultValue("minimum_name_count", 0);
			std::string profile_type = ParamReader::getRequiredParam("generate_profile_type");

			std::set<int> actors = pgdm->getActorsInEpoch(epoch_id, min_count, profile_type);
			SessionLogger::info("PG") << "Found " << actors.size() << " out-dated or novel names in this epoch." << std::endl;

			std::set<int> requestedActors = pgdm->getRequestedActors();
			BOOST_FOREACH(int id, requestedActors) {
				actors.insert(id);
			}

			std::string epoch_fpath = ParamReader::getRequiredParam("epoch_actors_file");
			UTF8OutputStream epoch_actors_ostream;
			epoch_actors_ostream.open(epoch_fpath.c_str());
			if (epoch_actors_ostream.fail()) {
				throw UnexpectedInputException("GenerateProfiles get_epoch_actor_ids failed to open output file", epoch_fpath.c_str());
			}
			BOOST_FOREACH(int id, actors) {	
				epoch_actors_ostream << id << L"\n";
			}
			epoch_actors_ostream.close();
			SessionLogger::info("PG") << "Wrote ids file for this epoch " << epoch_fpath << std::endl;

		} else if (mode.compare("make_profile") == 0 ){
			/// we get to generate some Profiles, unless they are all up todate
			std::list<PGActorInfo_ptr> actorsToProfile;

			int parallel = 0;

			// These are not required, but better to fail now than during processing
			EmploymentHypothesis::loadJobTitles();
			NameHypothesis::loadGenderNameLists();
			DescriptionHypothesis::loadGenderDescLists();

			if (!do_batch) {
				// non-batch mode: we generate one profile, using the name given the parameter
				// entity_name, or on the command line (which takes precedence).
				// we are constrained by generate_profile_type in the param file as well

				std::wstring name;
				if (commandLineEntityName.size() != 0)
					name = UnicodeUtil::toUTF16StdString(commandLineEntityName);
				else {
					std::wstringstream nameStream;
					std::string nstr = ParamReader::getParam("entity_name");
					if (nstr == "") {
						SessionLogger::info("PG") << "ProfileGeneratorMain in non-batch needs a param or arg for entity_name" << std::endl;
						throw UnrecoverableException("ProfileGeneratorMain", "non-batch mode requires entity_name param");
					}
					nameStream << nstr.c_str();
					name = nameStream.str();
				}
		
				Profile::profile_type_t profile_type = Profile::getProfileTypeForString(ParamReader::getParam("generate_profile_type"));		
				PGActorInfo_ptr target_actor = pgdm->getBestActorForName(name, profile_type);
				
				if (target_actor == PGActorInfo_ptr()) {
					SessionLogger::info("PG") << "No actor by this name with this type; exiting." << std::endl;
					return 0;
				} else {
					actorsToProfile.push_back(target_actor);
				}
			} else {
				// Generate profiles for every actor that appears on the provided list
				int epoch_id = ParamReader::getRequiredIntParam("epoch");
				parallel = ParamReader::getRequiredIntParam("parallel");
				int total_parallel = ParamReader::getRequiredIntParam("total_parallel");

				std::string epoch_fpath = ParamReader::getRequiredParam("epoch_actor_ids_file");
				boost::scoped_ptr<UTF8InputStream> epoch_actor_ids_istream_scoped_ptr(UTF8InputStream::build());
				UTF8InputStream& epoch_actor_ids_istream(*epoch_actor_ids_istream_scoped_ptr);
				epoch_actor_ids_istream.open(epoch_fpath.c_str());
				if (epoch_actor_ids_istream.fail()) {
					throw UnexpectedInputException(
						"GenerateProfiles get_epoch_actor_ids failed to open input file", epoch_fpath.c_str());
				}else{
					SessionLogger::info("PG") << "GenerateProfiles building profiles for epoch " << epoch_id << std::endl;
				}

				int count = 0;
				std::wstring line;
				while (!epoch_actor_ids_istream.eof()) {
					epoch_actor_ids_istream.getLine(line);
					if (line == L"")
						break;
					int id;
					try {  
						id = boost::lexical_cast<int>( line );
					} catch ( boost::bad_lexical_cast const& ) {
						std::wstringstream errstr;
						errstr << L"Value in epoch actor ID batch file was not convertible to int: " << line << "\n";
						throw UnexpectedInputException("ProfileGeneratorMain", errstr);
					}

					if (count++ % total_parallel != parallel) { continue; }

					PGActorInfo_ptr actor_info = pgdm->getActorInfoForId(id);
					if (actor_info == PGActorInfo_ptr()) {
						if (verbose) {
							SessionLogger::info("PG") << "No such actor in DB; skipping";
						}
						continue;
					} else {
						actorsToProfile.push_back(actor_info);

					}
				}				
				epoch_actor_ids_istream.close();
			}

			int count = 0;
			for (std::list<PGActorInfo_ptr>::iterator iter = actorsToProfile.begin(); iter != actorsToProfile.end(); ++iter) {
				PGActorInfo_ptr targetActor = *iter;
				int id = targetActor->getActorId();

				if (verbose) {
					SessionLogger::info("PG") << "T" << parallel << " Generating profile " << count << " for " << targetActor->toPrintableString() << "... ";
				}
				count++;

				if (pgdm->isUpToDate(id)) {
					if (verbose) 
						SessionLogger::info("PG") << "Profile was already up-to-date.";
					if (forceUpdateFlag) {
						SessionLogger::info("PG") << "  Recreating profile. " << std::endl;
					} else continue;
				}

				time_t start = time(NULL);
				Profile_ptr prof;
				try { 
					prof = profGen->generateFullProfile(targetActor);
				} catch (UnrecoverableException & e) {
					all_successful = false;
					if (allowFailure) {
						SessionLogger::warn("PG") << "T" << parallel << " SKIPPING profile generation failure for " << targetActor->toPrintableString() << ": " << e.getMessage() << " from " << e.getSource() << std::endl;
						continue;
					} else throw;
				}

				time_t middle = time(NULL);
				time_t creation = middle - start;

				try { 
					profGen->uploadProfile(prof);
				} catch (UnrecoverableException & e) {					
					all_successful = false;
					if (allowFailure) {
						SessionLogger::warn("PG") << "T" << parallel << " SKIPPING profile generation failure for " << targetActor->toPrintableString() << ": " << e.getMessage() << " from " << e.getSource() << std::endl;
						continue;
					}
					else throw;
				}

				time_t end = time(NULL);
				time_t upload = end - start;
				
				if (verbose) {
					std::ostringstream ostr;
					ostr << "...done (creation: " << creation << "s, upload: " << upload << "s)." << std::endl;
					SessionLogger::info("PG") << ostr.str();
				}

			}
		} // end mode choices

		if (ParamReader::isParamTrue("profile_pg_database_queries"))
			SessionLogger::info("PG_PROFILE") << pgdm->getProfileResults();

		if (!all_successful) {
			SessionLogger::info("PG") << "ProfileGeneratorMain failed for at least one profile; returning status -1" << std::endl;		
			exit(-1);
		}

	}// end try wrapper
	catch (UnrecoverableException & e) {
		SessionLogger::info("PG") << "ProfileGeneratorMain caught UnrecoverableException '" << e.getMessage() << "' from '" << e.getSource() << "', hard failing." << std::endl;
		exit(-1);
	}
	catch( std::exception & e ){
		SessionLogger::info("PG") << "ProfileGeneratorMain caught possibly recoverable '" << e.what() << ", hard failing." << std::endl;
		exit(-1);
	}
	catch( ... ){
		SessionLogger::info("PG") << "ProfileGeneratorMain caught unknown exception, hard failing." << std::endl;
		exit(-1);
	}

	return 0;
}// end main
