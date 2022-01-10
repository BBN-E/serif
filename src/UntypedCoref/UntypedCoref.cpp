// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

/*
 * Can be run either to train or decode
 * for each document
 *  - load parses and names
 *  - training:
 *    - load coreference information from an independent file
 *    - call LinkAllMentions::create_examples
 *  - decoding:
 *    - load model
 *    - call LinkAllMentions::decode
 *      - for now dump as c_flat
 */
#include "Generic/common/leak_detection.h"

#include <iostream>
#include <string>

#include "common/FileSessionLogger.h"
#include "common/ParamReader.h"
#include "common/version.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/NameTheory.h"
#include "ParseNameReader.h"
#include "CorefReader.h"
#include "Generic/common/UnexpectedInputException.h"

#include "Generic/theories/UTCoref.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/SVMClassifier.h"

/* maximum length of parameter settings */
#define UT_PARAM_MAX 1000


void load_parameter_file(char* parameter_filename);
void decode(const GrowableArray <Parse *> & parses,
			GrowableArray <NameTheory *> & names,
			GrowableArray <TokenSequence *> & tokens);
void train(const GrowableArray <Parse *> & parses,
		   GrowableArray <NameTheory *> & names,
		   GrowableArray <TokenSequence *> &tokens);
void load_parses_and_names(GrowableArray <Parse *> & parses,
						   GrowableArray <NameTheory *> & names,
						   GrowableArray <TokenSequence *> & tokens);

void load_corefs(LinkAllMentions &lam, UTCoref & corefs);

int main(int argc, char* argv[]) {
   GrowableArray <Parse *> parses;
   GrowableArray <NameTheory *> names;
   GrowableArray <TokenSequence *> tokens;

   if (argc == 3) {

	  try {
		 load_parameter_file(argv[2]);

		 load_parses_and_names(parses, names, tokens);

		 string mode = string(argv[1]);
		 if (mode == "train")
			train(parses, names, tokens);
		 else if (mode == "decode")
			decode(parses, names, tokens);
		 else
			std::cerr << "Unknown mode " << mode << " is niether 'train' nor 'decode'" << std::endl;
	  }
	  catch (UnrecoverableException &e) {
		 e.putMessage(std::cerr);
		 std::cerr << std::endl;
	  }
   }
   else {
	  std::cout << "Usage: ./UntypedCoref <mode> <parameter_file>" << std::endl;
	  std::cout << "  mode:            either 'train' or 'decode'" << std::endl;
	  std::cout << "  parameter_file   a serif parameter file" << std::endl;
   }
}

void load_parameter_file(char* parameter_filename) {
   try {
	  ParamReader::readParamFile(parameter_filename);

	  SessionLogger::logger = _new FileSessionLogger("utcoref.log", 0, 0);
	  SessionLogger::logger->beginMessage();
	  *SessionLogger::logger << "UTCoref version " << UT_COREF_VERSION << "\n"
							 << "Serif version: " << SerifVersion::getVersionString() << "\n"
							 << "Serif Language Module: " << SerifVersion::getSerifLanguage().toString() << "\n"
							 << "\n---------------------------------------\n"
							 << "Starting with parameters:\n";
	  ParamReader::logParams();
	  *SessionLogger::logger << "\n";
   }
   catch (UnrecoverableException &e) {
	  e.putMessage(std::cerr);
   }
}

void load_parses_and_names(GrowableArray <Parse *> & parses,
						   GrowableArray <NameTheory *> & names,
						   GrowableArray <TokenSequence *> & tokens) {
   char file_name[UT_PARAM_MAX+1];
   if (ParamReader::getNarrowParam(file_name, Symbol(L"training_file"), UT_PARAM_MAX)) {
	  *SessionLogger::logger << "Training file:\n" << file_name << "\n";
	  ParseNameReader::readAllParses(file_name, parses, names, tokens);
   }
   else {
	  throw UnexpectedInputException("UntypedCoref.cpp::load_parses_and_names",
									 "Param training_file is undefined");
   }
}

void load_corefs(LinkAllMentions &lam, UTCoref & corefs) {
   char file_name[UT_PARAM_MAX+1];
   if (ParamReader::getNarrowParam(file_name, Symbol(L"training_coref_file"), UT_PARAM_MAX)) {
      *SessionLogger::logger << "Training coref file:\n" << file_name << "\n";
	  CorefReader::readAllCorefs(file_name, lam, corefs);
   }
   else {
      throw UnexpectedInputException("UntypedCoref.cpp::load_corefs",
                                     "Param training_coref_file is undefined");
   }
}

void dump_corefs(const GrowableArray <Parse *> & parses, UTCoref & corefs) {
   char file_name[UT_PARAM_MAX+1];
   if (ParamReader::getNarrowParam(file_name, Symbol(L"decoding_coref_file_out"), UT_PARAM_MAX)) {
      *SessionLogger::logger << "Decoding coref file out:\n" << file_name << "\n";
	  CorefReader::writeAllCorefs(file_name, parses, corefs);
   }
   else {
      throw UnexpectedInputException("UntypedCoref.cpp::dump_corefs",
                                     "Param decoding_coref_file_out is undefined");
   }
}

void train(const GrowableArray <Parse *> & parses,
		   GrowableArray <NameTheory *> & names,
		   GrowableArray <TokenSequence *> & tokens) {

   UTCoref corefs;
   LinkAllMentions lam(&parses, &names, &tokens);
   lam.init_linkable_nodes();

   load_corefs(lam, corefs);
   lam.create_examples(corefs);
}

void decode(const GrowableArray <Parse *> & parses,
			GrowableArray <NameTheory *> & names,
			GrowableArray <TokenSequence *> & tokens) {

   UTCoref corefs;
   LinkAllMentions lam(&parses, &names, &tokens);
   lam.init_linkable_nodes();

   lam.decode(corefs);
   dump_corefs(parses, corefs);
}
