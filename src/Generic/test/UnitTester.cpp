#include "Generic/common/leak_detection.h"

#include "Generic/test/UnitTester.h"
#include "Generic/test/xx_UnitTester.h"

boost::shared_ptr<UnitTester::Factory> &UnitTester::_factory() {
	static boost::shared_ptr<UnitTester::Factory> factory(new DefaultUnitTesterFactory());
	return factory;
}
