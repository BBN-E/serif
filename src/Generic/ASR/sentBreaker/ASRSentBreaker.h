// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ASR_SENT_BREAKER_H
#define ASR_SENT_BREAKER_H


#include "Generic/common/Symbol.h"
#include "Generic/ASR/sentBreaker/ASRSentBreakerFVecModel.h"
#include "Generic/ASR/sentBreaker/ASRSentBreakerCustomModel.h"

#include <string>
#include <vector>

class UTF8InputStream;
class UTF8OutputStream;


class ASRSentBreaker {
public:
	ASRSentBreaker();

	void decode();

private:
	void decodeFile(const char *input_file, const char *output_file);

	void readHeader(std::wstring &header, UTF8InputStream &in);
	void readFooter(std::wstring &footer, UTF8InputStream &in);
	void readBody(std::vector<Symbol> &tokens,
				  std::vector<std::wstring> &preMetadata,
				  std::vector<std::wstring> &postMetadata,
				  UTF8InputStream &in);

	void writeContents(std::wstring &header,
					   std::vector<Symbol> &tokens,
					   std::vector<std::wstring> &preMetadata,
					   std::vector<std::wstring> &postMetadata,
					   std::wstring &footer,
					   UTF8OutputStream &out);

	void insertSentenceBreaks(std::vector<Symbol> &tokens,
							  std::vector<std::wstring> &preMetadata,
							  std::vector<std::wstring> &postMetadata);
	void breakLongSentence(std::vector<Symbol> &tokens,
						   std::vector<std::wstring> &preMetadata,
						   std::vector<std::wstring> &postMetadata,
						   size_t start, size_t end);

	void insertBreak(std::vector<std::wstring> &preMetadata,
					 std::vector<std::wstring> &postMetadata,
					 size_t i);

	double getSTScore(Symbol word, Symbol word1, Symbol word2,
					  int sent_len);


	const static Symbol START_SENTENCE;
	const static Symbol CONT_SENTENCE;
	const static wstring SENT_OPEN;
	const static wstring SENT_CLOSE;

	int _max_sentence_length;

	std::string _input_batch;
	std::string _output_dir;

	ASRSentBreakerFVecModel *_fvec_backoff_model;
	ASRSentBreakerCustomModel *_custom_model;
};

#endif

