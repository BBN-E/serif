// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/ASR/sentBreaker/ASRSentBreakerFVecModel.h"
#include "Generic/ASR/sentBreaker/ASRSentBreaker.h"

#include <string>
#include <vector>
#include <boost/scoped_ptr.hpp>


using namespace std;


const Symbol ASRSentBreaker::START_SENTENCE(L"ST");
const Symbol ASRSentBreaker::CONT_SENTENCE(L"CO");
const wstring ASRSentBreaker::SENT_OPEN(L"<TURN>");
const wstring ASRSentBreaker::SENT_CLOSE(L"</TURN>");

ASRSentBreaker::ASRSentBreaker()
	: _fvec_backoff_model(0), _custom_model(0)
{
	_max_sentence_length = ParamReader::getRequiredIntParam("max_sentence_length");

	_input_batch = ParamReader::getRequiredParam("input_batch");
	_output_dir = ParamReader::getRequiredParam("output_dir");

	std::string model_file = ParamReader::getParam("fvec_backoff_model_file");
	if (!model_file.empty()) {
		boost::scoped_ptr<UTF8InputStream> modelIn_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& modelIn(*modelIn_scoped_ptr);
		modelIn.open(model_file.c_str());
		if (modelIn.fail()) {
			throw UnexpectedInputException("ASRSentBreaker::ASRSentBreaker()",
										   "Unable to open model file");
		}
		_fvec_backoff_model = _new ASRSentBreakerFVecModel(modelIn);
		modelIn.close();
	}

	model_file = ParamReader::getParam("custom_model_file");
	if (!model_file.empty()) {
/*		UTF8InputStream modelIn;
		modelIn.open(_model_file);
		if (modelIn.fail()) {
			throw UnexpectedInputException("ASRSentBreaker::ASRSentBreaker()",
										   "Unable to open model file");
		}*/
		_custom_model = _new ASRSentBreakerCustomModel(model_file.c_str());
	}

	if (_fvec_backoff_model == 0 && _custom_model == 0) {
		throw UnexpectedInputException("ASRSentBreaker::ASRSentBreaker",
									   "No model file parameter specified.");
	}
}

void ASRSentBreaker::decode() {
	ifstream batch;
	batch.open(_input_batch.c_str());
	if (batch.fail()) {
		throw UnexpectedInputException("ASRSentBreaker::decode()",
									   "Unable to open input batch file");
	}

	for (;;) {
		char input_file[500];
		batch.getline(input_file, 500);

		if (input_file[0] == '\0')
			break;

		// extract file name from path
		string file_name = input_file;
		size_t name_start = file_name.rfind('/');
		if (name_start == file_name.npos)
			name_start = file_name.rfind('\\');
		if (name_start == file_name.npos)
			name_start = 0;
		file_name = file_name.substr(name_start);

		SessionLogger::info("SERIF") << "Processing: " << file_name.c_str() << "...\n";

		// append file name to output-dir to get output path
		string output_file = _output_dir;
		char last_dir_char = output_file.c_str()[output_file.length()-1];
		if (last_dir_char != '/' && last_dir_char != '\\')
			output_file += '/';
		output_file += file_name;

		decodeFile(input_file, output_file.c_str());
	}
}

void ASRSentBreaker::decodeFile(const char *input_file,
								const char *output_file)
{
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(input_file);
	if (in.fail()) {
		string message = "Unable to open input file: ";
		message += input_file;
		throw UnexpectedInputException("ASRSentBreaker::decodeFile()",
									   message.c_str());
	}

	UTF8OutputStream out;
	out.open(output_file);
	if (out.fail()) {
		string message = "Unable to create output file: ";
		message += output_file;
		throw UnexpectedInputException("ASRSentBreaker::decodeFile()",
									   message.c_str());
	}

	wstring header;
	readHeader(header, in);

	vector<Symbol> tokens;
	vector<wstring> preMetadata;
	vector<wstring> postMetadata;
	readBody(tokens, preMetadata, postMetadata, in);

	wstring footer;
	readFooter(footer, in);

	insertSentenceBreaks(tokens, preMetadata, postMetadata);

	writeContents(header, tokens, preMetadata, postMetadata, footer, out);
}

void ASRSentBreaker::readHeader(wstring &header, UTF8InputStream &in) {
	for (;;) {
		if (in.eof()) {
			throw UnexpectedInputException("ASRSentBreaker::readHeader()",
				"No <TEXT> tag found in input.");
		}

		wstring line;
		in.getLine(line);

		if (line.compare(L"<TEXT>") == 0)
			return;

		header += line;
		header += L"\n";
	}
}

void ASRSentBreaker::readFooter(wstring &footer, UTF8InputStream &in) {
	while (!in.eof()) {
		wstring line;
		in.getLine(line);

		footer += line;
		footer += L"\n";
	}
}

