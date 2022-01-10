// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
//#include "common/HeapChecker.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"

#include "ReplaceLowFreq/ReplaceLowFreq.h"
#include "ExtractBigrams/ExtractBigrams.h"
#include "ExtractBigrams/WSTokenizer.h"
#include "Cluster/MICluster.h"

#include <vector>
#if defined (_WIN32)
#include <direct.h>
#include <process.h>
#endif
#include <ctime>
#include <boost/scoped_ptr.hpp>


using namespace std;


//MAX_SENTENCE_TOKENS
const int max_token_sequences = 1;

void printDot(time_t &t) {
	time_t cur_time;
	time(&cur_time);
	if (cur_time - t >= 60) {
		cout << ".";
		t = cur_time;
	}
}

void printPercentage(int cur_count, int &prev_percentage, int total_count) {
	int percentage = static_cast<int>((cur_count / float(total_count)) * 100);
	if (percentage > prev_percentage) {
		if (percentage > 0)
			cout << "\b\b";
		if (percentage > 10)
			cout << "\b";
		cout << percentage << "%";
		prev_percentage = percentage;
	}
}

void print100Percent(int percentage) {
	if (percentage < 100) {
		cout << "\b\b\b100%";
	}
}

void runTokenizer(const char* param_file, const char* input_file, const char* output_file, const wchar_t* debug_filename) {
	/* We use a standalone tokenizer executable here. The main reason for this is the memory issue.
	** When working with >100M words input, the number of Symbol objects created finally crashes
	** the system. The standalone tokenizer is invoked for every file to have the memory clear.
	** USAGE: StandaloneTokenizer.exe <param_file> <input_file> <output_file> <append_to_output_bit>
	*/
	static const std::string standaloneTokenizerBin = ParamReader::getParam("standalone_tokenizer_bin", "StandaloneTokenizer.exe ");
	string command = standaloneTokenizerBin+" ";
	command.append("\"");
	command.append(param_file);
	command.append("\"");
	command.append(" \"");
	command.append(input_file);
	command.append("\" \"");
	command.append(output_file);
	command.append("\" 1");
	cout << "The tokenizer cmd is: " << command.c_str() << "\n";
	if (system(command.c_str()) != 0) {
		cerr << "The tokenizer has failed on " << debug_filename << "\n";

		//return -1;
	}
}

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "Cluster.exe should be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	const int MAX_TOKEN_COUNT = 1000000;

	char tokens_file[500];
	char temp_file[500];
	try {

		time_t cur_time;
		time(&cur_time);
		cout << ctime(&cur_time);
		cout << "Initializing...\n";
		const char* param_file = argv[1];
		ParamReader::readParamFile(param_file);

		char document_filelist[500];
		if(!ParamReader::getParam("cluster_document_filelist",document_filelist, 500)){
			throw UnexpectedInputException(
			"Cluster::Main()",
			"Missing Parameter: cluster-document-filelist");
		}

		// if unspecified, this will be false; i.e. default = do tokenization
		bool skip_tokenization = ParamReader::isParamTrue("skip_tokenization");
 		// if unspecified, this will be false; i.e. default = retain case information
        bool make_lowercase = ParamReader::isParamTrue("make_tokens_lowercase");
		if (make_lowercase && !skip_tokenization) {
			throw UnexpectedInputException(
			"Cluster::Main()",
			"make-tokens-lowercase can only be 'true' when skip-tokenization is also 'true'");
		}			
		
		if(!ParamReader::getParam("tokens_file", tokens_file, 500)){
			throw UnexpectedInputException(
			"Cluster::Main()",
			"Missing Parameter: tokens-file");
		}

		UTF8OutputStream tokenStream;
		if (skip_tokenization) {
			tokenStream.open(tokens_file);
		} else {
			// don't need temp file if we're skipping tokenization
			if(!ParamReader::getParam("temp_file",temp_file, 500)){
				throw UnexpectedInputException(
					"Cluster::Main()",
					"Missing Parameter: temp-file");
			}
		}

		char outfile[500];
		if(!ParamReader::getParam("cluster_outfile",outfile, 500)){
			throw UnexpectedInputException(
			"Cluster::Main()",
			"Missing Parameter: cluster-outfile");
		}
	
		char threshold_str[10];
		if(!ParamReader::getParam("prune_threshold",threshold_str, 10)){
			throw UnexpectedInputException(
			"Cluster::Main()",
			"Missing Parameter: prune-threshold");
		}
		int threshold = atoi(threshold_str);

		char output_rare_words[10];
		if(!ParamReader::getParam("output_rare_words",output_rare_words, 10)){
			throw UnexpectedInputException(
			"Cluster::Main()",
			"Missing Parameter: output-rare-words");
		}
		char rare_words_file[500];
		if (strcmp(output_rare_words, "true") == 0) {
			if(!ParamReader::getParam("rare_words_file",rare_words_file, 500)){
				throw UnexpectedInputException("Cluster::Main()", "Missing Parameter: rare-words-file");
			}
		}

		char serif_style_cluster_output[10];
		if(!ParamReader::getParam("serif_style_cluster_output", serif_style_cluster_output, 10)){
			throw UnexpectedInputException(
			"Cluster::Main()",
			"Missing Parameter: serif-style-cluster-output");
		}

		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& uis(*uis_scoped_ptr);
		char msg[1000];
		
		if(uis.fail()){
			strcpy(msg, "Couldn't open document file list: ");
			strcat(msg, document_filelist);
			throw UnexpectedInputException(
			"Cluster::Main()",
			msg);
		}

		wstring path;
		UTF8Token token;
		int token_counter = 0;
		int batch_counter = 0;
		uis.open(document_filelist);
		while (!uis.eof()) {
			uis.getLine(path);
			if (path.size() == 0)
				continue;
			Symbol path_sym = Symbol(path.c_str());
			boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(path.c_str()));
			UTF8InputStream& in(*in_scoped_ptr);
			
			if (skip_tokenization) {
				wstring line;
				while (!in.eof()) {
					in.getLine(line);
					if (make_lowercase) {
						std::wstring::size_type length = line.length();
						for (size_t i = 0; i < length; ++i) {
							line[i] = towlower(line[i]);
						}
					}
					tokenStream << line << L"\n";
				}
				in.close();
			} else {

				// Create temporary file TEMP_FILE containing no more than 1 million words.
				UTF8OutputStream temp(temp_file);
				batch_counter = 0;
				while (!in.eof()) {
					if (token_counter > MAX_TOKEN_COUNT) {
						temp.close();
						batch_counter++;
						time(&cur_time);
						cout << ctime(&cur_time);
						cout << "Running Tokenizer on " << path_sym.to_debug_string() << ", batch # " << batch_counter << " consisting of 1 million words...\n";
						runTokenizer(param_file, temp_file, tokens_file, path_sym.to_string());
						token_counter = 0;
						temp.open(temp_file);
					}
					token_counter++;
					in >> token;
					if (token_counter > 1)
						temp << L" ";
					temp << token.chars();
				}
				in.close();
				temp.close();
				if (token_counter > 0) {
					batch_counter++;
					time(&cur_time);
					cout << ctime(&cur_time);
					cout << "Running Tokenizer on " << path_sym.to_debug_string() << ", batch # " << batch_counter << " consisting of " << token_counter << " words...\n";
					runTokenizer(param_file, temp_file, tokens_file, path_sym.to_string());
				}

			}
		}
		uis.close();
		if (skip_tokenization)
			tokenStream.close();
		else remove(temp_file);

		int prev_percentage = -1;
		int cur_count = 0;
		int token_count = 0;


		time(&cur_time);
		cout << ctime(&cur_time);
		cout << "Counting the total number of words...";
		uis.close();
		boost::scoped_ptr<UTF8InputStream> tokensIn_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& tokensIn(*tokensIn_scoped_ptr);
		tokensIn.open(tokens_file);
		if(tokensIn.fail()){
			strcpy(msg, "Couldn't open tokens file: ");
			strcat(msg, document_filelist);
			throw UnexpectedInputException(
			"Cluster::Main()",
			msg);
		}
		
		while (!tokensIn.eof()) {
			tokensIn >> token;
			token_count++;
		}
		tokensIn.close();
		cout << token_count << "\n";

		time(&cur_time);
		cout << ctime(&cur_time);
		cout << "Adding counts...";
		ReplaceLowFreq * replaceLowFreq = _new ReplaceLowFreq();
		boost::scoped_ptr<UTF8InputStream> tokensIn2_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& tokensIn2(*tokensIn2_scoped_ptr);
		tokensIn2.open(tokens_file);
		while (!tokensIn2.eof()) {
			tokensIn2 >> token;
			replaceLowFreq->addCounts(token.symValue());
			cur_count++;
			printPercentage(cur_count, prev_percentage, token_count);
		}
		tokensIn2.close();
		print100Percent(prev_percentage);
		cout << endl;

		time(&cur_time);
		cout << ctime(&cur_time);
		cout << "Pruning...\n";
		if (strcmp(output_rare_words, "true") == 0)
			replaceLowFreq->pruneToThreshold(threshold, rare_words_file);
		else
			replaceLowFreq->pruneToThreshold(threshold);

		time(&cur_time);
		cout << ctime(&cur_time);
		cout << "Replacing low frequency words and Extracting Bigrams...";
		cur_count = 0;
		prev_percentage = -1;
		ExtractBigrams * extractor = _new ExtractBigrams();
		boost::scoped_ptr<UTF8InputStream> tokensIn3_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& tokensIn3(*tokensIn3_scoped_ptr);
		tokensIn3.open(tokens_file);
		while (!tokensIn3.eof()) {
			wstring tokenized_sent;
			tokensIn3.getLine(tokenized_sent);
			vector<wstring> tokens;
			WSTokenizer::tokenize(tokenized_sent, tokens);
			vector <wstring> results = replaceLowFreq->doReplaceLowFreq(tokens);
			extractor->extractBigrams(results);
			cur_count = cur_count + static_cast<int>(tokens.size());
			printPercentage(cur_count, prev_percentage, token_count);
		}
		tokensIn3.close();
		print100Percent(prev_percentage);
		cout << endl;
		delete replaceLowFreq;

		// if you'd like to save .voc and .bigram files, uncomment the following line. "test" will be the prefix of the files.
		//extractor->printFiles("test");

		time(&cur_time);
		cout << ctime(&cur_time);
		cout << "Loading bigrams...";
		MICluster * cluster = _new MICluster();
		cluster->loadVocabulary(extractor->getVocabulary());
		ExtractBigrams::BigramCount * bigramCount = extractor->getBigrams();
		for (ExtractBigrams::BigramCount::iterator iter = bigramCount->begin(); iter != bigramCount->end(); ++iter)
		{
			cluster->loadBigram((*iter).first._h, (*iter).first._f, (*iter).second);
			printDot(cur_time);
		}
		cout << endl;
		delete extractor;

		time(&cur_time);
		cout << ctime(&cur_time);
		cout << "Clustering...\n";
		cluster->doClusters(outfile);
		delete cluster;

		// delete the tokens file created by the StandaloneTokenizer
		remove(tokens_file);
	}
	catch (UnexpectedInputException& e){
		cout<<e.getMessage()<<" in "<<e.getSource()<<endl;
		return -1;
	}
	catch (char * err) {
		cout << "Internal error.\n" << err << "\nExiting...\n";
		remove(tokens_file);
		return -1;
	}
	catch (char const* err) {
		cout << "Internal error.\n" << err << "\nExiting...\n";
		remove(tokens_file);
		return -1;
	}

#ifdef ENABLE_LEAK_DETECTION
		ParamReader::finalize();

		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
		_CrtDumpMemoryLeaks();
#endif

#if defined(_DEBUG) || defined(_UNOPTIMIZED)
		printf("Press enter to exit....\n");
		getchar();
#endif

	//HeapChecker::checkHeap("main(); About to exit after successful run");

	return 0;
}
