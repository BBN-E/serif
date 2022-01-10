#ifndef TEST_ITEA_ENGLISH_TOKENIZER_H
#define TEST_ITEA_ENGLISH_TOKENIZER_H

#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/test/TestUtil.h"
#include "Generic/tokens/Tokenizer.h"

#pragma warning(push)
#pragma warning(disable : 4266)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include <string>

struct TestIteaEnglishTokenizerFixture {

	Tokenizer *tokenizer;
	TokenSequence **tokenSequenceBuf;
	int max_token_sequences;
	Document doc;

	TestIteaEnglishTokenizerFixture() : doc(Symbol(L"TEST_DOC")) {
		// Manually set the ITEA parameters
		ParamReader::setParam("use_itea_tokenization", "true");
		ParamReader::setParam("replace_question_marks", "true");
		
		tokenizer = Tokenizer::build();
		max_token_sequences = ParamReader::getRequiredIntParam("token_branch");
		tokenSequenceBuf = new TokenSequence*[max_token_sequences];
	}

	~TestIteaEnglishTokenizerFixture() {
		delete tokenizer;
		delete [] tokenSequenceBuf;

		// reset all params to their original values
		ParamReader::finalize();
		ParamReader::readParamFile(boost::unit_test::framework::master_test_suite().argv[1]);
	}
};

void specialized_replacements() {
	TestIteaEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	LocatedString input1(L" John ((Doe))");
	std::string expected1[] = {"John", "-LDB-", "Doe", "-RDB-"};
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));
	
	LocatedString input2(L" John ( (Doe) )");
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input2);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));


	LocatedString input3(L" John Doe (ALSO KNOWN AS James Bond)");
	std::string expected3[] = {"John", "Doe", "-LRB-", "AKA", "James", "Bond", "-RRB-"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input3);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected3, sizeof(expected3));
}

void replace_question_marks() {
	TestIteaEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	LocatedString input1(L" ? ");
	std::string expected1[] = {"-"};
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));

	// Not sure this is the intended behavior, so for now I'll
	// just document it as current behavior
	LocatedString input2(L" ????? ");
	std::string expected2[] = {"'", "--", "?", "'"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input2);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected2, sizeof(expected2));
}

void split_slashes_special() {
	TestIteaEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	// possible name followed by possible phone number
	LocatedString input1(L"Smith/7819239123");
	std::string expected1[] = {"Smith", "/", "7819239123"};
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));

	// possible phone number not long enough
	LocatedString input2(L"Smith/78192");
	std::string expected2[] = {"Smith/78192"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input2);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected2, sizeof(expected2));

	// plus sign okay immediately after slash
	LocatedString input3(L"Smith/+7819239123");
	std::string expected3[] = {"Smith", "/", "+7819239123"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf,f. max_token_sequences,
					&input3);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected3, sizeof(expected3));

	// plus sign not okay anywhere else
	LocatedString input4(L"Smith/781+9239123");
	std::string expected4[] = {"Smith/781+9239123"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input4);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected4, sizeof(expected4));
}

// For ITEA we don't remove leading punctuation
void leading_punctuation_itea() {
	TestIteaEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	LocatedString input1(L"**-leading");
	std::string expected1[] = {"**-leading"};
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));
	
}

#endif
