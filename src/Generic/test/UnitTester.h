#ifndef UNIT_TESTER_H
#define UNIT_TESTER_H

#include <boost/shared_ptr.hpp>

#pragma warning(push)
#pragma warning(disable : 4266)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

class UnitTester {
public:
	/** Create and return a new UnitTester. */
	static UnitTester *build() { return _factory()->build(); }
	/** Hook for registering new UnitTester factories. */
	struct Factory { virtual UnitTester *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }


	virtual boost::unit_test::test_suite* initUnitTests() = 0;

private:
	static boost::shared_ptr<Factory> &_factory();

};


#endif
