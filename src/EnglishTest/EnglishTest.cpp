#define BOOST_TEST_MAIN
#include "Generic/common/FeatureModule.h"
#include "Generic/common/ParamReader.h"
#include "English/Test/Test.h"
#include "English/Test/tokens/TestEnglishTokenizer.h"
#include "English/Test/tokens/TestIteaEnglishTokenizer.h"
#include <boost/test/unit_test.hpp>

struct ParamFixture {
	ParamFixture() {
		if (boost::unit_test::framework::master_test_suite().argc < 2) {
			std::cerr << "Error: expected a parameter file" << std::endl;
			throw new std::exception();
		}
		try {
			BOOST_TEST_MESSAGE("Reading param file");
			ParamReader::readParamFile(boost::unit_test::framework::master_test_suite().argv[1]);
			FeatureModule::load();
		} catch (UnexpectedInputException &e) {
			std::cerr << e.getMessage() << std::endl;
			throw;
		}
	}
};

BOOST_GLOBAL_FIXTURE( ParamFixture );
