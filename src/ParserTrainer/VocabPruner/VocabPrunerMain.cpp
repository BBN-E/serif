// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include <cstring>
#include "common/UTF8Token.h"
#include "common/Symbol.h"
#include "common/UnexpectedInputException.h"
#include "parse/VocabularyTable.h"
#include "common/NgramScoreTable.h"
#include "parse/ParserTrainer/VocabPruner.h"
#include "parse/ParserTrainer/PrunerPOS.h"
#include "parse/ParserTrainer/FeatureVectorTable.h"
#include "Generic/common/FeatureModule.h"
#include <boost/scoped_ptr.hpp>

// DIVERSITY CHANGES: diversity tags (such as NPP-NNP and PERSON-NNPS)
// are listed in the open class tags list

int main(int argc, char* argv[]) {
	UTF8Token token;

	if (argc != 19 && argc != 18) {
        cerr << "wrong number of arguments to pruner\n";
		cerr << "Usage:\n";
		cerr << "   7 pairs of input/output files --\n";
		cerr << "     vocab head pre post left right part-of-speech\n";
		cerr << "   unknown word pruning threshold\n";
		cerr << "   feature vector output file \n";
		cerr << "   language\n\n";
		cerr << "   [smooth]\n\n";
		cerr << "(language parameter is new: it should be English, Arabic, etc)\n\n";
		cerr << "if the 'smooth' option is specified, then the program will prune\n";
		cerr << " in such a way as to produce events for use in smoothing\n";

        return -1;
    }


	try {

	// Load the specified language feature module.
	FeatureModule::load(argv[17]);

	// if a 18th parameter exists, we are producing smoothing corpus events
	//   by pruning with the main corpus vocabulary
	bool smooth = (argc == 19);
	if ((argc == 19) && (strcmp(argv[18], "smooth") != 0)) {
		cerr << "If 18th parameter is specified, it must be the string 'smooth'\n";
		return -1;
	}

	VocabPruner* pruner = new VocabPruner(argv[1], 
								atoi(argv[15]), smooth);
	pruner->print_vocab(argv[2]);
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	
	in.open(argv[3]);
	pruner->headCounts = new NgramScoreTable (HEAD_NGRAM_SIZE + 1, in);
	in.close();
	pruner->prune_heads(argv[4]);
	
	in.open(argv[5]);
	pruner->modifierCounts = new NgramScoreTable (MODIFIER_NGRAM_SIZE + 1, in);
	in.close();
	pruner->prune_modifiers(argv[6]);
	
	in.open(argv[7]);
	pruner->modifierCounts = new NgramScoreTable (MODIFIER_NGRAM_SIZE + 1, in);
	in.close();
	pruner->prune_modifiers(argv[8]);
	
	in.open(argv[9]);
	pruner->lexicalCounts = new NgramScoreTable (LEXICAL_NGRAM_SIZE + 1, in);
	in.close();
	pruner->prune_lexical(argv[10]);
	
	in.open(argv[11]);
	pruner->lexicalCounts = new NgramScoreTable (LEXICAL_NGRAM_SIZE + 1, in);
	in.close();
	pruner->prune_lexical(argv[12]);

	in.open(argv[13]);
	PrunerPOS* POS = new PrunerPOS(in, pruner);
	in.close();
	POS->print(argv[14]);

	//use the POS file for feature prune as well
	in.open(argv[13]);
	FeatureVectorTable* feat = new FeatureVectorTable(in,pruner);
	in.close();
	feat->Print(argv[16]);

	// "WordFeature" table is new file from WordCollector

	char headBuf[501];
	char wordFeatInFN[501];
	size_t headsiz = strlen(argv[13]) - 4;
	if (headsiz > 490) headsiz = 490;
	_snprintf(headBuf, headsiz, "%s", argv[13]);
	headBuf[headsiz] = '\0';
	_snprintf(wordFeatInFN, 501, "%s.wordfeat",headBuf);
	//std::cerr<<"Reading word features from "<<wordFeatInFN<<"\n";
	char wordProbOutFN[501];
	_snprintf(wordProbOutFN, 501, "%s.wprob",headBuf);
	//std::cerr<<"Printing word features to "<<wordProbOutFN<<"\n";

	pruner->print_wordprob(wordFeatInFN,wordProbOutFN);  

	} catch (UnexpectedInputException uc) {
		uc.putMessage(std::cerr);
		return -1;

	} catch (std::exception e) {
		std::cerr << "Exception: " << e.what() << std::endl;\
		return -1;
	}




	return 0;
}

