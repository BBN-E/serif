// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Sexp.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/morphAnalysis/SessionLexicon.h"
#include "Generic/morphSelection/MorphModel.h"
#include "Generic/morphSelection/MorphDecoder.h"
#include "Generic/morphSelection/MorphSelector.h"
#include "Generic/normalizer/MTNormalizer.h"
#include "Generic/tokens/Tokenizer.h"
#include "Generic/tokens/SymbolSubstitutionMap.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/LexicalTokenSequence.h"
#include "Generic/theories/LexicalToken.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/common/version.h"
#include "Generic/common/FeatureModule.h" 
#include <boost/scoped_ptr.hpp>


#define MAX_LINE_LENGTH 15000
wchar_t line[MAX_LINE_LENGTH];

void print_usage() {
	std::cout << "Training Usage: MorphologyTrainer -t PARAM_FILE TRAIN_FILE MULT (MODEL_PREFIX)" << std::endl;
	std::cout << "Decode Usage (Untokenized Input): MorphologyTrainer -d PARAM_FILE [<input-file> <output-file>]" << std::endl;
	std::cout << "Decode Usage (for name-training sexp input): MorphologyTrainer -dsexp PARAM_FILE [<input-file> <output-file>]" << std::endl;
	std::cout << "Decode Usage (Pre-Tokenized and Analyzed Input): MorphologyTrainer -a PARAM_FILE" << std::endl;
	std::cout << "Tokenizer Usage: MorphologyTrainer -tok PARAM_FILE" << std::endl;
}

bool readSentence(UTF8InputStream& uis, LocatedString* loc) {
	int len, start, end;
	len = 0;

	while (!uis.eof()) {
		uis.getLine(line, MAX_LINE_LENGTH);
		len = static_cast<int>(wcslen(line));
		if (len == 0) 
			continue;
		start = 0;
		end = len-1;
		while((start < len) && iswspace(line[start])) start++;
		while((end > 0) && iswspace(line[end])) end--;
		if (start != end) 
			break;
	}
	if (len == 0) {
		loc = 0;
		return false;
	}
	if (line[start] != L'(') {
		throw UnexpectedInputException("MorphologyTrainerMain::readSentence()",
									   "Test sentence doesn't start with '('");
	}
	if (line[end] != L')') {
		throw UnexpectedInputException("MorphologyTrainerMain::readSentence()",
										"Test sentence doesn't end with ')'");
	}
	start++;
	end++;
	LocatedString* temp = _new LocatedString(line);
	temp->trim();
	start = temp->indexOf(L"(");
	end = temp->indexOf(L")");
	loc = temp->substring(start+1, end);
	loc->replace(L"-LRB-",L"(");
	loc->replace(L"-RRB-",L")");

	delete temp;
	return true;
}

