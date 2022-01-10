// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "names/IdFTrainer.h"
#include "names/IdFDecoder.h"
#include "common/UnrecoverableException.h"
#include "common/ParamReader.h"
#include "names/IdFSentence.h"
#include "names/NameClassTags.h"
#include "names/IdFWordFeatures.h"
#include "theories/EntityType.h"
#include "Generic/common/version.h"
#include "Generic/common/FeatureModule.h"
#include <time.h>
#include <boost/scoped_ptr.hpp>


// usage message
int print_usage() {
	std::cerr << "\nTrainer usage:\n";
	std::cerr << "-t -SERIF/-simple/-complex name_class_file ";
	std::cerr << "training_file_list model_file_prefix [-language language] ";
	std::cerr << "[-names|-timex|-other-values] ";
	std::cerr << "[-lists lists_file] [-smooth smoothing_file] [-kvalues # # # # #]\n";
	std::cerr << "\nThe smoothing and k-value parameters are for use only by someone";
	std::cerr << " very familiar with the code!\n";
	std::cerr << "\nThis executable also serves as a stand-alone decoder. Decoder usage:\n";
	std::cerr << "-d -SERIF/-simple/-complex name_class_file ";
	std::cerr << "model_file_prefix input_file output_file [-language language] ";
	std::cerr << "[-nbest] [-names|-timex|-other-values] [-lists lists_file] ";
	std::cerr << "[-prune forward_pruning_threshold forward_beam_width]\n";
	std::cerr << "\nGiven a name class parameter and a training file X, this executable will\n";
	std::cerr << "produce X.msg and X.key, which can be used to run tests with the MUC scorer.\n";
	std::cerr << "Usage: -m -SERIF/-simple/-complex name_class_file training_file [-language language]\n";
	std::cerr << "\nNOTES:\n";
	std::cerr << "You must use -language to specify a language name (eg English)\n";
	std::cerr << "Optional arguments are indicated by flags, as shown above in [], and they\n";
	std::cerr << " must be placed after the required arguments. After that, however, they can be\n";
	std::cerr << " in any order.\n";
	std::cerr << "\n* A name file is always required, and it can be indicated in one of three ways:\n";
	std::cerr << "  * SERIF: through a SERIF parameter file, using parameter entity-type-set\n";
	std::cerr << "  * simple:\n";
	std::cerr << "      2\n";
	std::cerr << "      PER\n";
	std::cerr << "      DATE\n";
	std::cerr << "  * complex:\n";
	std::cerr << "      2\n";
	std::cerr << "      PERSON|<ENAMEX TYPE=\"PER\">|</ENAMEX>\n";
	std::cerr << "      DATE|<TIMEX TYPE=\"DATE\">|</TIMEX>\n\n";
	std::cerr << "\nIf you use a list file, it must be the same in decode and training!\n";

	return -1;
}

