#include "Generic/common/leak_detection.h"

#include "Generic/test/TestUtil.h"
#include "Generic/theories/TokenSequence.h"

#pragma warning(push)
#pragma warning(disable : 4266)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include <vector>
#include <string>


void TestUtil::assertEqual(TokenSequence *theory, std::string* expected, size_t expected_size) {
	std::vector<std::string> e(expected, expected + expected_size / sizeof(expected[0]));
	TestUtil::assertEqual(theory, e);
}


void TestUtil::assertEqual(TokenSequence *theory, std::vector<std::string> expected) {

	BOOST_CHECK_EQUAL((size_t)theory->getNTokens(), expected.size());

	for (size_t i = 0; i < expected.size(); i++) {
		BOOST_CHECK_EQUAL(theory->getToken(i)->getSymbol().to_debug_string(), expected.at(i));
	}
}