bool readTokenizedAndAnalyzedSentence(UTF8InputStream& uis, TokenSequence** tokens, LocatedString &tokenString) {
	FeatureValueStructure *fvs = 0;
	Token **origTokens = _new Token*[MAX_SENTENCE_TOKENS];
	Token **morphTokens = _new Token*[MAX_SENTENCE_TOKENS];
	Sexp *sentence = _new Sexp(uis);
	int n_lex = 0;

	if (sentence->isVoid())
		return false;

	if (sentence->getNumChildren() != 2)
		throw UnexpectedInputException("MorphologyTrainerMain::readTokenizedAndAnalyzedSentence()",
									   "Encountered improper sexp format.");

	Sexp *tokenList = sentence->getFirstChild();
	Sexp *morphList = sentence->getSecondChild();
	tokenString = LocatedString(tokenList->to_token_string().c_str());
	int n_tokens = tokenList->getNumChildren() < MAX_SENTENCE_TOKENS ? tokenList->getNumChildren() : MAX_SENTENCE_TOKENS;

	if (morphList->getNumChildren() != tokenList->getNumChildren())
		throw UnexpectedInputException("MorphologyTrainerMain::readTokenizedAndAnalyzedSentence()",
									   "Encountered improper sexp format.");

	int curr_char = 0;
	for (int i = 0; i < n_tokens; i++) {
		
		Sexp *analysisList = morphList->getNthChild(i);
		int n_analyses = analysisList->getNumChildren();
		LexicalEntry **analyses = _new LexicalEntry*[n_analyses];
		
		for (int j = 0; j < n_analyses; j++) {
			Sexp *analysis = analysisList->getNthChild(j);
			int n_morphs = analysis->getNumChildren();
			LexicalEntry **morphEntries = _new LexicalEntry*[n_morphs];
			for (int k = 0; k < n_morphs; k++) {
				Sexp *morph = analysis->getNthChild(k);
				fvs = FeatureValueStructure::build(); // TODO: fix this
				morphEntries[k] = _new LexicalEntry(n_lex++, morph->getValue(), fvs, NULL, 0);
			}
			fvs = FeatureValueStructure::build(); // TODO: fix this
			analyses[j] = _new LexicalEntry(n_lex++, tokenList->getNthChild(i)->getValue(),
							  fvs, morphEntries, n_morphs);
		}

		std::wstring token_str = wstring(tokenList->getNthChild(i)->getValue().to_string());
		size_t token_len = token_str.length();
		OffsetGroup startGroup = OffsetGroup(CharOffset(curr_char), EDTOffset(curr_char));
		OffsetGroup endGroup = OffsetGroup(CharOffset(curr_char + token_len - 1), EDTOffset(curr_char + token_len - 1));

//#if defined(ARABIC_LANGUAGE) || defined(KOREAN_LANGUAGE)
		if (SerifVersion::isArabic() || SerifVersion::isKorean()) {
			origTokens[i] = _new LexicalToken(startGroup, endGroup, tokenList->getNthChild(i)->getValue(), i);
			morphTokens[i] = _new LexicalToken(startGroup, endGroup, tokenList->getNthChild(i)->getValue(), i, n_analyses, analyses);
		}
//#endif
		curr_char += token_len + 1; // add an extra character for whitespace
		delete [] analyses;
	}

//#if defined(ARABIC_LANGUAGE) || defined(KOREAN_LANGUAGE)
	if (SerifVersion::isArabic() || SerifVersion::isKorean()) {
		tokens[0] = _new LexicalTokenSequence(0, n_tokens, origTokens);
	} else {
//#else
		tokens[0] = _new TokenSequence(0, n_tokens, origTokens);
	}
//#endif
	tokens[0]->retokenize(n_tokens, morphTokens);

	delete [] origTokens;
	delete [] morphTokens;
	delete sentence;
	return true;
}

