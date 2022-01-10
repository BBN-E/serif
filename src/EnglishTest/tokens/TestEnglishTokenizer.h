#include "Generic/common/LocatedString.h"
#include "Generic/common/ParamReader.h"
#include "Generic/test/TestUtil.h"
#include "Generic/tokens/Tokenizer.h"
#include "English/tokens/en_Tokenizer.h"

#pragma warning(push)
#pragma warning(disable : 4266)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include <iostream>
#include <stdio.h>

static Symbol TELEPHONE_SOURCE_SYM = Symbol(L"telephone");

struct TestEnglishTokenizerFixture {

	Tokenizer *tokenizer;
	TokenSequence **tokenSequenceBuf;
	int max_token_sequences;
	Document doc;

	TestEnglishTokenizerFixture() : doc(Symbol(L"TEST_DOC")) {
		ParamReader::setParam("tokenizer_ignore_parentheticals", "true");
		tokenizer = Tokenizer::build();
		max_token_sequences = ParamReader::getRequiredIntParam("token_branch");
		tokenSequenceBuf = _new TokenSequence*[max_token_sequences];
	}

	~TestEnglishTokenizerFixture() {
		delete tokenizer;
		delete [] tokenSequenceBuf;

		// reset all params to their original values
		ParamReader::finalize();
		ParamReader::readParamFile(boost::unit_test::framework::master_test_suite().argv[1]);
	}
};


void tokenize_empty_sentence() {

	TestEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	LocatedString input1(L"");
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	BOOST_CHECK_EQUAL(f.tokenSequenceBuf[0]->getNTokens(), 0);

	LocatedString input2(L"\t\n");
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input2);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	BOOST_CHECK_EQUAL(f.tokenSequenceBuf[0]->getNTokens(), 0);

	f.doc.setSourceType(TELEPHONE_SOURCE_SYM);
	LocatedString input3(L" uh ");
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input3);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	BOOST_CHECK_EQUAL(f.tokenSequenceBuf[0]->getNTokens(), 0);

}

void dollar_signs() {

	TestEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	LocatedString input1(L"$1");
	std::string expected1[] = {"$", "1"};
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));

	LocatedString input2(L"$1 $2 $3 $4 $5 $6 $7 $8 $9");
	std::string expected2[] = {"$", "1", "$", "2", "$", "3", "$", "4", "$", "5", "$", "6", "$", "7", "$", "8", "$", "9"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input2);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected2, sizeof(expected2));

	LocatedString input3(L"$1$2$3");
	std::string expected3[] = {"$", "1$", "2$", "3"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input3);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected3, sizeof(expected3));

	LocatedString input4(L"$1 hello");
	std::string expected4[] = {"$", "1", "hello"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input4);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected4, sizeof(expected4));

	LocatedString input5(L"hello $1 hello");
	std::string expected5[] = {"hello", "$", "1", "hello"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input5);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected5, sizeof(expected5));

	LocatedString input6(L"hello $1");
	std::string expected6[] = {"hello", "$", "1"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input6);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected6, sizeof(expected6));

	LocatedString input7(L"he said, \"'now");
	std::string expected7[] = {"he", "said", ",", "``", "`", "now"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input7);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected7, sizeof(expected7));

	LocatedString input8(L"\"`hello");
	std::string expected8[] = {"``", "`", "hello"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input8);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected8, sizeof(expected8));

	LocatedString input9(L"he said, \"`now");
	std::string expected9[] = {"he", "said", ",", "``", "`", "now"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input9);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected9, sizeof(expected9));

	LocatedString input10(L"hello \"\" world");
	std::string expected10[] = {"hello", "``", "``", "world"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input10);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected10, sizeof(expected10));
}

