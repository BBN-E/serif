// Copyright 2015 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/FeatureModule.h"
#include "Generic/common/ParamReader.h"
#include "Generic/test/UnitTester.h"
#include <boost/test/unit_test.hpp>

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Error: expected a parameter file" << std::endl;
		throw new std::exception();
	}

	try {
		ParamReader::readParamFile(argv[1]);
		FeatureModule::load();
	} catch (UnexpectedInputException &e) {
		std::cerr << e.getMessage() << std::endl;
		throw e;
	}
	
	UnitTester *unitTester = UnitTester::build();
	return unitTester->initUnitTests();
}