void runTraining(int argc, char* argv[]) {
	try {
		char mprefix[500];

		if (argc != 5 && argc != 6) {
			print_usage();
			return;
		}
			
		const char* pfile = argv[2];
		const char* tfile = argv[3];
		const char* mbuffer = argv[4];
		int mult = atoi(mbuffer);
			
		ParamReader::readParamFile(pfile);
		FeatureModule::load();

		if (argc == 6) { 
			strncpy(mprefix, argv[5], 500);
		}
		else if	(argc == 5)	{
			ParamReader::getRequiredParam("MorphModel", mprefix, 500);
		}
		std::cout << "Model Prefix: " << mprefix << " Mult: " << mult << std::endl;

		//Create a default session logger since the lexicon assumes its there
		std::wstringstream buffer;
		buffer << mprefix << ".trainSession.log";
		SessionLogger* sessionLogger = _new FileSessionLogger(buffer.str().c_str(), 0, 0);
		SessionLogger::logger = sessionLogger;

		MorphModel *m = MorphModel::build();
		m->setMultiplier(mult);
		m->trainOnSingleFile(tfile, mprefix);

		delete m;
		delete sessionLogger;

	} catch (UnrecoverableException e) {
		std::cout << "Exception: " << e.getSource() << " " << e.getMessage() << std::endl;
	}	
}
void runTokenizer(int argc, char* argv[]) {
	try {

		const char* pfile = argv[2];
		ParamReader::readParamFile(pfile);
		FeatureModule::load();

		std::string dfile = ParamReader::getRequiredParam("input_file");
		std::cout << "Input File: " << dfile << std::endl;

		std::string ofile = ParamReader::getRequiredParam("output_file");
		std::cout << "Output File: " << ofile << std::endl;

		std::string map = ParamReader::getRequiredParam("reverse_subst_map");
		SymbolSubstitutionMap* reverseSubs = _new SymbolSubstitutionMap(map.c_str());

		LocatedString *sentenceString = 0;
		Tokenizer *tokenizer = Tokenizer::build();
		TokenSequence *tokens[1];

		//Create a default session logger since the lexicon assumes its there
		std::wstring log(ofile.begin(), ofile.end());
		log.append(L"-tokenize.log");
		SessionLogger* sessionLogger = _new FileSessionLogger(log.c_str(), 0, 0);
		SessionLogger::logger = sessionLogger;

        boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build(dfile.c_str()));
        UTF8InputStream& uis(*uis_scoped_ptr);
		UTF8OutputStream uos(ofile.c_str());
		int linecount = 0;

		while (!uis.eof()) {
			linecount++;
			if ((linecount % 250) == 0) {
				std::cout << "Read Line " << linecount << std::endl;
			}

			uis.getLine(line, MAX_LINE_LENGTH);
			if (wcslen(line) == 0) 
				continue;
			sentenceString = _new LocatedString(line);

			tokenizer->getTokenTheories(tokens, 1, sentenceString);

			for (int j = 0; j < tokens[0]->getNTokens(); j++) {
				Symbol out = reverseSubs->replace(tokens[0]->getToken(j)->getSymbol());
				uos << out.to_string() << " ";
			}
			uos << L"\n";

			delete tokens[0];
			delete sentenceString;
		}

		uis.close();
		uos.close();
		
		delete tokenizer;
		delete sessionLogger;
		delete reverseSubs;

	} catch (UnrecoverableException e) {
		std::cout << "Exception: " << e.getSource() << " " << e.getMessage() << std::endl;
	}		

}

