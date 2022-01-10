// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/TimedSection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/PartOfSpeechTable.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/FeatureModule.h" 
#include <boost/scoped_ptr.hpp>
/*
// #ifdef ARABIC_LANGUAGE
// #include "Arabic/parse/ar_ChartDecoder.h"
// #include "Arabic/parse/ar_SplitTokenSequence.h"
// #else
*/
#include "Generic/parse/ChartDecoder.h"
//#endif



//NOTE:  The StandAloneParser reads in pre tokenized, sentence broken text in the
// (word word .... word .) and decodes that text using the SERIF decoder.
// IF the USE_ARABIC parameter is invoked, the ar_parser (with clitic tokenization) is
// invoked, otherwise the standard parser is used.

// usage message
Symbol _sentence[MAX_SENTENCE_LENGTH];
Symbol _realSentence[MAX_SENTENCE_LENGTH];
Symbol _pos[MAX_SENTENCE_LENGTH];

PartOfSpeechTable* _auxPosTable;  // passed to buld ChartDecoder

// [edloper] As far as I can tell, the value of this variable
// (_constraints) never actually gets used.  In particular, there is a
// lcoal variable inside main() which was formerly *also* named
// _constraints, which shadowed this variable.  I renamed that
// variable to _constraints2 to avoid confusion.  Now this variable
// gets filled as part of readSentenceTokens(), but we never do
// anything with its value after that.
std::vector<Constraint> _constraints;

int print_usage() {
	std::cerr << "param_file input_file output_file log_file languageModule";
	std::cerr << "[-nbest] [-print_heads] ";
	std::cerr << "[USE_ARABIC/BW] [DO_NOT_COLLAPSE_NPAS]\n";
	return -1;
}
bool getBoolParamValue(const char* str){
	char result[50];
	if(ParamReader::getParam(str, result,  49)){
		if(strcmp(result, "true")== 0)
			return true;
	}
	return false;
}
int readSentenceTokens(UTF8InputStream& stream)
throw(UnexpectedInputException) {
	UTF8Token token;
	int numTok = 0;

	if (stream.eof())
		return 0;
	stream >> token;
	if (stream.eof())
		return 0;

	if(token.symValue() != Symbol(L"("))
		throw UnexpectedInputException("IdFSentenceTokens::readDecodeSentence",
		"ill-formed sentence -- no leading paren");
	bool constraints = false;
	stream >> token;
	if (token.symValue()==Symbol(L"(")) {
		constraints = true;
	} else if(token.symValue()==Symbol(L")")) {
		return 0;
	} else {
		if (numTok < MAX_SENTENCE_LENGTH) {
			_sentence[numTok] = token.symValue();
		}
		numTok++;
	}

	while (true) {
		stream >> token;
		if(token.symValue()==Symbol(L")"))
			break;
		if (numTok < MAX_SENTENCE_LENGTH) {
			_sentence[numTok] = token.symValue();
		}
		numTok++;
	}

	if (constraints) {
		stream >> token;
		if(token.symValue() != Symbol(L"("))
			throw UnexpectedInputException("IdFSentenceTokens::readDecodeSentence",
				"ill-formed sentence  -- no leading paren for constraints");
		while (true) {
			stream >> token;
			if(token.symValue() == Symbol(L")"))
				break;

			if(token.symValue() != Symbol(L"("))
				throw UnexpectedInputException("IdFSentenceTokens::readDecodeSentence",
					"ill-formed sentence -- no leading paren for a constraint");
			stream >> token;
			int constraint_left = _wtoi(token.chars());
			stream >> token;
			int constraint_right = _wtoi(token.chars());
			stream >> token;
			Symbol constraint_type = token.symValue();
			EntityType constraint_entityType = EntityType::getOtherType();
			stream >> token;
			if(token.symValue() != Symbol(L")"))
				throw UnexpectedInputException("IdFSentenceTokens::readDecodeSentence",
					"ill-formed sentence -- no closing paren for a constraint");
			_constraints.push_back(Constraint(constraint_left,
											  constraint_right,
											  constraint_type,
											  constraint_entityType));
		}

		stream >> token;
			if(token.symValue() != Symbol(L")"))
				throw UnexpectedInputException("IdFSentenceTokens::readDecodeSentence",
					"ill-formed sentence -- no closing paren for constraints");

	}

	return numTok;

}


