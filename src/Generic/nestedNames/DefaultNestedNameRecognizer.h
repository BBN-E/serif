#ifndef DEFAULT_NESTED_NAME_RECOGNIZER_H
#define DEFAULT_NESTED_NAME_RECOGNIZER_H

#include "Generic/nestedNames/NestedNameRecognizer.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED DefaultNestedNameRecognizer : public NestedNameRecognizer {

public:
	const static int MAX_DOC_NAMES = 2000;

	DefaultNestedNameRecognizer();
	~DefaultNestedNameRecognizer();

	int getNestedNameTheories(NestedNameTheory **results, int max_theories,
		TokenSequence *tokenSequence, NameTheory *nameTheory);

	void resetForNewSentence(const Sentence *sentence) {};
	void resetForNewDocument(class DocTheory *docTheory = 0) {};

private:
	bool _debug_flag;
	bool _do_nested_names;
	bool _skip_all_namefinding;
	int _num_names;
	int _sent_no;
	DocTheory *_docTheory;
	const TokenSequence* _tokenSequence;
	std::set<std::wstring> _nameList; 

	// functions for finding token matches for nested names
	int findMatchingStartToken(int index, int start_token, int end_token);
	int findMatchingEndToken(int index, int start_token, int end_token);

};

#endif
