#ifndef xx_UNIT_TESTER_H
#define xx_UNIT_TESTER_H

#include "Generic/test/UnitTester.h"
#include <boost/test/unit_test.hpp>

class DefaultUnitTester: public UnitTester {

	boost::unit_test::test_suite* initUnitTests() { return 0; }

};


class DefaultUnitTesterFactory: public UnitTester::Factory {
	virtual UnitTester *build() { return _new DefaultUnitTester(); } 
};


#endif