int main(int argc, char* argv[])
{
	bool get_vars_from_param = false;

	/*
	if(argc != 2) {
		return print_usage();
	}
	*/

	if(argc == 2){
		get_vars_from_param = true;
	}
	else if(argc < 6 || argc > 10){
		std::cerr<<"argc: "<<argc<<std::endl;
		return print_usage();
	}

	//open files
	const wchar_t* context[] ={L"Session",
								  L"Document",
								  L"Sentence",
								  L"Stage"};

	try{
		int numTok;
		char infile[500];
		char warmfile[500];
		char outfilebase[500];
		char logfile[500];
		char languageModuleName[500];
		boost::scoped_ptr<UTF8InputStream> inStream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& inStream(*inStream_scoped_ptr);
		boost::scoped_ptr<UTF8InputStream> warmStream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& warmStream(*warmStream_scoped_ptr);
		UTF8OutputStream parseOutStream;
		//UTF8OutputStream tokOutStream;

		ParamReader::readParamFile(argv[1]);
		
		bool collapseNPA = true;
		bool useArabic = false;
		bool useBW = false;
		bool useNBest = false;
		bool printHeadInfo = false;
		bool warmup = false;  
		bool realrun = true;
		if(!get_vars_from_param){
			//log file
			strcpy_s(logfile, argv[4]);
			//parse, tokens files
			strcpy_s(outfilebase, argv[3]);
			//infile
			strcpy_s(infile, argv[2]);
			//Language Modules
			strcpy_s(languageModuleName, argv[5]);
			for (int arg = 6; arg < argc; arg++) {
				if (strcmp(argv[arg],"DO_NOT_COLLAPSE_NPAS")==0)
					collapseNPA = false;
				if (strcmp(argv[arg],"USE_ARABIC")==0)
					useArabic = true;
				if (strcmp(argv[arg],"USE_BW")==0)
					useBW = true;
				if (strcmp(argv[arg],"-nbest")==0)
					useNBest = true;
				if (strcmp(argv[arg],"-print_heads")==0)
					printHeadInfo = true;
				if (strcmp(argv[arg],"-warmup")==0){
					warmup = true;
					arg++;
					strcpy_s(warmfile,argv[arg]);
				}
			}
		}
		else{
			collapseNPA = getBoolParamValue("collapse_npa");
			useArabic = getBoolParamValue("use_arabic");
			useBW = getBoolParamValue("use_BW");
			useNBest = getBoolParamValue("use_nbest");
			printHeadInfo = getBoolParamValue("print_heads");
			if(!ParamReader::getParam("outfile",outfilebase, 500)){
				throw UnrecoverableException("Parser no outfile specified");
			}
			if(!ParamReader::getParam("infile",infile, 500)){
				throw UnrecoverableException("Parser no infile specified");
			}
			if(!ParamReader::getParam("logfile",logfile, 500)){
				throw UnrecoverableException("Parser no logfile specified");
			}
			if(!ParamReader::getParam("language_module", languageModuleName, 500)) {
				throw UnrecoverableException("Parser no language module specified");
			}
		}

		FeatureModule::load(languageModuleName);

		//read in aux POS table if specified, else make one that is empty
		char aux_pos[500];
		if(ParamReader::getParam("aux_pos_table", aux_pos, 500)){
			boost::scoped_ptr<UTF8InputStream> pos_uis_scoped_ptr(UTF8InputStream::build(aux_pos));
			UTF8InputStream& pos_uis(*pos_uis_scoped_ptr);
			//std::cerr<<"Opened aux_pos_table "<<aux_pos<<std::endl;
			_auxPosTable = _new PartOfSpeechTable(pos_uis);
			pos_uis.close();
			std::cerr<<"StandaloneParser loaded aux_pos_table "<<aux_pos<<std::endl;
		}else{
			_auxPosTable = _new PartOfSpeechTable();
		}
 
		std::string logfile_as_string(logfile);
		std::wstring logfile_as_wstring(logfile_as_string.begin(), logfile_as_string.end());
		//strcpy(file_buffer, outfilebase);
		//strcat(file_buffer ,".tokens");
		//tokOutStream.open(file_buffer);
		//session logger
		SessionLogger::logger = _new FileSessionLogger(logfile_as_wstring.c_str(),
								4, context);
		SessionLogger::logger->updateContext(0, "PARSING");
		SessionLogger::logger->beginMessage();
		*SessionLogger::logger << "_________________________________________________\n"
								<< "Starting session: \""
								<< "Parsing \"\n"
								<< "Parameters:\n";

		ParamReader::logParams();
		*SessionLogger::logger << "\n";
		char param_model_prefix[500];
		if (!ParamReader::getParam("parser_model",param_model_prefix,									 500))	{
		throw UnexpectedInputException("Parser::Parser()",
									   "Param `parser_model' not defined");
		}
		char frag_prob_str[500];
		if (!ParamReader::getParam("parser_frag_prob",frag_prob_str,			500))	{
		throw UnexpectedInputException("Parser::Parser()",
			"Param `parser_frag_prob' not defined");
		}
		double frag_prob = atof(frag_prob_str);

		bool constrainpos = getBoolParamValue("constrain_pos");
		bool constrainspans = getBoolParamValue("constrain_spans");
		bool skiptop = getBoolParamValue("skip_top_label");
		bool print_confidence =  getBoolParamValue("print_confidence");
		bool print_score = getBoolParamValue("print_score");


		if (frag_prob < 0)
			throw UnexpectedInputException("Parser::Parser()",
				"Param `parser_frag_prob' less than zero");

		std::vector<Constraint> _constraints2;

		if (useArabic && useBW) {
			return print_usage();
		}

		if (useArabic){
			std::cout<< "cant use arabic parser (USE_ARABIC)"<<std::endl;
			return print_usage();
/*
//	#ifdef ARABIC_LANGUAGE

			std::cout<<"Parsing Untokenized Data\n";
			ArabicChartDecoder* decoder;
			decoder = _new ArabicChartDecoder(param_model_prefix, frag_prob);
			//read sentence, decode, print parse
			int sentCount =0;
			while( (numTok = readSentenceTokens(inStream)) !=0 ){
				if(numTok > MAX_SENTENCE_LENGTH){
					std::cout<<"Truncating Sentence\n";
					numTok = MAX_SENTENCE_LENGTH;
				}
				std::cout<<"decoding sentence "<<sentCount<<"num Words= "<<numTok<<std::endl;
				//since our goal is evalb comparision, collapse npas
				ParseNode* result = decoder->decode(_sentence,numTok, 0, 0, collapseNPA);
				std::cout<<"Finished sentence "<<sentCount<<std::endl;

				parseOutStream <<result->toWString()<<"\n";

				tokOutStream<<result->writeTokens()<<"\n";

				sentCount++;
				delete result;
			}
			delete decoder;
//	#endif
*/

		}
		else if (useBW) {
			std::cout<< "cant use arabic parser (USE_BW)"<<std::endl;
			return print_usage();
/*
//	#ifdef ARABIC_LANGUAGE
			std::cout<<"Parsing Buckwalter's Tokenized Data\n";
			ArabicChartDecoder* decoder = _new ArabicChartDecoder(param_model_prefix, frag_prob);
			int sentCount = 0;
			SplitTokenSequence* toks = new SplitTokenSequence();
			int sent_num = 0;

			std::cout<< "READ AND PARSE\n";
			while(toks->readBuckwalterSentence(inStream)){
				std::cout << "--------------"<<std::endl;
				std::cout <<"start decode " <<sent_num++<<"\n";
				ParseNode* result = decoder->decode(toks,collapseNPA);
				std::cout <<"finished decode\n";
				parseOutStream <<result->toWString()<<L"\n";

				tokOutStream<<L"( S "<<result->writeTokens()<<L" )\n";
				tokOutStream.flush();
				parseOutStream.flush();
				delete toks;
				toks = new SplitTokenSequence();
			}
//	#endif
	*/
		}
		else {
			LanguageSpecificFunctions::setAsStandAloneParser();
			ChartDecoder* decoder;
			std::cout<<"Parsing Tokenized Data \n";

			decoder = _new ChartDecoder(param_model_prefix,frag_prob, _auxPosTable);
			if(print_confidence)
				decoder->readWordProbTable(param_model_prefix);

			if (warmup) {
				warmStream.open(warmfile);
				std::cout << "running warmup file " << warmfile << std::endl;
				} 
			inStream.open(infile);

			
			while (warmup || realrun) {
	
			  if (! warmup) std::cout << "running real file " << infile << std::endl;

			  char file_buffer[520];
			    //parse output file
			    strcpy_s(file_buffer, outfilebase);
			    strcat_s(file_buffer,".parses");
			    parseOutStream.open(file_buffer);
			  
			  {
				  TimedSection ts("parsing file");
			
			  int sentCount = 0;
			  bool moresent = true;
			  while( moresent ){
				if (warmup) 
					  numTok = readSentenceTokens(warmStream);
				else 
					  numTok = readSentenceTokens(inStream);
				if (numTok == 0) break;

			   //read sentence, decode, print parse(s)
				if ((sentCount % 100) == 0){
					if (warmup) std::cout << "warmup "<<sentCount<<" ";
				    else std::cout<<"decoding sentence "<<sentCount<<std::endl;
				}


				if(numTok > MAX_SENTENCE_LENGTH){
					std::cout<<"Truncating Sentence\n";
					numTok = MAX_SENTENCE_LENGTH;
				}
				for(int i = 0; i< numTok; i++){
					_realSentence[i] = _sentence[i];
				}
				Symbol* posconstraints = 0;
				if(constrainpos){

					int numpos = readSentenceTokens(inStream);
					if(numpos > MAX_SENTENCE_LENGTH){
						std::cout<<"Truncating Sentence\n";
						numTok = MAX_SENTENCE_LENGTH;
					}
					if(numpos != numTok){
						std::cerr<<"POS count "<<numpos
							<<" does not agree with token count "<<numTok<<std::endl;
						return -1;
					}
					for(int i=0; i<numpos; i++){
						_pos[i] = _sentence[i];
					}
					posconstraints = _pos;
				}
				if(constrainspans){
					UTF8Token tok;
					inStream >> tok;
					/*sentence n
					*	NPA	Start End
					*	....
					*END
					*/
					while(wcscmp(tok.chars(), L"END") != 0){
						inStream >> tok;
						int constraint_left = _wtoi(tok.chars());
						inStream >> tok;
						int constraint_right = _wtoi(tok.chars());
						inStream >> tok;
						_constraints2.push_back(Constraint(constraint_left,
														   constraint_right,
														   ParserTags::CONSTIT_CONSTRAINT));
					}

				}

				ParseNode* result = decoder->decode(_realSentence, numTok, _constraints2,
													collapseNPA, posconstraints);
				if (useNBest) {
					// print top scoring theory first, please
					parseOutStream << L"(\n";
					int score_index = 0;
					ParseNode* iter_result = result;
					while (iter_result != 0) {
						if (score_index == decoder->highest_scoring_final_theory) {
							parseOutStream << L"(";
							parseOutStream << decoder->theory_scores[score_index];
							parseOutStream << L" ";
							parseOutStream << iter_result->toWString(printHeadInfo);
							parseOutStream << L")";
							parseOutStream << L"\n";
						}
						iter_result = iter_result->next;
						score_index++;
					}
					iter_result = result;
					score_index = 0;
					while (iter_result != 0) {
						if (score_index != decoder->highest_scoring_final_theory) {
							parseOutStream << L"(";
							parseOutStream << decoder->theory_scores[score_index];
							parseOutStream << L" ";
							parseOutStream << iter_result->toWString(printHeadInfo);
							parseOutStream << L")";
							parseOutStream << L"\n";
						}
						iter_result = iter_result->next;
						score_index++;
					}
					parseOutStream << L")\n";
				}
				else {
					ParseNode* iter_result = result;
					int score_index = 0;
					while (iter_result != 0) {

						if (score_index == decoder->highest_scoring_final_theory) {
							if(skiptop)
								parseOutStream <<iter_result->toWString(printHeadInfo);
							else
								parseOutStream <<L"(TOP "<<iter_result->toWString(printHeadInfo)<<L")";

							parseOutStream<<L"\n";
							if (print_score || print_confidence) {
								float parsescore = decoder->theory_scores[score_index];
								if (print_confidence) {
									float unigramscore =  decoder->getProbabilityOfWords(_realSentence, numTok);
									float confidence = parsescore - unigramscore;
									parseOutStream << confidence <<" (P(W|MS): "<<parsescore
													<<") (P(W|MU) "<<unigramscore<<L")\n";
								}
								else {
									parseOutStream << parsescore << L"\n";
								}
							}

						}
						iter_result = iter_result->next;
						score_index++;
					}
				} // end tokens loop
				sentCount++;

				delete result;
			 } //end sentences in file  loop
			 } // end timed section for parsing this file
	
				parseOutStream.close();
				if (warmup) {
						warmup = false;
						std::cout<<"Parsing warm pass Completed"<<std::endl;
						*SessionLogger::logger<<"Parsing warm pass Completed \n";
						warmStream.close();
				}else {
						realrun = false;
						std::cout<<"Parsing Completed"<<std::endl;
						*SessionLogger::logger<<"Parsing Completed \n";
						inStream.close();
				}
			}// end looping through warmup and real
			delete decoder;
		}//end decode choice if
}// end try
catch (UnrecoverableException &e) {
	std::cout<<e.getSource()<<std::endl;
	std::cout<<e.getMessage()<<std::endl;
	*SessionLogger::logger<<e.getSource()<<"\n";
	*SessionLogger::logger <<e.getMessage()<<"\n";
	return 0;
}
 std::cout<<"Parsing Process Completed"<<std::endl;
  *SessionLogger::logger<<"Parsing Process Completed \n";
return 0;
}
