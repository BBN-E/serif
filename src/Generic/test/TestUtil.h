#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <vector>
#include <string>

class TokenSequence;

class TestUtil {
public:
	static void assertEqual(TokenSequence *theory, std::string* expected, size_t expected_size);
	static void assertEqual(TokenSequence *theory, std::vector<std::string> expected);
};

#endif