void ASRSentBreaker::readBody(vector<Symbol> &tokens,
							  vector<wstring> &preMetadata,
							  vector<wstring> &postMetadata,
							  UTF8InputStream &in)
{
	int token_no = 0;

	for (;;) {
		if (in.eof()) {
			throw UnexpectedInputException("ASRSentBreaker::readBody()",
				"No </TEXT> tag found in input.");
		}

		wstring line;
		in.getLine(line);

		if (line.compare(L"</TEXT>") == 0)
			return;

		size_t token_start = line.rfind(L' ');
		if (token_start == line.npos) {
			cerr << "Warning: Throwing out input line (token "
				 << token_no << ").\n";
			continue;
		}
		token_start += 1; // we actually want position right after space

		size_t token_end = line.length();
/*		size_t token_end = line.length() - 1;
		while (line.c_str()[token_end] == L'\r' ||
			   line.c_str()[token_end] == L'\n')
		{
			token_end--;
		}*/

		tokens.push_back(Symbol(
			line.substr(token_start, token_end - token_start).c_str()));

		preMetadata.push_back(line.substr(0, token_start));
		postMetadata.push_back(line.substr(token_end));

		token_no++;
	}
}

void ASRSentBreaker::writeContents(wstring &header,
								   vector<Symbol> &tokens,
								   vector<wstring> &preMetadata,
								   vector<wstring> &postMetadata,
								   wstring &footer,
								   UTF8OutputStream &out)
{
	out << header;
	out << L"<TEXT>\n";

	for (size_t i = 0; i < tokens.size(); i++) {
		out << preMetadata[i];
		out << tokens[i].to_string();
		out << postMetadata[i];
		out << L"\n";
	}

	out << L"</TEXT>\n";
	out << footer;
}

void ASRSentBreaker::insertSentenceBreaks(vector<Symbol> &tokens,
										  vector<wstring> &preMetadata,
										  vector<wstring> &postMetadata)
{
	size_t n_tokens = tokens.size();

	if (n_tokens == 0)
		return; // this probably shouldn't happen....

	preMetadata[0] = SENT_OPEN + preMetadata[0];
	postMetadata[n_tokens-1] = SENT_CLOSE + postMetadata[n_tokens-1];

	if (n_tokens < 3)
		return;

	Symbol word, word1, word2;

	int sent_len = 2;
	word1 = tokens[0];
	word = tokens[1];
	for (size_t i = 2; i < n_tokens; i++) {
		word2 = word1;
		word1 = word;
		word = tokens[i];

		double score = getSTScore(word, word1, word2, sent_len);

		if (score > 0) {
			insertBreak(preMetadata, postMetadata, i);

			sent_len = 0;

		} else {
			
			sent_len++;

			if (sent_len > _max_sentence_length) {
				breakLongSentence(tokens, preMetadata, postMetadata,
								  i - sent_len, i);
				sent_len = 0;
			}
		}
	}
}

void ASRSentBreaker::breakLongSentence(vector<Symbol> &tokens,
									   vector<wstring> &preMetadata,
									   vector<wstring> &postMetadata,
									   size_t start, size_t end)
{
	SessionLogger::info("SERIF") << "Breaking long sentence from "
		 << (int)start << " to " << (int)end << ".\n";

	// we don't want to break right near the ends:
	// (this also assures that the 2 seed tokens precede my_start)
	size_t my_start = start + 5;
	size_t my_end = end - 5;

	Symbol word, word1, word2;

	double best_score = -10000;
	size_t best_sent;
	bool best_sent_set = false;
	word1 = tokens[my_start-2];
	word = tokens[my_start-1];
	for (size_t i = my_start; i < my_end; i++) {
		word2 = word1;
		word1 = word;
		word = tokens[i];

		double score = getSTScore(word, word1, word2, (int) (i - start));

		if (score > best_score) {
			best_sent = i;
			best_sent_set = true;
			best_score = score;
		}
	}

	if (best_sent_set) { // surely this is the case
		insertBreak(preMetadata, postMetadata, best_sent);

		// if we still have really long sentences, we must recurse
		if ((int) (best_sent - start) > _max_sentence_length) {
			breakLongSentence(tokens, preMetadata, postMetadata,
							  start, best_sent);
		}
		if ((int) (end - best_sent) > _max_sentence_length) {
			breakLongSentence(tokens, preMetadata, postMetadata,
							  best_sent, end);
		}
	}
}

void ASRSentBreaker::insertBreak(vector<wstring> &preMetadata,
								 vector<wstring> &postMetadata,
								 size_t i)
{
    postMetadata[i-1] = SENT_CLOSE + SENT_OPEN + postMetadata[i-1];
//	preMetadata[i].append(SENT_OPEN);
}

double ASRSentBreaker::getSTScore(Symbol word, Symbol word1, Symbol word2,
								  int sent_len)
{
	double p_st, p_co;
	ASRSentModelInstance stInstance(START_SENTENCE, word, word1, word2, 0);
	ASRSentModelInstance coInstance(CONT_SENTENCE, word, word1, word2, 0);

	if (_fvec_backoff_model) {
		p_st = _fvec_backoff_model->getProbability(&stInstance);
		p_co = _fvec_backoff_model->getProbability(&coInstance);
	}
	else { // _custom_model
		p_st = _custom_model->getProbability(&stInstance);
		p_co = _custom_model->getProbability(&coInstance);
	}

	return p_st - p_co;
}

