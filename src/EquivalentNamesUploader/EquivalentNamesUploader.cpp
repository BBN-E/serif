#include "Generic/common/leak_detection.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <boost/make_shared.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "database/XDocRetriever.h"
#include "database/ResultSet.h"
#include "database/ValueVector.h"
#include "learnit/db/LearnItDB.h" 
#include "learnit/Seed.h"
#include "learnit/SlotConstraints.h"
#include "learnit/Target.h"
#include "learnit/SlotFiller.h"
#pragma warning(push, 0)
#include "boost/shared_ptr.hpp"
#include "boost/foreach.hpp"
#include "boost/thread/thread.hpp"
#include "boost/filesystem.hpp"
#include "boost/thread.hpp" 
#pragma warning(pop)

using boost::make_shared;

int main(int argc, char** argv) {
#ifdef NDEBUG
	try{
#endif
	if (argc != 4) {
		std::cerr << "Standard usage is EquivalentNamesUploader.exe par_file input_db output_db" << std::endl;
		exit(-1);
	}
	ParamReader::readParamFile(argv[1]);

	//replace nfs for Windows
	typedef std::pair<string, string> ParamPair;
	BOOST_FOREACH( ParamPair p, ParamReader::getAllParams() ) {
		size_t pos = p.second.find( "nfs" );
		if ( pos != string::npos ) {
		   string s = p.second;
		   s.replace( pos, 3, "" );
		   ParamReader::setParam(p.first.c_str(),s.c_str());
		}
	}


    ConsoleSessionLogger logger(std::vector<wstring>(), L"[EQNU]");
	SessionLogger::setGlobalLogger(&logger);
	// When the unsetter goes out of scope, due either to normal termination or an exception, it will unset the logger ptr for us.
	SessionLoggerUnsetter unsetter;

	if (strcmp(argv[2],argv[3])){
		// Copy our input database to our output database
		try {
			boost::filesystem::remove(argv[3]);
			boost::filesystem::copy_file(argv[2], argv[3]);
		} catch( std::exception e ){
			SessionLogger::err("file_copy_err") << "Failed to copy " 
				<< argv[2] << " to " << argv[3] << "\n" << e.what();
			exit(-1);
		} 
	}
	LearnItDB_ptr learnit_db = make_shared<LearnItDB>(std::string(argv[3]), false);
	
	char server[200];
	char database[200];
	char username[200];
	char password[200];
	ParamReader::getRequiredParam("db_server", server, 200 );
	ParamReader::getRequiredParam("xdoc_db_database", database, 200 );
	ParamReader::getRequiredParam("db_username", username, 200 );
	ParamReader::getRequiredParam("db_password", password, 200 );

	SessionLogger::info("LEARNIT") << "Initializing XDocRetriever" << "\n";
	XDocRetriever* xdocRetriever = NULL;
	//Try to initialize three times, with break between them before giving up.
	for( int i = 0; i < 3; ++i ){
		try{
			xdocRetriever = _new XDocRetriever(server, database, username, password);
		}catch(...){
			if( i < 2 ){
				SessionLogger::info("LEARNIT") << "Initialization failed! Waiting three seconds and trying again" << std::endl;
				boost::this_thread::sleep(boost::posix_time::seconds(3));
				continue;
			}else{
				SessionLogger::info("LEARNIT") << "Giving up!" << endl;
				throw;
			}
		}
		break;
	}
	SessionLogger::info("LEARNIT") << "Initialized XDocRetriever" << "\n";

	std::vector<Seed_ptr> seeds = learnit_db->getSeeds();
	BOOST_FOREACH(Seed_ptr seed, seeds){
		Target_ptr target = seed->getTarget();
		std::vector<SlotConstraints_ptr> all_slot_constraints = target->getAllSlotConstraints();
		int islotn = 0;
		BOOST_FOREACH(AbstractLearnItSlot_ptr slot, seed->getAllSlots()){
			const wstring& name = slot->name();
			SlotConstraints_ptr slot_constraints_n = all_slot_constraints.at(islotn++);
			if (!slot_constraints_n->slotIsMentionSlot()){
				continue;
			}
			if (learnit_db->containsEquivalentNamesForName(name)){
				continue;
			}

			ResultSet results;
			//TODO: second argument should be entity type.
			xdocRetriever->getY2EquivalentNames(name.c_str(), L"", &results);

			SessionLogger::info("LEARNIT") << "Name: " << name << "\n";
			for ( size_t i=0; i < results.getColumnValues(0)->getNValues(); i++) {
				std::wstring equivalent_name = results.getColumnValues(0)->getValue(i);
				std::wstring freq = results.getColumnValues(1)->getValue(i);
				std::wstring score = results.getColumnValues(2)->getValue(i);
				std::transform(equivalent_name.begin(), equivalent_name.end(), 
					equivalent_name.begin(), towlower);
				//SessionLogger::info("LEARNIT") << freq << "\n";
				//SessionLogger::info("LEARNIT") << "Column2: " << res2 << "\n";
				learnit_db->insertEquivalentNameRow(name, equivalent_name, score);
			}
		}
	}
	delete xdocRetriever;

#ifdef NDEBUG
	// If something broke, then say what it was.
	} catch (UnrecoverableException &e) {
		std::cerr << "\n" << e.getMessage() << std::endl;
		std::cerr << "Error Source: " << e.getSource() << std::endl;
		return -1;
	}
	catch (std::exception &e) {
		std::cerr << "Uncaught Exception: " << e.what() << std::endl;
		return -1;
	}
#endif
}
