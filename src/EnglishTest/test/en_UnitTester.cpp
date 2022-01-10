#include "Generic/common/leak_detection.h"

#include "EnglishTest/tokens/TestEnglishTokenizer.h"
#include "EnglishTest/tokens/TestIteaEnglishTokenizer.h"
#include "EnglishTest/test/en_UnitTester.h"

EnglishUnitTester::EnglishUnitTester() {}

boost::unit_test::test_suite* EnglishUnitTester::initUnitTests() {

	boost::unit_test::test_suite* ts1 = BOOST_TEST_SUITE("English Tokenization");
	ts1->add( BOOST_TEST_CASE ( &tokenize_empty_sentence ));
	ts1->add( BOOST_TEST_CASE ( &dollar_signs ));
	ts1->add( BOOST_TEST_CASE ( &double_quotes ));
	ts1->add( BOOST_TEST_CASE ( &open_quotes ));
	ts1->add( BOOST_TEST_CASE ( &leading_punctuation ));
	ts1->add( BOOST_TEST_CASE ( &ignore_parentheticals ));
	ts1->add( BOOST_TEST_CASE ( &leading_punctuation_protected ));
	ts1->add( BOOST_TEST_CASE ( &split_hyphenated_caps ));

	boost::unit_test::framework::master_test_suite().add(ts1);

	boost::unit_test::test_suite* ts2 = BOOST_TEST_SUITE("ITEA English Tokenization");
	ts2->add( BOOST_TEST_CASE ( &specialized_replacements ));
	ts2->add( BOOST_TEST_CASE ( &replace_question_marks ));
	ts2->add( BOOST_TEST_CASE ( &split_slashes_special ));
	ts2->add( BOOST_TEST_CASE ( &leading_punctuation_itea ));
	
	boost::unit_test::framework::master_test_suite().add(ts2);

	return 0;
}