void runDecoderNameTraining(int argc, char* argv[]){
	// Expects input of the form ((word1 NONE-ST) (word2 GPE-ST) ... )
	// Outputs the same form.
	try {
		if (argc != 3 && argc != 5) {
			print_usage();
			return;
		}

		const char* pfile = argv[2];
		ParamReader::readParamFile(pfile);
		FeatureModule::load();
	
		char dfile[600];
		char ofile[600];
		if (argc == 3) {
			ParamReader::getRequiredParam("input_file", dfile, 600);
			ParamReader::getRequiredParam("output_file", ofile, 600);
		}
		else {
			strncpy(dfile, argv[3], 600);
			strncpy(ofile, argv[4], 600);
		}

		std::cout << "Input File: " << dfile << std::endl;
		std::cout << "Output File: " << ofile << std::endl;

		//Create a default session logger since the lexicon assumes its there
		std::wstringstream buffer;
		buffer << ofile << "-tokenize.log";
		SessionLogger* sessionLogger = _new FileSessionLogger(buffer.str().c_str(), 0, 0);
		SessionLogger::logger = sessionLogger;
		
		Symbol *origTags = _new Symbol[MAX_SENTENCE_TOKENS];

		
		MorphologicalAnalyzer *morphAnalysis = MorphologicalAnalyzer::build();
		std::cout << "Initialized Morphological Analyzer" << std::endl;

		MorphSelector *morphSelector = _new MorphSelector();
		std::cout << "Initialized Morphological Selector" << std::endl;

		NameClassTags *nameClassTags = _new NameClassTags();
		
		MTNormalizer *mtNormalizer;
		bool do_mt_normalization = ParamReader::isParamTrue("do_mt_normalization");
		if(do_mt_normalization)
			mtNormalizer = MTNormalizer::build();

        boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build(dfile));
        UTF8InputStream& uis(*uis_scoped_ptr);
		UTF8OutputStream uos(ofile);
		int linecount = 0;

		TokenSequence *tokenSequenceBuf;
		Token **origTokens;

		origTokens = _new Token*[MAX_SENTENCE_TOKENS];

		while (!uis.eof()) {
			linecount++;
			if ((linecount % 250) == 0) {
				std::cout << "Read Line " << linecount << std::endl;
				// This line appears to fix a memory leak that causes a crash after about 23000 sentences.
				SessionLexicon::getInstance().getLexicon()->clearDynamicEntries();
			}

			Sexp sentence(uis);
			if(sentence.isVoid())
				continue;
			int n_tokens = sentence.getNumChildren() < MAX_SENTENCE_TOKENS ? sentence.getNumChildren() : MAX_SENTENCE_TOKENS;
			int curr_char = 0;
			for (int i = 0; i < n_tokens; i++) {
				Sexp *pair = sentence.getNthChild(i);
				std::wstring token_str = wstring(pair->getFirstChild()->getValue().to_string());
				size_t token_len = token_str.length();
				OffsetGroup startGroup = OffsetGroup(CharOffset(curr_char), EDTOffset(curr_char));
				OffsetGroup endGroup = OffsetGroup(CharOffset(curr_char + token_len - 1), EDTOffset(curr_char + token_len - 1));
//#if defined(ARABIC_LANGUAGE) || defined(KOREAN_LANGUAGE)
				if (SerifVersion::isArabic() || SerifVersion::isKorean()) {
					origTokens[i] = _new LexicalToken(startGroup, endGroup, pair->getFirstChild()->getValue(), i);
				}
//#endif			
				origTags[i] = pair->getSecondChild()->getValue();
				curr_char += token_len + 1; // add an extra character for whitespace
			}

//#if defined(ARABIC_LANGUAGE) || defined(KOREAN_LANGUAGE)
			if (SerifVersion::isArabic() || SerifVersion::isKorean()) {
				tokenSequenceBuf = _new LexicalTokenSequence(0, n_tokens, origTokens);
			} else {
//#else
				tokenSequenceBuf = _new TokenSequence(0, n_tokens, origTokens);
			}
//#endif
			LocatedString *sentenceString = _new LocatedString(sentence.to_token_string().c_str());
			morphAnalysis->getMorphTheories(tokenSequenceBuf);
			morphSelector->selectTokenization(sentenceString, tokenSequenceBuf);
			if(do_mt_normalization){
				int numTokensBeforeNormalization = tokenSequenceBuf->getNTokens();

				mtNormalizer->normalize(tokenSequenceBuf);

				int numTokensAfterNormalization = tokenSequenceBuf->getNTokens();
				if(numTokensBeforeNormalization != numTokensAfterNormalization){
					// In the rare event that a token is deleted, it could cause a problem like the first tag being NONE-CO.
					// Rather than fixing the tag, we just skip the sentence.
					std::cout << "Number of tokens changed during normalization. Skipping line.\n";
					delete tokenSequenceBuf;
					morphAnalysis->resetForNewSentence();
					morphAnalysis->resetDictionary();
					morphSelector->resetForNewSentence();
					mtNormalizer->resetForNewSentence();
					continue;
				}
			}
			
//#if defined(ARABIC_LANGUAGE) || defined(KOREAN_LANGUAGE)
			if (SerifVersion::isArabic() || SerifVersion::isKorean()) {
				uos << "( " ;
				for (int i = 0; i < tokenSequenceBuf->getNTokens(); i++) {
					const LexicalToken *out_token = dynamic_cast<const LexicalToken*>(tokenSequenceBuf->getToken(i));
					Symbol out_tag = origTags[out_token->getOriginalTokenIndex()];

					// If out_token is not at the beginning of an original token, replace out_tag with it's -CO form
					if(i > 0)
						if(out_token->getOriginalTokenIndex() == dynamic_cast<const LexicalToken*>(tokenSequenceBuf->getToken(i-1))->getOriginalTokenIndex())
							out_tag = nameClassTags->getTagSymbol(nameClassTags->getContinueForm(nameClassTags->getIndexForTag(out_tag)));

					uos << "( " << out_token->getSymbol().to_string() << " " << out_tag.to_string()  << " )";
				}
				uos << " )" << L"\n";
			}
//#endif

			delete tokenSequenceBuf;
			delete sentenceString;
			morphAnalysis->resetForNewSentence();
			morphAnalysis->resetDictionary();
			morphSelector->resetForNewSentence();
			if(do_mt_normalization){
				mtNormalizer->resetForNewSentence();
			}
		}

		uis.close();
		uos.close();

		delete sessionLogger;
		delete [] origTokens;
		delete [] origTags;
		delete morphAnalysis;
		delete morphSelector;
		delete nameClassTags;
		if(do_mt_normalization)
			delete mtNormalizer;

	} catch (UnrecoverableException e) {
		std::cout << "Exception: " << e.getSource() << " " << e.getMessage() << std::endl;
	}		
}
void runDecoder(int argc, char* argv[]) {
	try {
		if (argc != 3 && argc != 5) {
			print_usage();
			return;
		}

		const char* pfile = argv[2];
		ParamReader::readParamFile(pfile);
		FeatureModule::load();
	
		char dfile[600];
		char ofile[600];
		if (argc == 3) {
			ParamReader::getRequiredParam("input_file", dfile, 600);
			ParamReader::getRequiredParam("output_file", ofile, 600);
		}
		else {
			strncpy(dfile, argv[3], 600);
			strncpy(ofile, argv[4], 600);
		}

		std::cout << "Input File: " << dfile << std::endl;
		std::cout << "Output File: " << ofile << std::endl;

		// Cluster training doesn't need sexp, just text.
		bool output_for_cluster_training = ParamReader::isParamTrue("output_for_cluster_training");

		LocatedString *sentenceString = 0;
		Tokenizer *tokenizer = Tokenizer::build();
		TokenSequence *tokens[1];
		Symbol words[MAX_SENTENCE_TOKENS];
		Symbol sentence[MAX_SENTENCE_TOKENS];

		//Create a default session logger since the lexicon assumes its there
		std::string expt_dir = ParamReader::getRequiredParam("experiment_dir");
		std::wstring log_file(expt_dir.begin(), expt_dir.end());
		log_file.append(L"/decode.log");
		SessionLogger* sessionLogger = _new FileSessionLogger(log_file.c_str(), 0, 0);
		SessionLogger::logger = sessionLogger;

		MorphologicalAnalyzer *morphAnalyzer = MorphologicalAnalyzer::build();
		std::cout << "Initialized Morphological Analyzer" << std::endl;

		MorphSelector *morphSelector = _new MorphSelector();
		std::cout << "Initialized Morphological Selector" << std::endl;

		MTNormalizer *mtNormalizer;
		bool do_mt_normalization = ParamReader::isParamTrue("do_mt_normalization");
		if(do_mt_normalization)
			mtNormalizer = MTNormalizer::build();
		
		boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build(dfile));
		UTF8InputStream& uis(*uis_scoped_ptr);
		UTF8OutputStream uos(ofile);
		int linecount = 0;

		while (!uis.eof()) {
			linecount++;
			if ((linecount % 250) == 0) {
				std::cout << "Read Line " << linecount << std::endl;
				// This line fixes a memory leak in runDecoderNameTraining(), and was added here out of caution.
				SessionLexicon::getInstance().getLexicon()->clearDynamicEntries();
			}

			uis.getLine(line, MAX_LINE_LENGTH);
			if (wcslen(line) == 0){
				std::cout << "Blank line " << linecount << std::endl;
				continue;
			}
			sentenceString = _new LocatedString(line);

			tokenizer->getTokenTheories(tokens, 1, sentenceString);
			morphAnalyzer->getMorphTheories(tokens[0]);
			morphSelector->selectTokenization(sentenceString, tokens[0]);
			if(do_mt_normalization)
				mtNormalizer->normalize(tokens[0]);
			
			int num_tokens = tokens[0]->getNTokens();

			if(!output_for_cluster_training)
				uos<< "( ";
			for (int j = 0; j < num_tokens; j++) {
				uos << tokens[0]->getToken(j)->getSymbol().to_string() << " ";
			}
			if(!output_for_cluster_training)
				uos << L" )";
			uos << "\n";
				
			delete tokens[0];
			delete sentenceString;

			morphAnalyzer->resetDictionary();
			morphSelector->resetForNewSentence();
			if(do_mt_normalization)
				mtNormalizer->resetForNewSentence();
		}
		
		uis.close();
		uos.close();

		delete tokenizer;
		delete sessionLogger;
		delete morphAnalyzer;
		delete morphSelector;
		if(do_mt_normalization)
			delete mtNormalizer;

	} catch (UnrecoverableException e) {
		std::cout << "Exception: " << e.getSource() << " " << e.getMessage() << std::endl;
	}
}
void runDecoderPretokenized(int argc, char* argv[]) {
	try {
		char buffer[700];

		const char* pfile = argv[2];
		ParamReader::readParamFile(pfile);
		FeatureModule::load();

		ParamReader::getParam("reverse_subst_map", buffer, 700);
		SymbolSubstitutionMap* reverseSubs = _new SymbolSubstitutionMap(buffer);

		//Create a default session logger since the lexicon assumes its there
		std::string log_file = ParamReader::getRequiredParam("log_file");
		SessionLogger* sessionLogger = _new FileSessionLogger(std::wstring(log_file.begin(), log_file.end()).c_str(), 0, 0);
		SessionLogger::logger = sessionLogger;

		TokenSequence **tokens = _new TokenSequence*[1];
		LocatedString sentenceString(L"");
		Symbol words[MAX_SENTENCE_TOKENS];
		int map[MAX_SENTENCE_TOKENS];

		MorphDecoder *morphDecoder = MorphDecoder::build();
		std::cout << "Initialized Morphological Decoder" << std::endl;

		if (ParamReader::isParamTrue("list_mode")) {
			char file_list[600];
			char dfile[600];
			char ofile[600];

			ParamReader::getRequiredParam("input_file_list", file_list, 600);
			std::cout << "Input File List: " << file_list << std::endl;
				
			std::basic_ifstream<char> in;
			in.open(file_list);
			if (in.fail()) {
				throw UnexpectedInputException("MorphologyTrainerMain::main()",
												"could not open input file list");
			}

			while (!in.eof()) {
				in.getline(dfile, 600);
				sprintf(ofile, "%s.morph", dfile);

				if (strcmp(dfile, "") == 0) 
					break;

				std::cout << "Input File: " << dfile << std::endl;
				std::cout << "Output File: " << ofile << std::endl;

				boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build(dfile));
				UTF8InputStream& uis(*uis_scoped_ptr);
				UTF8OutputStream uos(ofile);
				int linecount = 0;

				while (readTokenizedAndAnalyzedSentence(uis, tokens, sentenceString)) {
					linecount++;
					if ((linecount % 250) == 0) {
						std::cout << "Read Line " << linecount << std::endl;
					}

					int num_tokens = morphDecoder->getBestWordSequence(sentenceString, tokens[0], words, map, MAX_SENTENCE_TOKENS);
					
					//morphDecoder->printTrellis(uos);

					int map_idx = 0;
					uos << "( (";
					for (int j = 0; j < num_tokens; j++) {
						Symbol out = reverseSubs->replace(words[j]);
						if (map_idx != map[j]) {
							uos << ") (";
							// mark tokens where decoder had >1 analysis to choose from
//#if defined(ARABIC_LANGUAGE) || defined(KOREAN_LANGUAGE)
							if (SerifVersion::isArabic() || SerifVersion::isKorean()) {

								if (dynamic_cast<const LexicalToken*>(tokens[0]->getToken(map[j]))->getNLexicalEntries() > 1)
									uos << "*";
							}
//#endif
							map_idx = map[j];
						}
						uos << words[j].to_string();
						if ((j != num_tokens - 1) && (map[j] == map[j+1]))
							uos << "+";
					}
					uos << L") )\n";
				
					delete tokens[0];
				}
			
				uis.close();
				uos.close();
			}
				
			in.close();
		}
		else {
			char dfile[600];
			ParamReader::getRequiredParam("input_file", dfile, 600);
			std::cout << "Input File: " << dfile << std::endl;

			char ofile[600];
			ParamReader::getRequiredParam("output_file", ofile, 600);
			std::cout << "Output File: " << ofile << std::endl;
		
			boost::scoped_ptr<UTF8InputStream> uis_scoped_ptr(UTF8InputStream::build(dfile));
			UTF8InputStream& uis(*uis_scoped_ptr);
			UTF8OutputStream uos(ofile);
			int linecount = 0;

			while (readTokenizedAndAnalyzedSentence(uis, tokens, sentenceString)) {
				linecount++;
				if ((linecount % 250) == 0) {
					std::cout << "Read Line " << linecount << std::endl;
				}

				int num_tokens = morphDecoder->getBestWordSequence(sentenceString, tokens[0], words, map, MAX_SENTENCE_TOKENS);
				
				//morphDecoder->printTrellis(uos);

				int map_idx = 0;
				uos << "( (";
				for (int j = 0; j < num_tokens; j++) {
					Symbol out = reverseSubs->replace(words[j]);
					if (map_idx != map[j]) {
						uos << ") (";
						// mark tokens where decoder had >1 analysis to choose from
//#if defined(ARABIC_LANGUAGE) || defined(KOREAN_LANGUAGE)
						if (SerifVersion::isArabic() || SerifVersion::isKorean()) {
							if (dynamic_cast<const LexicalToken*>(tokens[0]->getToken(map[j]))->getNLexicalEntries() > 1)
								uos << "*";
						}
//#endif
						map_idx = map[j];
					}
					uos << words[j].to_string();
					if ((j != num_tokens - 1) && (map[j] == map[j+1]))
						uos << "+";
				}
				uos << L") )\n";
			
				delete tokens[0];
			}
		
			uis.close();
			uos.close();
		}

		delete sessionLogger;
		delete reverseSubs;
		delete morphDecoder;

	} catch (UnrecoverableException e) {
		std::cout << "Exception: " << e.getSource() << " " << e.getMessage() << std::endl;
	}
}
int main(int argc, char* argv[]) {

	std::cout << "MorphologyTrainerMain v2.0" << std::endl;
	if (argc != 3 && argc != 5 && argc != 6) {
		print_usage();
		return 0;
	}

	if (strcmp(argv[1], "-t") == 0) {
		runTraining(argc, argv);
	} else if (strcmp(argv[1], "-tok") == 0) {
		runTokenizer(argc, argv);
	} else if (strcmp(argv[1], "-dsexp") == 0) {
		runDecoderNameTraining(argc, argv);
	} else if (strcmp(argv[1], "-d") == 0) {
		runDecoder(argc, argv);
	} else if (strcmp(argv[1], "-a") == 0) {
		runDecoderPretokenized(argc, argv);
	} else {
		print_usage();
		return 0;
	}

	return 0;
}