void double_quotes() {

	TestEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	LocatedString input1(L"\"hello");
	std::string expected1[] = {"``", "hello"};
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));

	LocatedString input2(L"he said, \"now is the time");
	std::string expected2[] = {"he", "said", ",", "``", "now", "is", "the", "time"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input2);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected2, sizeof(expected2));

	
	LocatedString input3(L"end of my story.\"");
	std::string expected3[] = {"end", "of", "my", "story", ".", "''"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input3);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected3, sizeof(expected3));

	LocatedString input4(L"\"'hello");
	std::string expected4[] = {"``", "`", "hello"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input4);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected4, sizeof(expected4));

	LocatedString input5(L"he said, \"'now");
	std::string expected5[] = {"he", "said", ",", "``", "`", "now"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input5);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected5, sizeof(expected5));
}

void open_quotes() {
	TestEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	LocatedString input1(L"hello''");
	std::string expected1[] = {"hello", "''"};
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));

	LocatedString input2(L"hello ''' world");
	std::string expected2[] = {"hello", "'", "''", "world"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input2);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected2, sizeof(expected2));

	
	LocatedString input3(L"hello ''90s world");
	std::string expected3[] = {"hello", "''", "90s", "world"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input3);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected3, sizeof(expected3));
	
	LocatedString input4(L"he said, ''nobody likes Michael Bolton");
	std::string expected4[] = {"he", "said", ",", "``", "nobody", "likes", "Michael", "Bolton"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input4);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected4, sizeof(expected4));
	
	LocatedString input5(L"why would you write ''_hello ?");
	std::string expected5[] = {"why", "would", "you", "write", "''", "_hello", "?"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input5);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected5, sizeof(expected5));

	LocatedString input6(L"hello.'");
	std::string expected6[] = {"hello", ".", "'"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input6);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected6, sizeof(expected6));

	LocatedString input7(L"end of my story.'`great story!");
	std::string expected7[] = {"end", "of", "my", "story.", "'", "`", "great", "story", "!"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input7);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected7, sizeof(expected7));

	LocatedString input8(L"that '70s show");
	std::string expected8[] = {"that", "'", "70s", "show"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input8);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected8, sizeof(expected8));
	
	LocatedString input9(L"don't tell me you're still here");
	std::string expected9[] = {"do", "n't", "tell", "me", "you", "'re", "still", "here"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input9);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected9, sizeof(expected9));

	LocatedString input10(L"the F.B.I.'s anti-privacy campaign");
	std::string expected10[] = {"the", "F.B.I.", "'s", "anti-privacy", "campaign"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input10);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected10, sizeof(expected10));

	LocatedString input11(L"so he said, 'what gives?");
	std::string expected11[] = {"so", "he", "said", ",", "`", "what", "gives", "?"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input11);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected11, sizeof(expected11));

	LocatedString input12(L"why would you say '_hello ?");
	std::string expected12[] = {"why", "would", "you", "say", "'", "_hello", "?"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input12);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected12, sizeof(expected12));

	LocatedString input13(L"'i am sam. sam i am.");
	std::string expected13[] = {"`", "i", "am", "sam.", "sam", "i", "am", "."};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input13);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected13, sizeof(expected13));

	LocatedString input14(L"_hello");
	std::string expected14[] = {"_hello"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input14);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected14, sizeof(expected14));

}

void leading_punctuation() {
	TestEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	LocatedString input1(L"**-leading");
	std::string expected1[] = {"leading"};
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));

	LocatedString input2(L"**leading");
	std::string expected2[] = {"**leading"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input2);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected2, sizeof(expected2));
}

