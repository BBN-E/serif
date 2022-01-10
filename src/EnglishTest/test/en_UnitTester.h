#ifndef en_UNIT_TESTER_H
#define en_UNIT_TESTER_H

#include "Generic/test/UnitTester.h"

class EnglishUnitTester : public UnitTester {
private:
	friend class EnglishUnitTesterFactory;

public:
	boost::unit_test::test_suite* initUnitTests();
	
private:
	EnglishUnitTester();

};


class EnglishUnitTesterFactory: public UnitTester::Factory {
	virtual UnitTester *build() { return _new EnglishUnitTester(); } 
};
 

#endif