int main(int argc, char* argv[])
{
	// NOTE:
	//
	// This executable also serves as a stand-alone decoder. I don't want to have 
	// another workspace for a stand-alone decoder, but it doesn't make sense to 
	// have another project cluttering up the SERIF workspace. So, despite the
	// name of this executable, it can also be used to decode a text file,
	// a capability not needed for SERIF, but sharing the same codebase.
	// See usage message for details.
	//
	// ALSO: given a training file, it can also produce a message and 
	// key file that will enable you to run the MUC scorer

	//
	// SOME FILE FORMAT EXAMPLES:
	//

	// sample_training_file_list:
	// 3
	// filename1
	// filename2
	// filename3
	//
	// NOTE: there must be at least 2 files in the list!

	// sample_name_class_file:
	// 3
	// PERSON
	// ORGANIZATION
	// LOCATION
	//
	// USE_SERIF means it will use the name classes from EntityType

	// sample_training_file
 	// ((the NONE-ST)(United ORG-ST)(Nations ORG-CO)(succeeded NONE-ST)(. NONE-CO))
	// ((this NONE-ST)(is NONE-CO)(a NONE-CO)(sample NONE-CO))
	//
	// -ST = start, -CO = continue

	// sample_input_file (to be decoded) -- note that it is pre-tokenized!!!
	// (this is a sample)
	// (the United Nations succeeded .)

	// sample lists_file:
	// lots-of-person-names.list
	// lots-of-gpe-names.list

	// sample list:
	// (Bob)
	// (Joe)
	// (Mary Carol)
	
	time_t stime;
	time( &stime );


	// assign all variables to their correct positions
	char* language = 0;
	bool useSerif = false;
	char* training_file_list = 0;
	char* model_file_prefix = 0; 
	char* smoothing_file = 0;
	char* lists_file = 0;
	char* training_file = 0;
	char* input_file = 0;
	char* output_file = 0;
	bool setPruneParams = false;
	bool setKValues = false;
	bool useNBest = false;
	float kvalues[5];
	float forward_pruning_threshold = 0;
	int forward_beam_width = 0;
	int word_features_mode = IdFWordFeatures::DEFAULT;

	enum run_mode{train, decode, muc};
	run_mode mode;
	if (argc < 2)
		return print_usage();

	if (strcmp(argv[1], "-t") == 0)
		mode = train;
	else if (strcmp(argv[1], "-d") == 0)
		mode = decode;
	else if (strcmp(argv[1], "-m") == 0)
		mode = muc;
	else return print_usage();

	if (mode == train || mode == decode) {
		int flag_start = 0;
		if (mode == train) {
			if (argc < 6) return print_usage();
			training_file_list = argv[4];
			model_file_prefix = argv[5];
			flag_start = 6;
		} else {
			if (argc < 7) return print_usage();
			model_file_prefix = argv[4];
			input_file = argv[5];
			output_file = argv[6];
			flag_start = 7;
		}
		for (int i = flag_start; i < argc; ) {
			if (strcmp(argv[i],"-language") == 0) {
				if (i+1 == argc)
					return print_usage();
				else language = argv[i+1];
				i+=2;
			} else if (strcmp(argv[i],"-smooth") == 0) {
				if (i+1 == argc)
					return print_usage();
				else smoothing_file = argv[i+1];
				i+=2;
			} else if (strcmp(argv[i],"-nbest") == 0) {
				useNBest = true;
				i+=1;
			} else if (strcmp(argv[i],"-lists") == 0) {
				if (i+1 == argc)
					return print_usage();
				else lists_file = argv[i+1];
				i+=2;
			} else if (strcmp(argv[i],"-prune") == 0) {
				if (i+1 == argc || i+2 == argc)
					return print_usage();
				else setPruneParams = true;
                forward_pruning_threshold = (float) atof(argv[i+1]);
				forward_beam_width = atoi(argv[i+2]);
				if (mode == train)
					std::cerr << "pruning threshold is irrelevant in training, ignoring it\n";
				i+=3;
			} else if (strcmp(argv[i],"-kvalues") == 0) {
				setKValues = true;
				for (int z = 0; z < 5; z++) {
					if (i+z+1 == argc)
						return print_usage();
					kvalues[z] = (float) atof(argv[i+z+1]);
				}
				if (mode == decode)
					std::cerr << "k values are irrelevant in training, ignoring them\n";
				i+=6;
			} else if (strcmp(argv[i], "-names") == 0) {
				word_features_mode = IdFWordFeatures::DEFAULT;
				i+=1;
			} else if (strcmp(argv[i], "-timex") == 0) {
				word_features_mode = IdFWordFeatures::TIMEX;
				i+=1;
			} else if (strcmp(argv[i], "-other-values") == 0) {
				word_features_mode = IdFWordFeatures::OTHER_VALUE;
				i+=1;
			} else return print_usage();
		}
	} else if (mode == muc) {
		if ((argc != 7) || (strcmp(argv[5],"-language") != 0))
			return print_usage();
		training_file = argv[4];
		language = argv[6];
	} 
	if (language == 0)
		return print_usage();
	try {
		// Load the specified language feature module.
		FeatureModule::load(language);

		NameClassTags *nameClassTags;
		if (strcmp(argv[2], "-SERIF") == 0) {
			ParamReader::readParamFile(argv[3]);
			std::cerr << "Initializing with SERIF name classes from ";
			char file_name[100];
			ParamReader::getNarrowParam(file_name, Symbol(L"entity_type_set"), 100);
			std::cerr << file_name << ".\n\n";
			nameClassTags = new NameClassTags();
		} else if (strcmp(argv[2], "-simple") == 0) {
			std::cerr << "Initializing with simple name classes from ";
			std::cerr << argv[3] << ".\n\n";
			nameClassTags = new NameClassTags(argv[3], false);
		} else if (strcmp(argv[2], "-complex") == 0) {
			std::cerr << "Initializing with complex name classes from ";
			std::cerr << argv[3] << ".\n\n";
			nameClassTags = new NameClassTags(argv[3], true);
		} else return print_usage();

		if (mode == train) {
			std::cerr << "TRAINING off of " << training_file_list << "\n";
			IdFTrainer *trainer = new IdFTrainer(nameClassTags, lists_file, word_features_mode);
			trainer->collectCountsFromFiles(training_file_list);

			// CHINESE is soooo different from the other languages...
			if (SerifVersion::isChinese()) {
//#ifdef CHINESE_LANGUAGE
				trainer->setKValues(4, 5, 3, 9, 9);
//#endif
			}
			if (setKValues) {
				trainer->setKValues(kvalues[0], kvalues[1], kvalues[2], kvalues[3], kvalues[4]);
				std::cerr << "Using k values: ";
				std::cerr << kvalues[0] << " ";
				std::cerr << kvalues[1] << " ";
				std::cerr << kvalues[2] << " ";
				std::cerr << kvalues[3] << " ";
				std::cerr << kvalues[4] << "\n";
			}

			trainer->deriveTables();
			if (smoothing_file != 0) {
				std::cerr << "\nSMOOTHING with " << smoothing_file << "\n";
				trainer->estimateLambdas(smoothing_file);
			}
			std::cerr << "\nPRINTING to " << model_file_prefix << "\n";
			trainer->printTables(model_file_prefix);
		} else if (mode == decode) {

			IdFDecoder *decoder = new IdFDecoder(model_file_prefix, nameClassTags, 
												lists_file, word_features_mode);
			if (setPruneParams) {
				decoder->setForwardPruningThreshold(forward_pruning_threshold);
				decoder->setForwardBeamWidth(forward_beam_width);
			}			
			
			time_t ttime;
			time( &ttime );
			time_t t = ttime - stime;
			std::cerr << "\n";
			cout << ((long) t / 60) << " minutes " << ((long) t % 60) << " seconds model loading time\n";

			std::cerr << "DECODING " << input_file;
			std::cerr << ", with training from " << model_file_prefix;
			std::cerr << "\nPrinting results to " << output_file << "\n";

			if (useNBest) {
				decoder->decodeNBest(input_file, output_file);
			} else decoder->decode(input_file, output_file);

			time_t t2time;
			time( &t2time );
			t = t2time - ttime;
			std::cerr << "\n";
			cout << ((long) t / 60) << " minutes " << ((long) t % 60) << " seconds decoding time\n";

		} else if (mode == muc) {
			std::cerr << "CREATING " << training_file << ".msg and " << training_file << ".key";
			boost::scoped_ptr<UTF8InputStream> inStream_scoped_ptr(UTF8InputStream::build());
			UTF8InputStream& inStream(*inStream_scoped_ptr);
			inStream.open(training_file);
			UTF8OutputStream sentStream;
			UTF8OutputStream keyStream;
			char buffer[500];
			sprintf(buffer, "%s.msg", training_file);
			sentStream.open(buffer);
			sprintf(buffer, "%s.key", training_file);
			keyStream.open(buffer);
			IdFSentence *sentence = new IdFSentence(nameClassTags);
			keyStream << L"<DOC>\n<DOCID>0</DOCID>\n";
			while (sentence->readTrainingSentence(inStream)) {
				sentStream << L"(" << sentence->to_just_tokens_string() << L")\n";
				keyStream << sentence->to_enamex_sgml_string() << "\n\n";
			}
			keyStream << L"</DOC>\n";

			sentStream.close();
			keyStream.close();
		}
		
	} catch (UnrecoverableException uc) {
		
		uc.putMessage(std::cerr);
		return -1;
		
	} 
	
	time_t etime;
	time( &etime );
	time_t t = etime - stime;
	std::cerr << "\n";
	cout << "TOTAL TIME: " << ((long)t / 60) << " minutes " << ((long)t % 60) << " seconds\n";


	return 0;

}