void ignore_parentheticals() {
	TestEnglishTokenizerFixture f;

	f.tokenizer->resetForNewSentence(&f.doc, 0);

	LocatedString input1(L" John ((Doe))");
	std::string expected1[] = {"John", "Doe"};
	int n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input1);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected1, sizeof(expected1));

	LocatedString input2(L"Ordinary English sentences (like this one)");
	std::string expected2[] = {"Ordinary", "English", "sentences", "-LRB-", "like", "this", "one", "-RRB-"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input2);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected2, sizeof(expected2));

	LocatedString input3(L"green onions (scallions)");
	std::string expected3[] = {"green", "onions"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input3);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected3, sizeof(expected3));

	LocatedString input4(L"four (4)");
	std::string expected4[] = {"four"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input4);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected4, sizeof(expected4));

	LocatedString input5(L"four (comment: yep)");
	std::string expected5[] = {"four", "-LRB-", "yep", "-RRB-"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input5);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected5, sizeof(expected5));

	// this is probably not the intended behavior
	LocatedString input6(L"four (if you want you can comment on my list)");
	std::string expected6[] = {"four", "-LRB-", "my", "list", "-RRB-"};
	n_sequences = f.tokenizer->getTokenTheories(
					f.tokenSequenceBuf, f.max_token_sequences,
					&input6);
	BOOST_CHECK_EQUAL(n_sequences, 1);
	TestUtil::assertEqual(f.tokenSequenceBuf[0], expected6, sizeof(expected6));
}


/* 
 * Class derived from EnglishTokenizer to enable access to
 * protected methods.
 */
class TestEnglishTokenizerProtected : public EnglishTokenizer {

public:
	void replaceParentheticalMarkup(LocatedString *string) {
		EnglishTokenizer::replaceParentheticalMarkup(string);
	}

	void splitHyphenatedCaps(LocatedString *string) {
		EnglishTokenizer::splitHyphenatedCaps(string);
	}

	void removeLeadingPunctuation(LocatedString *string) {
		EnglishTokenizer::removeLeadingPunctuation(string);
	}

};

struct TestEnglishTokenizerProtectedFixture {

	TestEnglishTokenizerProtected *tokenizer;

	TestEnglishTokenizerProtectedFixture() {
		tokenizer = new TestEnglishTokenizerProtected();
	}

	~TestEnglishTokenizerProtectedFixture() {
		delete tokenizer;
	}
};



void leading_punctuation_protected() {
	TestEnglishTokenizerProtectedFixture f;

	LocatedString input1(L"***-leading");
	f.tokenizer->removeLeadingPunctuation(&input1);
	BOOST_CHECK_EQUAL(input1.toSymbol().to_debug_string(), "leading");
}


void split_hyphenated_caps() {
	TestEnglishTokenizerProtectedFixture f;

	LocatedString input1(L"-");
	LocatedString input2(L"");
	LocatedString input3(L"A BBN-internal memo");
	LocatedString input4(L"A bbn-internal memo");

	LocatedString input5(L"A bbn-Internal memo");
    LocatedString input6(L"A -bbn-Internal memo");
	LocatedString input7(L"A bbn--Internal memo");

	f.tokenizer->splitHyphenatedCaps(&input1);
	BOOST_CHECK_EQUAL(input1.toSymbol().to_debug_string(), "-");

	f.tokenizer->splitHyphenatedCaps(&input2);
	BOOST_CHECK_EQUAL(input2.toSymbol().to_debug_string(), "");

	f.tokenizer->splitHyphenatedCaps(&input3);
	BOOST_CHECK_EQUAL(input3.toSymbol().to_debug_string(), "A BBN -internal memo");

	f.tokenizer->splitHyphenatedCaps(&input4);
	BOOST_CHECK_EQUAL(input4.toSymbol().to_debug_string(), "A bbn-internal memo");

	f.tokenizer->splitHyphenatedCaps(&input5);
	BOOST_CHECK_EQUAL(input5.toSymbol().to_debug_string(), "A bbn- Internal memo");

	f.tokenizer->splitHyphenatedCaps(&input6);
	BOOST_CHECK_EQUAL(input6.toSymbol().to_debug_string(), "A -bbn-Internal memo");

	f.tokenizer->splitHyphenatedCaps(&input7);
	BOOST_CHECK_EQUAL(input7.toSymbol().to_debug_string(), "A bbn--Internal memo");
}

