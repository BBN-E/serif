// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstring>
#include <fstream>
#include "math.h"
#include <stdlib.h>
#include "Generic/parse/ChartDecoder.h"
#include "Generic/parse/BridgeExtension.h"
#include "Generic/parse/BridgeKernel.h"
#include "Generic/parse/KernelKey.h"
#include "Generic/parse/ExtensionKey.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Entity.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "dynamic_includes/parse/ParserConfig.h"
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

const size_t ChartDecoder::maxSentenceLength = MAX_SENTENCE_LENGTH;
const size_t ChartDecoder::maxTagsPerWord = MAX_TAGS_PER_WORD;

// ripped this off from gsl library
static int
__fcmp (const double x1, const double x2, const double epsilon = 1.e-6)
{
  int exponent;
  double delta, difference;

  /* Find exponent of largest absolute value */

  {
    double max = (fabs (x1) > fabs (x2)) ? x1 : x2;

    frexp (max, &exponent);
  }

  /* Form a neighborhood of size  2 * delta */

  delta = ldexp (epsilon, exponent);

  difference = x1 - x2;

  if (difference > delta)       /* x1 > x2 */
    {
      return 1;
    }
  else if (difference < -delta) /* x1 < x2 */
    {
      return -1;
    }
  else                          /* -delta <= difference <= delta */
    {
      return 0;                 /* x1 ~=~ x2 */
    }
}


ChartDecoder::ChartDecoder(const KernelTable* kernelTableArg,
                           const ExtensionTable* extensionTableArg,
                           const PriorProbTable* priorProbTableArg,
                           HeadProbs* headProbsArg,
                           ModifierProbs* premodProbsArg,
                           ModifierProbs* postmodProbsArg,
                           LexicalProbs* leftLexicalProbsArg,
                           LexicalProbs* rightLexicalProbsArg,
                           const PartOfSpeechTable* partOfSpeechTableArg,
                           const VocabularyTable* vocabularyTableArg,
                           const NgramScoreTable* featTableArg,
                           const SequentialBigrams* bigrams,
                           SignificantConstitOracle* scOracleArg, 
                           CacheType cacheType, 
                           long cacheMax,
			   const PartOfSpeechTable* auxPOSTable,
			   const Symbol* restrictedPOSTags, 
			   int restrictedPOSTagsSize,
			   const TokenTagTable* parserShortcutsArg,
			   bool useLowerCaseForUnknown,
			   bool constrainKnownNounsAndVerbsParam,
			   float lambda, 
			   int maxEntriesPerCell)
  : DECODER_TYPE(MIXED),
    kernelTable(kernelTableArg), 
    extensionTable(extensionTableArg),
    priorProbTable(priorProbTableArg), 
    cache_max(cacheMax),
    simpleCache(cacheMax),
#if !defined(_WIN32) && !defined(__APPLE_CC__)
    lruCache(cache_max),
#endif
    cache_type(cacheType),
    headProbs(headProbsArg),
    premodProbs(premodProbsArg), 
    postmodProbs(postmodProbsArg),
    leftLexicalProbs(leftLexicalProbsArg),
    rightLexicalProbs(rightLexicalProbsArg),
    partOfSpeechTable(partOfSpeechTableArg),
    vocabularyTable(vocabularyTableArg),
    featTable(featTableArg),
    sequentialBigrams(bigrams),
    scOracle(scOracleArg),
	wordFeatures(WordFeatures::build()),
	lambda(lambda),
	maxEntriesPerCell(maxEntriesPerCell),
	theory_scores(_new float[maxEntriesPerCell]),
	theory_sc_strings(_new string[maxEntriesPerCell]),
	theories(_new ChartEntry*[maxEntriesPerCell])
{
	wordProbTable = _new NgramScoreTable(1, 500);
	for (size_t i = 0; i < maxSentenceLength; i++) {
        chart[i][i] = _new ChartEntry*[maxTagsPerWord + 1];
		chart[i][i][0] = 0;
    }
    for (size_t j = 0; j < maxSentenceLength - 1; j++) {
		for (size_t k = j + 1; k < maxSentenceLength; k++) {
            chart[j][k] = _new ChartEntry*[maxEntriesPerCell + 1];
			chart[j][k][0] = 0;
        }
    }
	auxPartOfSpeechTable = auxPOSTable; // may be empty
	restrictedPosTags = restrictedPOSTags; // default is empty Symbol array
	restrictedPosTagsSize = restrictedPOSTagsSize; 
	parserShortcuts = parserShortcutsArg; // may be empty
	lowerCaseForUnknown = useLowerCaseForUnknown; //default is false
	constrainKnownNounsAndVerbs = constrainKnownNounsAndVerbsParam; //default is false

	SessionLogger::dbg("chart_decoder") << "ChartDecoder::ChartDecoder:  Using MAX_ENTRIES_PER_CELL of "
		<< maxEntriesPerCell << " and lambda of " << lambda << "\n";
}

ChartDecoder::~ChartDecoder() {
	delete[] theory_scores;
	delete[] theory_sc_strings;
	delete[] theories;
	delete wordProbTable;
	if (kernelTable != 0)          { delete kernelTable; }
	if (extensionTable != 0)       { delete extensionTable; }
	if (priorProbTable != 0)       { delete priorProbTable; }
	if (headProbs != 0)            { delete headProbs; }
	if (premodProbs != 0)          { delete premodProbs; }
	if (postmodProbs != 0)         { delete postmodProbs; }
	if (leftLexicalProbs != 0)     { delete leftLexicalProbs; }
	if (rightLexicalProbs != 0)    { delete rightLexicalProbs; }
	if (partOfSpeechTable != 0)    { delete partOfSpeechTable; }
	if (vocabularyTable != 0)      { delete vocabularyTable; }
	if (featTable != 0)            { delete featTable; }
	if (sequentialBigrams != 0)    { delete sequentialBigrams; }
	if (scOracle  != 0)            { delete scOracle ; }
	if (restrictedPosTags  != 0)   { delete[] restrictedPosTags ; }
	if (parserShortcuts != 0)      { delete parserShortcuts; }
    for (size_t j = 0; j < maxSentenceLength - 1; j++) {
		for (size_t k = j; k < maxSentenceLength; k++) {
            delete[] chart[j][k];
        }
    }

}

ChartDecoder::ChartDecoder(const char* model_prefix, double frag_prob){
 // for compatibility with callers not using the aux POS table
	PartOfSpeechTable* _auxPosTable = _new PartOfSpeechTable();
    new(this) ChartDecoder(model_prefix, frag_prob, _auxPosTable);
}
ChartDecoder::ChartDecoder(const char* model_prefix, double frag_prob, const PartOfSpeechTable* auxPOSTable) : 
	_frag_prob(frag_prob), DECODER_TYPE(MIXED)
{

	//paramStream >> kernelFile;

	CacheType cacheType = None;
	std::string buffer = ParamReader::getParam("probs_cache_type");
	if (buffer == "simple") {
		cacheType = Simple;
#if !defined(_WIN32) && !defined(__APPLE_CC__)
		// Not currently supported for Windows; see note in Generic/common/lru_cache.h
	} else if (buffer == "lru") {
		cacheType = Lru;
#endif
	}
	long cacheMax = 1000;
	buffer = ParamReader::getParam("probs_cache_max_k_entries");
	if (!buffer.empty()) {
		cacheMax = atol(buffer.c_str()) * 1000;
	}


	// wordPropTable gets allocated in the constructor that we call with placement new.
	//wordProbTable = new NgramScoreTable(1, 500);
	//ifstream paramStream;
	//paramStream.open(paramFile);
	

	std::string model_prefix_str(model_prefix);

	boost::scoped_ptr<UTF8InputStream> kernelStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& kernelStream(*kernelStream_scoped_ptr);
	buffer = model_prefix_str + ".kernels";
	kernelStream.open(buffer.c_str());
	KernelTable* kernelTable = _new KernelTable(kernelStream);
	kernelStream.close();
	
	boost::scoped_ptr<UTF8InputStream> extensionStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& extensionStream(*extensionStream_scoped_ptr);
	buffer = model_prefix_str + ".extensions";
	extensionStream.open(buffer.c_str());
	ExtensionTable* extensionTable = _new ExtensionTable(extensionStream);
	extensionStream.close();
	
	boost::scoped_ptr<UTF8InputStream> priorProbStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& priorProbStream(*priorProbStream_scoped_ptr);
	buffer = model_prefix_str + ".prior";
	priorProbStream.open(buffer.c_str());
	PriorProbTable* priorProbTable = _new PriorProbTable(priorProbStream);
	priorProbStream.close();
	
	boost::scoped_ptr<UTF8InputStream> headProbStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& headProbStream(*headProbStream_scoped_ptr);
	buffer = model_prefix_str + ".head";
	headProbStream.open(buffer.c_str());
	HeadProbs* headProbs = _new HeadProbs(headProbStream, cacheType, cacheMax);
	headProbStream.close();
	
	boost::scoped_ptr<UTF8InputStream> premodProbStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& premodProbStream(*premodProbStream_scoped_ptr);
	buffer = model_prefix_str + ".pre";
	premodProbStream.open(buffer.c_str());
	ModifierProbs* premodProbs = _new ModifierProbs(premodProbStream, cacheType, cacheMax, "pre");
	premodProbStream.close();
	
	boost::scoped_ptr<UTF8InputStream> postmodProbStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& postmodProbStream(*postmodProbStream_scoped_ptr);
	buffer = model_prefix_str + ".post";
	postmodProbStream.open(buffer.c_str());
    ModifierProbs* postmodProbs = _new ModifierProbs(postmodProbStream, cacheType, cacheMax, "post");
    postmodProbStream.close();
		
    boost::scoped_ptr<UTF8InputStream> leftLexicalProbStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& leftLexicalProbStream(*leftLexicalProbStream_scoped_ptr);
	buffer = model_prefix_str + ".left";
	leftLexicalProbStream.open(buffer.c_str());
    LexicalProbs* leftLexicalProbs = _new LexicalProbs(leftLexicalProbStream, cacheType, cacheMax, "left");
    leftLexicalProbStream.close();
		
    boost::scoped_ptr<UTF8InputStream> rightLexicalProbStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& rightLexicalProbStream(*rightLexicalProbStream_scoped_ptr);
	buffer = model_prefix_str + ".right";
    rightLexicalProbStream.open(buffer.c_str());
    LexicalProbs* rightLexicalProbs = _new LexicalProbs(rightLexicalProbStream, cacheType, cacheMax, "right");
    rightLexicalProbStream.close();
		
    boost::scoped_ptr<UTF8InputStream> posStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& posStream(*posStream_scoped_ptr);
	buffer = model_prefix_str + ".pos";
    posStream.open(buffer.c_str());
    PartOfSpeechTable* partOfSpeechTable = _new PartOfSpeechTable(posStream);
    posStream.close();

    boost::scoped_ptr<UTF8InputStream> vocabularyStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& vocabularyStream(*vocabularyStream_scoped_ptr);
	buffer = model_prefix_str + ".voc";
    vocabularyStream.open(buffer.c_str());
    VocabularyTable* vocabularyTable = _new VocabularyTable(vocabularyStream);
    vocabularyStream.close();
	//mrf- add featureTable
	//The featureTable is used to put the log 1/# of different words that had
	//a given Feature set in training as the initial score for a word entry.
	//If the feature Table is empty, the scores will be 0 (as they were initially)
    boost::scoped_ptr<UTF8InputStream> featStream_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& featStream(*featStream_scoped_ptr);
	NgramScoreTable* featTable;
	if(ParamReader::isParamTrue("feature_adjusted_parse")) {
		buffer = model_prefix_str + ".feat";
		featStream.open(buffer.c_str());
		featTable = _new NgramScoreTable(1, featStream);
		featStream.close();

	}
	else{
		featTable = _new NgramScoreTable(1,20);
	}

	SequentialBigrams* bigrams;	
	//mrf - make bigram file its own parameter
	buffer = ParamReader::getParam("bigrams");
	if (!buffer.empty()) {
		bigrams = _new SequentialBigrams(buffer.c_str());
	}
	else{	//if there isn't a bigrams file
		bigrams = _new SequentialBigrams();
	}
	ChartEntry::set_sequentialBigrams(bigrams);

	std::string inventory = ParamReader::getParam("inventory");
	const char *inventory_char = 0;
	if (!inventory.empty())
		inventory_char = inventory.c_str();
	SignificantConstitOracle * scOracle;
	scOracle = SignificantConstitOracle::build(kernelTable, extensionTable, inventory_char);	

	int maxsec = ParamReader::getOptionalIntParamWithDefaultValue("max_parser_seconds", 100*60);
	MAX_CLOCKS = maxsec * CLOCKS_PER_SEC;

	Symbol *restrictedPOSTags = _new Symbol[50];
	int restrictedPOSTagsSize = (int)ParamReader::getSymbolArrayParam("unknown_pos_tags", restrictedPOSTags, 50);


	TokenTagTable* parserShortcutsArg;
	std::string shortcuts = ParamReader::getParam("parser_shortcuts");
	if(!shortcuts.empty()) {
		boost::scoped_ptr<UTF8InputStream> shortcutsStream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& shortcutsStream(*shortcutsStream_scoped_ptr);
		shortcutsStream.open(shortcuts.c_str());
		parserShortcutsArg = _new TokenTagTable(shortcutsStream);
		shortcutsStream.close();
	}else{
		parserShortcutsArg = _new TokenTagTable();
	}

	bool useLowerCaseForUnknown = ParamReader::getOptionalTrueFalseParamWithDefaultVal("lower_case_for_unknown", false);

	bool constrainKnownNounsAndVerbsParam = ParamReader::getOptionalTrueFalseParamWithDefaultVal("constrain_known_nouns_and_verbs", false);
	
	// For "fast parsing" use lambda=-1.
	float lambda = static_cast<float>(ParamReader::getOptionalFloatParamWithDefaultValue("parser_lambda", -5));

	// For "fast parsing" use max_entries_per_cell=5.
	int max_entries_per_cell = ParamReader::getOptionalIntParamWithDefaultValue("parser_max_entries_per_cell", 10);

	// Use placement-new to initialize self (this may not be a good way to do this)
	new(this) ChartDecoder(kernelTable, extensionTable, priorProbTable,
		headProbs, premodProbs, postmodProbs, leftLexicalProbs,
		rightLexicalProbs, partOfSpeechTable, vocabularyTable, featTable, 
		bigrams, scOracle, cacheType, cacheMax, 
		auxPOSTable, restrictedPOSTags, restrictedPOSTagsSize, 
	    parserShortcutsArg, useLowerCaseForUnknown, constrainKnownNounsAndVerbsParam,
		lambda, max_entries_per_cell);
		
}

/** Does this method ever get called?? */
void ChartDecoder::readWordProbTable(const char* model_prefix){
	delete wordProbTable;
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	std::string model_prefix_str(model_prefix);
	std::string buffer = model_prefix_str + ".voc.wordprob";
	in.open(buffer.c_str());
	wordProbTable =  new NgramScoreTable(1, in);

}

ParseNode* ChartDecoder::decode(Symbol* sentence, int length,
    std::vector<Constraint> & constraints,
	bool collapseNPlabels, Symbol* pos_constraints)
{
	// Use NULL tokenSequence -- this POS sequence will just get thrown
	// away after we decode, and we'll never access its token sequence.
	PartOfSpeechSequence tempPOS(NULL, 0, length);
	if(pos_constraints != 0){
		for(int i =0; i< length; i++){
			tempPOS.addPOS(pos_constraints[i], 1, i);
		}
	}
	ParseNode* result = decode(sentence, length, constraints, collapseNPlabels, &tempPOS);
	return result;
}

ParseNode* ChartDecoder::returnDefaultParse(Symbol* sentence, int length, std::vector<Constraint> & constraints, bool collapseNPlabels)
{
	ParseNode *defaultParse = getDefaultParse(sentence, length, constraints);
	highest_scoring_final_theory = 0;
	theory_scores[0] = 0;
	int replacementPosition = 0;
	replaceWords(defaultParse, replacementPosition, sentence);
	postprocessParse(defaultParse, constraints, collapseNPlabels);
	cleanupChart(length);
	highest_scoring_final_theory = 0;
	return defaultParse;
}

ParseNode* ChartDecoder::decode(Symbol* sentence, int length,
		std::vector<Constraint> & constraints,
		bool collapseNPlabels,  const PartOfSpeechSequence* pos_constraints)
{
	highest_scoring_final_theory = 0; // this needs to be set before we return!!

	// AZ 4/1/05: There is an obscure bug where if the first sentence of a document 
	// is one token long, and the token in OOV, the parser sometimes crashes. 
	// This routine handles one token sentences without trying to parse.
	if (length == 1) 
		return returnDefaultParse(sentence, length, constraints, collapseNPlabels);

	//mrf 10/2007 sentences that are mostly punctuation cause strange parses.  
	//These can cause stack overflows in during proposition finding.  
	//Create a default default parse for these sentences.  
	double word_bigram_ratio = 1;
	if(length > 40){
		bool prev_is_word = true;
		bool is_word;
		int n_word_bigrams = 0;
		for(int wnum = 0; wnum < length; wnum++){
			const Symbol &word = sentence[wnum];
			is_word = false;
			for(size_t cnum = 0; (cnum < wcslen(word.to_string()) && !is_word); cnum++){
				if(!(iswpunct(word.to_string()[cnum]))){
					is_word = true;
				} else if(word.to_string()[cnum] == ','){ //mrf- comma's suggest a list, and we might want to include long lists of names
					is_word = true;
				}
			}
			if(is_word && prev_is_word){
				n_word_bigrams++;
			}
			prev_is_word = is_word;
		}
		word_bigram_ratio = ((double)n_word_bigrams) / length;
	}
	if(word_bigram_ratio < .1){
		SessionLogger::info("SERIF") << "Flattening parse for long sentence with mostly punctuation. Length: " << length << " word_bigram_ratio: "<<word_bigram_ratio<<"\n";
		for(int i = 0; i< length; i++){
			SessionLogger::info("SERIF") <<sentence[i].to_debug_string()<<" ";
		}

		ParseNode *defaultParse = getCompletelyDefaultParse(sentence, length);
		highest_scoring_final_theory = 0;
		theory_scores[0] = 0;
		int replacementPosition = 0;
		replaceWords(defaultParse, replacementPosition, sentence);
		postprocessParse(defaultParse, constraints, collapseNPlabels);
		cleanupChart(length);
		highest_scoring_final_theory = 0;
		return defaultParse;
	}
	//end punctuation only check

	startClock();
	
	bool lastIsPunct = LanguageSpecificFunctions::isSentenceEndingPunctuation(sentence[length - 1]);

	initPunctuationUpperBound(sentence, length);

	initChart(sentence, length, constraints, pos_constraints);

	bool blockPrepsFlag = (parserShortcuts->lookup(Symbol(L"PURE-PREPS")) == Symbol(L"NRC"));
	bool blockAdverbsFlag = (parserShortcuts->lookup(Symbol(L"PURE-ADVERBS")) == Symbol(L"NRC"));

	for (int nc=0; nc<length; nc++){
		nonLeftClosableToken[nc] = false;
		nonRightClosableToken[nc] = false;

		const Symbol &word = sentence[nc];

		Symbol shortcut = parserShortcuts->lookup(word);
		bool inNLC = (shortcut == Symbol(L"NLC"));
		bool inNRC = (shortcut == Symbol(L"NRC"));

		// all the other issues about punctuation and sentence boundaries seem not to matter for NLC
		if (inNLC && 
			nc > 1 && 
			!LanguageSpecificFunctions::isNoCrossPunctuation(sentence[nc-1])) 
				nonLeftClosableToken[nc] = true;


		if ((int)punctuationUpperBound[nc] <= nc+1) {
			continue;
		}
		if (inNRC || 
			(blockPrepsFlag && chartHasOnlyGeneralPrepositionPOS(nc)) ||
			(blockAdverbsFlag && chartHasOnlyAdverbPOS(nc))){
			if (chartHasParticlePOS(nc)) {
				// if we have a particle and context then allow closure
				if ((nc > 0) && chartHasVerbPOS(nc-1)){
					continue;
				}else if ((nc > 1) &&
							chartHasPronounPOS(nc-1) &&
							chartHasVerbPOS(nc-2)){
					continue;
				}
			}
			nonRightClosableToken[nc] = true;
		}
	}

	for (int span = 2; span <= length; span++) {
		for (int start = 0; start <= (length - span); start++) {
			if (timedOut(sentence, length)) {
				ParseNode *defaultParse = getDefaultParse(sentence, length, constraints);
				highest_scoring_final_theory = 0;
				theory_scores[0] = 0;
				int replacementPosition = 0;
				replaceWords(defaultParse, replacementPosition, sentence);
				postprocessParse(defaultParse, constraints, collapseNPlabels);
				cleanupChart(length);
				highest_scoring_final_theory = 0;
				return defaultParse;
			}
			int end = start + span;


			numTheories = 0;
			for (int mid = (start + 1); mid < end; mid++) {
				if ((end == length) && lastIsPunct &&
					!((mid == (length - 1)) && (start == 0))) {
					continue;
				}
				if (punctuationCrossing(start, end, length)) {
					continue;
				}
				leftClosable = !crossingConstraintViolation(start, mid, constraints) && !nonLeftClosableToken[start];
				rightClosable =  !crossingConstraintViolation(mid, end, constraints) && !nonRightClosableToken[end-1];
				for (ChartEntry** leftEntry = chart[start][mid - 1];
				*leftEntry; ++leftEntry)
				{
					for (ChartEntry** rightEntry = chart[mid][end - 1];
					*rightEntry; ++rightEntry)
					{
						addKernelTheories(*leftEntry, *rightEntry);
						addExtensionTheories(*leftEntry, *rightEntry);
					}
				}
			}
			transferTheoriesToChart(start, end);
		}
	}
	float finalScore;

	ParseNode* returnValue = getBestParse(finalScore, 0, length, false);
	if (returnValue == 0){ 
		returnValue = getDefaultParse(sentence, length, constraints);
		highest_scoring_final_theory = 0;
	}
	if (length > 40) {
		int depth = getTreeDepth(returnValue);
		if ((float) depth / length > 0.8F) { 
			SessionLogger::info("SERIF") << "Flattening parse for very deep, long sentence. Depth: " << depth << " Length: " << length << "\n";
			returnValue = getCompletelyDefaultParse(sentence, length);
			highest_scoring_final_theory = 0;
		}
		else if (((float) depth / length > 0.7F) && (word_bigram_ratio < .4 ) ){ 
			SessionLogger::info("SERIF") << "Flattening parse for deep, long sentence, with mostly punctuation. Depth: " << depth << " Length: " 
				<< length <<" WordBigramRatio: "<<word_bigram_ratio<< "\n";
			returnValue = getCompletelyDefaultParse(sentence, length);
			highest_scoring_final_theory = 0;
		}
	}

	// diversity parsing iteration
	ParseNode* iterReturnValue = returnValue;
	while (iterReturnValue != 0) {
		int replacementPosition = 0;
		replaceWords(iterReturnValue, replacementPosition, sentence);
		postprocessParse(iterReturnValue, constraints, collapseNPlabels);
		iterReturnValue = iterReturnValue->next;
	}

	cleanupChart(length);

	return returnValue;
		
}


void ChartDecoder::addKernelTheories(ChartEntry* leftEntry,
    ChartEntry* rightEntry)
{
	if (leftClosable && rightClosable)
	{
		KernelKey key1(BRANCH_DIRECTION_RIGHT, leftEntry->constituentCategory,
			rightEntry->constituentCategory, rightEntry->headTag);
		int numKernels;
		BridgeKernel* kernels = kernelTable->lookup(key1, numKernels);
		for (int i = 0; i < numKernels; i++) {
			bool left = scOracle->isSignificant(leftEntry, kernels[i].headChain);
			bool right = scOracle->isSignificant(rightEntry, kernels[i].modifierChain);
			SignificantConstitNode *significantConstitNode = 
				_new SignificantConstitNode(left, leftEntry->significantConstitNode, 
				leftEntry->leftToken, leftEntry->rightToken, 
				right, rightEntry->significantConstitNode, 
				rightEntry->leftToken, rightEntry->rightToken);
			ChartEntry* entry = _new ChartEntry(
			    /*constituentCategory =*/ kernels[i].constituentCategory,
			    /*headConstituent =*/ kernels[i].headChainFront,
			    /*headWord =*/ leftEntry->headWord,
			    /*headIsSignificant =*/ leftEntry->headIsSignificant,
			    /*headTag =*/ leftEntry->headTag,
			    /*leftEdge =*/ ParserTags::adjSymbol,
			    /*leftTag =*/ leftEntry->headTag,
			    /*leftWord =*/ leftEntry->headWord,
			    /*rightEdge =*/ kernels[i].modifierChainFront,
			    /*rightTag =*/ rightEntry->headTag,
			    /*rightWord =*/ rightEntry->headWord,
			    /*leftChild =*/ leftEntry,
			    /*rightChild =*/ rightEntry,
			    /*leftToken =*/ leftEntry->leftToken,
			    /*rightToken =*/ rightEntry->rightToken,
			    /*nameType =*/ ParserTags::nullSymbol,
			    /*SignificantConstitNode =*/ significantConstitNode,
			    // for PP attachment:
			    /*isPPofSignificantConstit =*/ (LanguageSpecificFunctions::isPPLabel(kernels[i].constituentCategory) && right),
			    /*kernelOp =*/ &kernels[i],
			    /*isPreterminal =*/ false);
			scoreKernel(entry);
			addTheory(entry);
		}
		KernelKey key2(BRANCH_DIRECTION_LEFT, rightEntry->constituentCategory,
			leftEntry->constituentCategory, leftEntry->headTag);
		kernels = kernelTable->lookup(key2, numKernels);
		for (int j = 0; j < numKernels; j++) {
			bool left = scOracle->isSignificant(leftEntry, kernels[j].modifierChain);
			bool right = scOracle->isSignificant(rightEntry, kernels[j].headChain);
			SignificantConstitNode *significantConstitNode = 
				_new SignificantConstitNode(left, 
				leftEntry->significantConstitNode, leftEntry->leftToken,
				leftEntry->rightToken, right, rightEntry->significantConstitNode, 
				rightEntry->leftToken, rightEntry->rightToken);
			ChartEntry* entry = _new ChartEntry(
			    /*constituentCategory =*/ kernels[j].constituentCategory,
			    /*headConstituent =*/ kernels[j].headChainFront,
			    /*headWord =*/ rightEntry->headWord,
			    /*headIsSignificant =*/ rightEntry->headIsSignificant,
			    /*headTag =*/ rightEntry->headTag,
			    /*leftEdge =*/ kernels[j].modifierChainFront,
			    /*leftTag =*/ leftEntry->headTag,
			    /*leftWord =*/ leftEntry->headWord,
			    /*rightEdge =*/ ParserTags::adjSymbol,
			    /*rightTag =*/ rightEntry->headTag,
			    /*rightWord =*/ rightEntry->headWord,
			    /*leftChild =*/ leftEntry,
			    /*rightChild =*/ rightEntry,
			    /*leftToken =*/ leftEntry->leftToken,
			    /*rightToken =*/ rightEntry->rightToken,
			    /*nameType =*/ ParserTags::nullSymbol,
			    /*SignificantConstitNode =*/ significantConstitNode,
			    /*isPPofSignificantConstit =*/ false,
			    /*kernelOp =*/ &kernels[j],
			    /*isPreterminal =*/ false);
			scoreKernel(entry);
			addTheory(entry);
		}
	}
}

void ChartDecoder::addExtensionTheories(ChartEntry* leftEntry,
    ChartEntry* rightEntry)
{
	if (rightClosable) {
		ExtensionKey key1(BRANCH_DIRECTION_RIGHT, leftEntry->constituentCategory,
			leftEntry->headConstituent, rightEntry->constituentCategory,
			leftEntry->rightEdge, rightEntry->headTag);
		int numExtensions;
		BridgeExtension* extensions = extensionTable->lookup(key1, numExtensions);
		for (int i = 0; i < numExtensions; i++) {
			bool right = scOracle->isSignificant(rightEntry, extensions[i].modifierChain);
			SignificantConstitNode *significantConstitNode = 
				_new SignificantConstitNode(false, 
				leftEntry->significantConstitNode, 0,0,
				right, rightEntry->significantConstitNode, 
				rightEntry->leftToken, rightEntry->rightToken);
			ChartEntry* entry = _new ChartEntry(
			    /*constituentCategory =*/ leftEntry->constituentCategory,
			    /*headConstituent =*/ leftEntry->headConstituent,
			    /*headWord =*/ leftEntry->headWord,
			    /*headIsSignificant =*/ leftEntry->headIsSignificant,
			    /*headTag =*/ leftEntry->headTag,
			    /*leftEdge =*/ leftEntry->leftEdge,
			    /*leftTag =*/ leftEntry->leftTag,
			    /*leftWord =*/ leftEntry->leftWord,
			    /*rightEdge =*/ extensions[i].modifierChainFront,
			    /*rightTag =*/ rightEntry->rightTag,
			    /*rightWord =*/ rightEntry->rightWord,
			    /*leftChild =*/ leftEntry,
			    /*rightChild =*/ rightEntry,
			    /*leftToken =*/ leftEntry->leftToken,
			    /*rightToken =*/ rightEntry->rightToken,
			    /*nameType =*/ ParserTags::nullSymbol,
			    /*significantConstitNode =*/ significantConstitNode,
			    // for PP attachment
			    /*isPPofSignificantConstit =*/ (LanguageSpecificFunctions::isPPLabel(leftEntry->constituentCategory) && right),
			    /*extensionOp =*/ &extensions[i],
			    /*isPreterminal =*/ false);
			scoreExtension(entry);
			addTheory(entry);
		}
	}
	if (leftClosable) {
		ExtensionKey key2(BRANCH_DIRECTION_LEFT, rightEntry->constituentCategory,
			rightEntry->headConstituent, leftEntry->constituentCategory,
			rightEntry->leftEdge, leftEntry->headTag);
		int numExtensions;
		BridgeExtension* extensions = extensionTable->lookup(key2, numExtensions);
		for (int j = 0; j < numExtensions; j++) {
			bool left = scOracle->isSignificant(leftEntry, extensions[j].modifierChain);
			SignificantConstitNode *significantConstitNode = 
				_new SignificantConstitNode(left, 
				leftEntry->significantConstitNode, leftEntry->leftToken,
				leftEntry->rightToken, false, rightEntry->significantConstitNode, 
				0,0);
			ChartEntry* entry = _new ChartEntry(
			    /*constituentCategory =*/ rightEntry->constituentCategory,
			    /*headConstituent =*/ rightEntry->headConstituent,
			    /*headWord =*/ rightEntry->headWord,
			    /*headIsSignificant =*/ rightEntry->headIsSignificant,
			    /*headTag =*/ rightEntry->headTag,
			    /*leftEdge =*/ extensions[j].modifierChainFront,
			    /*leftTag =*/ leftEntry->leftTag,
			    /*leftWord =*/ leftEntry->leftWord,
			    /*rightEdge =*/ rightEntry->rightEdge,
			    /*rightTag =*/ rightEntry->rightTag,
			    /*rightWord =*/ rightEntry->rightWord,
			    /*leftChild =*/ leftEntry,
			    /*rightChild =*/ rightEntry,
			    /*leftToken =*/ leftEntry->leftToken,
			    /*rightToken =*/ rightEntry->rightToken,
			    /*nameType =*/ ParserTags::nullSymbol,
			    /*significantConstitNode =*/ significantConstitNode,
			    /*isPPofSignificantConstit =*/ false,
			    /*extensionOp =*/ &extensions[j],
			    /*isPreterminal =*/ false);
			scoreExtension(entry);
			addTheory(entry);
		}
	}
}

namespace {
	template <typename T> struct pointer_values_equal
	{
		const T* to_find;
		bool operator()(const T* other) const
		{ return *to_find == *other; }
	};
}

void ChartDecoder::addTheory(ChartEntry* chartEntry)
{
    if (chartEntry->rankingScore <= -10000) {
			//cout << "deleting " << chartEntry->significantConstitNode << endl;
			delete chartEntry;
      return;
    }

	// Check if we already have this chartEntry.
    pointer_values_equal<ChartEntry> eq = { chartEntry };
    ChartEntry** match = std::find_if(theories, theories+numTheories, eq);

    if (match != (theories+numTheories)) {

#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
      if (chartEntry->rankingScore > (*match)->rankingScore) 
#else
      if (__fcmp (chartEntry->rankingScore, 
		  (*match)->rankingScore) > 0) 
#endif
	{
            delete (*match);
            (*match) = chartEntry;
            return;
        } else {
			delete chartEntry;
            return;
        }
    }
    if (numTheories < maxEntriesPerCell) {
        theories[numTheories++] = chartEntry;
        return;
    }
    int lowestScoring = 0;
    for (int j = 1; j < numTheories; j++) {
#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
        if (theories[j]->rankingScore < theories[lowestScoring]->rankingScore)
#else
        if (__fcmp (theories[j]->rankingScore,
		    theories[lowestScoring]->rankingScore) < 0)
#endif
            lowestScoring = j;
    }

#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
    if (chartEntry->rankingScore > theories[lowestScoring]->rankingScore) 
#else
    if (__fcmp(chartEntry->rankingScore,
	       theories[lowestScoring]->rankingScore) > 0)
#endif
      {            
        delete theories[lowestScoring];
        theories[lowestScoring] = chartEntry;
        return;
    } else {
        delete chartEntry;
        return;
    }
}

void ChartDecoder::transferTheoriesToChart(int start, int end)
{
    if (numTheories == 0) {
		return;
    }

    int highestScoring = 0;
    for (int i = 1; i < numTheories; i++) {
#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
        if (theories[i]->rankingScore > theories[highestScoring]->rankingScore)
#else
        if (__fcmp (theories[i]->rankingScore,
		    theories[highestScoring]->rankingScore) > 0)
#endif
            highestScoring = i;
    }
    float threshold = theories[highestScoring]->rankingScore + lambda;
    int j = 0;
    for (int k = 0; k < numTheories; k++) {
		// boundaries should already be checked, but just in case, check
		// against maxEntriesPerCell
        if (
#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
	    theories[k]->rankingScore > threshold 
#else
	    __fcmp (theories[k]->rankingScore, threshold) > 0
#endif
	    && j < maxEntriesPerCell) {
            chart[start][end - 1][j++] = theories[k];
        } else {
			delete theories[k];
        }
    }
    chart[start][end - 1][j] = 0;
}

void ChartDecoder::initChart(Symbol* sentence, int length,
							 std::vector<Constraint> & constraints,
							 const PartOfSpeechSequence* pos_constraints)
{
	bool name_word[maxSentenceLength];
	for (int i = 0; i < length; i++) {
		name_word[i] = false;
		possiblePunctuationOrConjunction[i] = false;
	}
	BOOST_FOREACH(Constraint constraint, constraints) {
		int left = constraint.left;
		int right = constraint.right;
		const Symbol &type = constraint.type;
		EntityType entityType = constraint.entityType;
		if (left >= MAX_SENTENCE_LENGTH ||
			  right >= MAX_SENTENCE_LENGTH) 
			  continue;
		if (type.is_null())
			continue;
		// nested names get handled as a post-process
		if (type == ParserTags::NESTED_NAME_CONSTRAINT)
			continue;
		// heads have to be only one word!
		if (type == ParserTags::HEAD_CONSTRAINT && left != right)
			continue;
		for (int k = left; k <= right; k++) {
			name_word[k] = true;
		}
		addConstraintEntry(left, right, sentence, type, entityType);
	}
	for (int m = 0; m < length; m++) {
		if (!name_word[m]){
			initChartWord(sentence[m], m, m == 0, pos_constraints);

		}
	}
	
	
}

void ChartDecoder::addConstraintEntry(int left, int right,
									  Symbol* sentence, const Symbol &type, EntityType entityType) {
	
	Symbol word = sentence[right];
	const Symbol* tags;
	int numTags;
	bool is_unknown_word = false;
	if (vocabularyTable->find(word)) {
		tags = partOfSpeechTable->lookup(word, numTags);
	} else {
		is_unknown_word = true;
		word = wordFeatures->features(sentence[right], 0);
		tags = partOfSpeechTable->lookup(word, numTags);
		if (numTags == 0) {
			word = wordFeatures->reducedFeatures(sentence[right], 0);
			tags = partOfSpeechTable->lookup(word, numTags);
		}
	}
	
	// EMB 03/10/04:
	// head constraints: we just want these to be heads of NPs, so we try to constrain 
	// the tag options for the headword
	// one-word core npas: we want to contrain the tag options the same way, except
	// we want to set the entry->nameType field, so they become their own node
	// even if the parser screws up
	// 
	int index = 0;
	ChartEntry *entry;
	if (type == ParserTags::HEAD_CONSTRAINT || type == LanguageSpecificFunctions::getCoreNPlabel()) 
	{
		for (int k = 0; k < numTags; k++) {
			const Symbol &tag = tags[k];
			if (index < maxEntriesPerCell && 
				LanguageSpecificFunctions::isNPtypePOStag(tag))
			{
				entry = _new ChartEntry();
				if (type == ParserTags::HEAD_CONSTRAINT)
					entry->nameType = ParserTags::nullSymbol;
				else entry->nameType = type;
				fillWordEntry(entry, tag, word, sentence[right], left, right);
				chart[left][right][index++] = entry;
			}
		}
		if (index == 0) {
			for (int k = 0; k < numTags; k++) {
				const Symbol &tag = tags[k];
				if (index < maxEntriesPerCell)
				{
					entry = _new ChartEntry();
					if (type == ParserTags::HEAD_CONSTRAINT)
						entry->nameType = ParserTags::nullSymbol;
					else entry->nameType = type;
					fillWordEntry(entry, tag, word, sentence[right], left, right);
					chart[left][right][index++] = entry;
				}
			}
		}
		chart[left][right][index] = 0;
		return;
	} else if (type == ParserTags::DATE_CONSTRAINT) {
		for (int k = 0; k < numTags; k++) {
			const Symbol &tag = tags[k];
			if (index < maxEntriesPerCell) {
				entry = _new ChartEntry();
				entry->nameType = LanguageSpecificFunctions::getDateLabel();
				fillWordEntry(entry, tag, word, sentence[right], left, right);
				chart[left][right][index++] = entry;
			}
		}
		chart[left][right][index] = 0;
		return;
	}
	
	// REVISED CURRENT STRATEGY (JCS 2/2/04)
	// Primary, secondary and default tags (and default name words) can optionally be specified 
	// by EntityType.  Otherwise, fall back on the original defaults in LanguageSpecificFunctions.
	//
	// CURRENT STRATEGY (EMB 3/31/03)
	// for known words, pick up all primary or secondary tags and let the parser decide what's best
	// for unknown words, only pick up primary tags
	//
	// NOTE: If we continue to expand the set of things we want to use this for (in
	// particular, non-named entities), we should make the LanguageSpecificFunctions 
	// functions used here be particular to the type of tag: clearly, nuclear substances 
	// should have different primary/secondary/default tags than person names. 
	// For now, though, we just hope for the best.
	
	for (int k = 0; k < numTags; k++) {
		const Symbol &tag = tags[k];
		if (index < maxEntriesPerCell && 
			(LanguageSpecificFunctions::isPrimaryNamePOStag(tag, entityType) ||
			 (!is_unknown_word &&
			  LanguageSpecificFunctions::isSecondaryNamePOStag(tag, entityType))))
		{
			entry = _new ChartEntry();
			entry->nameType = type;
			fillWordEntry(entry, tag, word, sentence[right], left, right);
			chart[left][right][index++] = entry;
		}
	}


	// if we can't get a tag we want for this word, we are going to set the word to an unknown
	// vector that will. This way the parse won't fragment on account of the name. 
	// Specifically, we set the word to the feature vector of getDefaultNameWord().
	if (index == 0 && !LanguageSpecificFunctions::getDefaultNameWord(DECODER_TYPE, entityType).is_null()) {
		word = wordFeatures->features(LanguageSpecificFunctions::getDefaultNameWord(DECODER_TYPE, entityType), false);
		tags = partOfSpeechTable->lookup(word, numTags);
		for (int k = 0; k < numTags; k++) {
			const Symbol &tag = tags[k];
			if (index < maxEntriesPerCell &&
				(LanguageSpecificFunctions::isPrimaryNamePOStag(tag, entityType)))
			{
				entry = _new ChartEntry();
				entry->nameType = type;
				fillWordEntry(entry, tag, word, sentence[right], left, right);
				chart[left][right][index++] = entry;
			}
		}
	}

	// if no default name word or no primary tags fit the default name word, just add this in,
	// but be aware that this WILL fragment the parse
	if (index == 0) {
		entry = _new ChartEntry();
		entry->nameType = type;
		fillWordEntry(entry, LanguageSpecificFunctions::getDefaultNamePOStag(entityType), word, 
			sentence[right], left, right);
		chart[left][right][index++] = entry;
	}

	chart[left][right][index] = 0;

}




void ChartDecoder::initChartWord(Symbol word, int chartIndex, bool firstWord, 
								 const PartOfSpeechSequence* pos_constraints)
{
	int numTags = 0;  
	int tmpNumTags = 0;
	bool lcTags = false; // this means: I used tags generated from the lowercase word
	bool usedModelLc = false; // this means: I actually want to use the lowercase word in the model
	const Symbol* tags;
	Symbol good_tags[MAX_TAGS_PER_WORD];
	const Symbol* tmp_tags;
	Symbol collected_tags[MAX_TAGS_PER_WORD];
	Symbol selectedTags[MAX_TAGS_PER_WORD];
	int n_good_tags = 0;
	Symbol originalWord = word;
	Symbol lcword; 
	int n_allowed_pos = 0;
	PartOfSpeech* allowed_pos = 0;
	if((pos_constraints != 0) && (chartIndex < pos_constraints->getNTokens())){
		allowed_pos = pos_constraints->getPOS(chartIndex);
		n_allowed_pos= allowed_pos->getNPOS();
	}

	if (vocabularyTable->find(word)) {
		tags = partOfSpeechTable->lookup(word, numTags);
	} else {
		lcword = SymbolUtilities::lowercaseSymbol(word);

		word = wordFeatures->features(originalWord, firstWord);
		tags = partOfSpeechTable->lookup(word, numTags);
		if (numTags == 0) { 
			word = wordFeatures->reducedFeatures(originalWord, firstWord);
			tags = partOfSpeechTable->lookup(word, numTags);
		}

		// All of the following if statements overrule the word-feature
		// lookup that was just performed...

		if (lowerCaseForUnknown && firstWord && vocabularyTable->find(lcword)){
			tmp_tags = partOfSpeechTable->lookup(lcword, tmpNumTags);
			if (tmpNumTags > 0) { 
				lcTags = true;
				usedModelLc = true;
			}
		}
		if (!lcTags){  //try auxiliary table, which is all lowercased
			tmp_tags = auxPartOfSpeechTable->lookup(lcword, tmpNumTags);
			if (tmpNumTags > 0) {
				// If the auxiliary table was generated by the same model
				// as being used here, there will always be overlap.
				// But if not, it's not guaranteed, and restricting to the
				// tags in the auxiliary table could force your whole parse to fragment!
				bool found_overlap = false;
				for (int t1 = 0; t1 < tmpNumTags; t1++) {
					for (int t2 = 0; t2 < numTags; t2++) {
						if (tmp_tags[t1] == tags[t2]) {
							found_overlap = true;
							break;
						}
					}
				}
				lcTags = found_overlap;
			}
		}
		if (lcTags){// lc ploy worked, now add the proper noun option if needed
			bool needsNNP = (lcword.to_string()[0] != originalWord.to_string()[0]);
			const Symbol &symbolNNP = LanguageSpecificFunctions::getProperNounLabel();
			if (needsNNP){
				for (int ti=0; ti<tmpNumTags; ti++){
					collected_tags[ti] = tmp_tags[ti];
					if (tmp_tags[ti] == symbolNNP){
						needsNNP = false;
						break;
					}
				}
			}
			if (needsNNP){
				collected_tags[tmpNumTags] = symbolNNP;
				tmpNumTags++;
				tags = collected_tags;
			}else{
				tags = tmp_tags;
				if (usedModelLc) word = lcword;
			}
			numTags = tmpNumTags;
		} else if (constrainKnownNounsAndVerbs) {
			
			// This is only implemented in English right now, and uses WordNet
			bool is_known_noun = LanguageSpecificFunctions::isKnownNoun(lcword);
			bool is_known_verb = LanguageSpecificFunctions::isKnownVerb(lcword);

			if (is_known_noun && !is_known_verb) {
				// only remove all verb tags if we can find a noun tag
				for (int t = 0; t < numTags; t++) {
					if (LanguageSpecificFunctions::isNounPOS(tags[t])) {
						SessionLogger::info("SERIF") << "Removing verb tags for " << lcword.to_debug_string() << "\n";
						int nseltags = 0;
						for (int t = 0; t < numTags; t++) {
							if (!LanguageSpecificFunctions::isVerbPOS(tags[t]))
								selectedTags[nseltags++] = tags[t];		
						}
						numTags = nseltags;
						tags = selectedTags;
						break;
					}
				}
			}
			if (is_known_verb && !is_known_noun && !LanguageSpecificFunctions::isPotentialGerund(word)) {
				// only remove all noun tags if we can find a verb tag
				for (int t = 0; t < numTags; t++) {
					if (LanguageSpecificFunctions::isVerbPOS(tags[t])) {
						SessionLogger::info("SERIF") << "Removing noun tags for " << lcword.to_debug_string() << "\n";
						int nseltags = 0;
						for (int t = 0; t < numTags; t++) {
							if (!LanguageSpecificFunctions::isNounPOS(tags[t]))
								selectedTags[nseltags++] = tags[t];	
						}
						numTags = nseltags;
						tags = selectedTags;
						break;
					}
				}
			}
		}

		if (restrictedPosTagsSize > 0){
			if (numTags == 0){
				tags = restrictedPosTags;
				numTags = restrictedPosTagsSize;
			}else if (numTags >= restrictedPosTagsSize){
				int nseltags = 0;
				for (int ir=0; ir<restrictedPosTagsSize; ir++){
					for (int ti=0; ti<numTags; ti++){
						if (tags[ti] == restrictedPosTags[ir]){
							selectedTags[nseltags++] = restrictedPosTags[ir];
							break;
						}
					}
				}
				numTags = nseltags;
				tags = selectedTags;
			}
		}

	}

	Symbol ignored_tags[MAX_TAGS_PER_WORD];
	int n_ignored = 0;
	for(int i = 0; i< numTags; i++){
		bool found = false;
		for(int j =0; j< n_allowed_pos; j++){
			const Symbol &label = LanguageSpecificFunctions::convertPOSTheoryToParserPOS(allowed_pos->getLabel(j));
			if(label == tags[i]){
				found = true;
				good_tags[n_good_tags++] = tags[i];
				break;
			}
		}
		if(!found){
			ignored_tags[n_ignored++] = tags[i];
		}
		
	}
	/*
	//debugging print statements
	std::cout<<"n_allowed_pos: "<<n_allowed_pos<<" for index "<<chartIndex<<" -\t found "<<n_good_tags<<" for "<<numTags<<" original pos labels"<<std::endl;
	if((n_good_tags == 0) && (n_allowed_pos > 0)){
		std::cout<<"orig_word: "<<originalWord.to_debug_string()<<" used word: "<<word.to_debug_string()<<" \t";
		std::cout<<"Alowed:     ";
		for(int j =0; j< n_allowed_pos; j++){
			std::cout<<allowed_pos->getLabel(j).to_debug_string()<<",   ";
		}
		std::cout<<std::endl;
		std::cout<<"Parser:     ";
		for(int j =0; j< numTags; j++){
			std::cout<<tags[j].to_debug_string()<<",   ";
		}
		std::cout<<std::endl;
	}
	else if((n_allowed_pos != 0) &&(n_good_tags < numTags)){
		std::cout<<"removed tags for orig_word: "<<originalWord.to_debug_string()<<" used word: "<<word.to_debug_string()<<" \t";
		for(int i =0; i< n_ignored; i++){
			std::cout<<ignored_tags[i].to_debug_string()<<",   ";
		}
		std::cout<<" with allowed tags: ";
		for(int j =0; j< n_allowed_pos; j++){
			std::cout<<allowed_pos->getLabel(j).to_debug_string()<<",   ";
		}
		std::cout<<std::endl;
	}
*/
	//some part of speech tags were matched, some were filtered
	if((n_good_tags > 0) && (n_good_tags < numTags) ){
		tags = good_tags;
		numTags = n_good_tags;
	}

	/*
	//this is how constraints worked for the SB parsing experiments, a word was reduced to its features
	//if its pos tag wasn't known.  For Serif, we want to go with what the parser thinks, mrf
	Symbol constrained_tag_array[1];
	bool found_tag = false;
	Symbol replacement_word = word;
	if((allowed_pos != 0)
		&& (allowed_pos->getNPOS() == 1)){
		Symbol pos = allowed_pos->getLabel(0);
		for(int i = 0; i<numTags; i++){
			if(tags[i] == pos){
				found_tag = true;
				break;
			}
		}
		if(!found_tag){
			const Symbol* othertags;
			int nother = 0;
			Symbol features = wordFeatures->features(originalWord, firstWord);
			othertags = partOfSpeechTable->lookup(features, nother);
			for(int i=0; i< nother; i++){
				if(othertags[i] == pos){
					found_tag = true;
					replacement_word = features;
					break;
				}
			}
		}
		if(!found_tag){
			const Symbol* othertags;
			int nother = 0;
			Symbol features = wordFeatures->reducedFeatures(originalWord, firstWord);
			othertags = partOfSpeechTable->lookup(features, nother);
			for(int i=0; i< nother; i++){
				if(othertags[i] == pos){
					found_tag = true;
					replacement_word = features;
					break;
				}
			}
		}
		if(found_tag){
			constrained_tag_array[0] = pos;
			tags = constrained_tag_array;
			numTags = 1;
			word = replacement_word;
		}

	}
	*/
			 


	for (int j = 0; j < numTags; j++) {
		const Symbol &tag = tags[j];
		if (LanguageSpecificFunctions::isBasicPunctuationOrConjunction(tag))
		{
			possiblePunctuationOrConjunction[chartIndex] = true;
		}
		
		ChartEntry *entry = _new ChartEntry();
		entry->nameType = ParserTags::nullSymbol;
		fillWordEntry(entry, tag, word, originalWord, chartIndex, chartIndex);
		
		chart[chartIndex][chartIndex][j] = entry;
	}

	if (numTags == 0) {
		SessionLogger::warn("parser_word_features")
			<< "ChartDecoder::initChartWord(): some word is reducing to a feature vector\n"
			<< "never seen in training: word -- "
			<< originalWord.to_debug_string()
			<< ", vector -- "
			<< word.to_debug_string();
		ChartEntry *entry = _new ChartEntry();
		entry->nameType = ParserTags::nullSymbol;
		// don't actually calculate ranking score (that's what the "false" parameter indicates)
		fillWordEntry(entry, ParserTags::unknownTag, word, 
			originalWord, chartIndex, chartIndex, false);
		chart[chartIndex][chartIndex][0] = entry;	
		numTags = 1;
	}

	chart[chartIndex][chartIndex][numTags] = 0;
}
void ChartDecoder::fillWordEntry(ChartEntry *entry,
								 const Symbol &tag, const Symbol &word, const Symbol &originalWord,
								 int left, int right, bool setRankingScore) {
	fillWordEntry(entry, tag, tag, word, originalWord, left, right, setRankingScore);
}

void ChartDecoder::fillWordEntry(ChartEntry *entry, const Symbol &cat, 
								 const Symbol &tag, const Symbol &word, const Symbol &originalWord,
								 int left, int right, bool setRankingScore) {
	entry->constituentCategory = cat;
	entry->headConstituent = ParserTags::nullSymbol;
	entry->headWord = word;
	entry->headTag = tag;
	entry->leftEdge = ParserTags::adjSymbol;
	entry->rightEdge = ParserTags::adjSymbol;
	
	// added to be able to do lexical bigrams
	entry->leftTag = tag;
	entry->rightTag = tag;
	entry->leftWord = word;
	entry->rightWord = word;
	
	entry->leftChild = 0;
	entry->rightChild = 0;
	/* //mrf change score to account for features

	entry->insideScore = 0;
	if (setRankingScore)
		// Now we want the score of the tag, for fragments with no parents
		entry->rankingScore = priorProbTable->lookup(cat, tag) +
					  		  (float)log((leftLexicalProbs->lookupML(word, tag) +
							  rightLexicalProbs->lookupML(word, tag))
							  * 0.5);
	else
		entry->rankingScore = 0;
	*/
	Symbol ngram[1];
	ngram[0]= word;
	float wordScore = featTable->lookup(ngram);
	entry->insideScore = wordScore;
	if (setRankingScore) {
		// Now we want the score of the tag, for fragments with no parents
		float lex = (leftLexicalProbs->lookupML(word, tag) 
			+ rightLexicalProbs->lookupML(word, tag)) * 0.5f;
		if (lex == 0) 
			lex = -10000;
		else lex = (float)log(lex);
		entry->rankingScore = priorProbTable->lookup(cat, tag) + lex + wordScore;
	} else
		entry->rankingScore = wordScore;

	

	entry->leftCapScore = 0;
	entry->rightCapScore = 0;
	entry->isPreterminal = true;
	entry->isPPofSignificantConstit = false;
	
	// diversity parsing
	entry->leftToken = left;
	entry->rightToken = right;
	if (entry->nameType != ParserTags::nullSymbol)
		entry->headIsSignificant = true;
	else if (LanguageSpecificFunctions::isNPtypePOStag(tag)) 
		entry->headIsSignificant = scOracle->isPossibleDescriptorHeadWord(originalWord);
	else entry->headIsSignificant = false;
	
	entry->significantConstitNode = _new SignificantConstitNode();
	
}


void ChartDecoder::scoreKernel(ChartEntry* entry)
{
  float headScore = headProbs->lookup(
                                      entry->kernelOp->headChain,
                                      entry->constituentCategory,
                                      entry->headWord,
                                      entry->headTag);
  const ChartEntry* modifier;
  ModifierProbs* modProbs;
  LexicalProbs* lexProbs;
  LexicalProbs* altLexProbs;
  if (entry->kernelOp->branchingDirection == BRANCH_DIRECTION_LEFT) {
    modifier = entry->leftChild;
    modProbs = premodProbs;
    lexProbs = leftLexicalProbs;
    altLexProbs = rightLexicalProbs;
  } else {
    modifier = entry->rightChild;
    modProbs = postmodProbs;
        lexProbs = rightLexicalProbs;
        altLexProbs = leftLexicalProbs;
  }
  float modScore = modProbs->lookup(
                                    entry->kernelOp->modifierChain,
                                    modifier->headTag,
                                    entry->constituentCategory,
                                    entry->headConstituent,
                                    ParserTags::adjSymbol,
                                    entry->headWord,
                                    entry->headTag);
  float lexScore = lexProbs->lookup(
                                    altLexProbs,
                                    modifier->headWord,
                                    entry->kernelOp->modifierChainFront,
                                    modifier->headTag,
                                    entry->constituentCategory,
                                    entry->headConstituent,
                                    entry->headWord,
                                    entry->headTag); 
  float attachmentScore = headScore * modScore * lexScore;
  if (attachmentScore == 0) {
    entry->insideScore = -10000;
    entry->rankingScore = -10000;
  } else {
    entry->insideScore = (float)log(attachmentScore) +
      entry->leftChild->insideScore +
      entry->leftChild->leftCapScore +
      entry->leftChild->rightCapScore +
      entry->rightChild->insideScore +
      entry->rightChild->leftCapScore +
      entry->rightChild->rightCapScore;
    
    // special case for top
    if (ParserTags::TOPTAG == entry->headTag) {
      float priorScore = priorProbTable->lookup(entry->constituentCategory,
                                                entry->headTag);
      entry->rankingScore = entry->insideScore + priorScore;
      
    }
    
    else {
      Symbol ngram[] = {entry->constituentCategory, entry->headTag, entry->headWord};
      if (cache_type == Simple) {
        Cache<cacheN>::simple::iterator iter = simpleCache.find(ngram);
        if (iter != simpleCache.end()) {
          entry->rankingScore = entry->insideScore + (*iter).second;
        } else {
          float partial_score = computePartialRankingScore(entry);
          entry->rankingScore = entry->insideScore + partial_score;
          simpleCache.insert(std::make_pair(ngram, partial_score));
        }
#if !defined(_WIN32) && !defined(__APPLE_CC__)
      } else if(cache_type == Lru) {
        Cache<cacheN>::lru::iterator iter = lruCache.find(ngram);
        if (iter != lruCache.end()) {
          entry->rankingScore = entry->insideScore + (*iter).second;
        } else {
          float partial_score = computePartialRankingScore(entry);
          entry->rankingScore = entry->insideScore + partial_score;
          lruCache.insert(std::make_pair(ngram, partial_score));
        }
#endif
      } else {
        entry->rankingScore = entry->insideScore + computePartialRankingScore(entry);
      }
    }
  }
  
  capLeft(entry);
  capRight(entry);
}

inline
float ChartDecoder::computePartialRankingScore(ChartEntry* entry) {
  float priorScore = priorProbTable->lookup(entry->constituentCategory,
                                            entry->headTag);
  float lexMLScore = (leftLexicalProbs->lookupML(entry->headWord, entry->headTag) +
                      rightLexicalProbs->lookupML(entry->headWord, entry->headTag))
    * 0.5f;
  if (lexMLScore == 0)
    lexMLScore = -10000;
  else 
    lexMLScore = (float)log(lexMLScore);
          
  return priorScore + lexMLScore;

}
  
void ChartDecoder::scoreExtension(ChartEntry* entry)
{
    const ChartEntry* modifier;
    ModifierProbs* modProbs;
    Symbol prevMod;
	Symbol prevWord;
	Symbol prevTag;
    LexicalProbs* lexProbs;
    LexicalProbs* altLexProbs;
	bool use_special_lexical_probs;
    if (entry->extensionOp->branchingDirection == BRANCH_DIRECTION_LEFT) {
        modifier = entry->leftChild;
        modProbs = premodProbs;
        prevMod = entry->rightChild->leftEdge;
		prevWord = entry->rightChild->leftWord;
		prevTag = entry->rightChild->leftTag;
        lexProbs = leftLexicalProbs;
        altLexProbs = rightLexicalProbs;
		if (sequentialBigrams->use_left_sequential_bigrams(entry->constituentCategory))
			use_special_lexical_probs = true;
		else use_special_lexical_probs = false;
    } else {
        modifier = entry->rightChild;
        modProbs = postmodProbs;
        prevMod = entry->leftChild->rightEdge;
		prevWord = entry->leftChild->rightWord;
		prevTag = entry->leftChild->rightTag;
        lexProbs = rightLexicalProbs;
        altLexProbs = leftLexicalProbs;
		if (sequentialBigrams->use_right_sequential_bigrams(entry->constituentCategory))
			use_special_lexical_probs = true;
		else use_special_lexical_probs = false;
    }
	float modScore;
    if (!use_special_lexical_probs) {
		modScore = modProbs->lookup(
			entry->extensionOp->modifierChain,
			modifier->headTag,
			entry->constituentCategory,
			entry->headConstituent,
			prevMod,
			entry->headWord,
			entry->headTag);
	} else {
		modScore = modProbs->lookup(
			entry->extensionOp->modifierChain,
			modifier->headTag,
			entry->constituentCategory,
			entry->headConstituent,
			prevMod,
			prevWord,
			prevTag);
	}
	float lexScore;
	if (!use_special_lexical_probs) {
		lexScore = lexProbs->lookup(
			altLexProbs,
			modifier->headWord,
			entry->extensionOp->modifierChainFront,
			modifier->headTag,
			entry->constituentCategory,
			entry->headConstituent,
			entry->headWord,
			entry->headTag);
	} else {
		lexScore = lexProbs->lookup(
			altLexProbs,
			modifier->headWord,
			entry->extensionOp->modifierChainFront,
			modifier->headTag,
			entry->constituentCategory,
			entry->headConstituent,
			prevWord,
			prevTag);
	}
    float attachmentScore = modScore * lexScore;
    if (attachmentScore == 0) {
        entry->insideScore = -10000;
        entry->rankingScore = -10000;
    } else {
        entry->insideScore = (float)log(attachmentScore) +
            entry->leftChild->insideScore +
            entry->rightChild->insideScore +
            modifier->leftCapScore +
            modifier->rightCapScore;
		float lexMLScore = (leftLexicalProbs->lookupML(entry->headWord, entry->headTag) +
				rightLexicalProbs->lookupML(entry->headWord, entry->headTag))
				* 0.5f;
		if (lexMLScore == 0) 
			lexMLScore = -10000;
		else lexMLScore = (float)log(lexMLScore);
        entry->rankingScore = entry->insideScore +
            priorProbTable->lookup(entry->constituentCategory,
			    entry->headTag) + lexMLScore;
		
    }
    if (entry->extensionOp->branchingDirection == BRANCH_DIRECTION_LEFT) {
        entry->rightCapScore = entry->rightChild->rightCapScore;
        capLeft(entry);
    } else {
        entry->leftCapScore = entry->leftChild->leftCapScore;
        capRight(entry);
    }
}

void ChartDecoder::capLeft(ChartEntry* entry)
{
    float capScore = premodProbs->lookup(
        ParserTags::exitSymbol,
        ParserTags::exitSymbol,
        entry->constituentCategory,
        entry->headConstituent,
        entry->leftEdge,
        entry->headWord,
        entry->headTag);
    if (capScore == 0)
        entry->leftCapScore = -10000;
    else
        entry->leftCapScore = (float)log(capScore);

}

void ChartDecoder::capRight(ChartEntry* entry)
{
    float capScore = postmodProbs->lookup(
        ParserTags::exitSymbol,
        ParserTags::exitSymbol,
        entry->constituentCategory,
        entry->headConstituent,
        entry->rightEdge,
        entry->headWord,
        entry->headTag);
    if (capScore == 0)
        entry->rightCapScore = -10000;
    else
        entry->rightCapScore = (float)log(capScore);

}

ParseNode* ChartDecoder::getBestParse(float &score, 
									  int start,
									  int end,
									  bool topRequired)
{

	int endChartIndex = end - 1;


	// prepare the global theories array with valid standalone sentences
    if (chart[start][endChartIndex][0] != 0) {
        numTheories = 0;
        ChartEntry leftEntry;
		leftEntry.significantConstitNode = _new SignificantConstitNode();
        leftEntry.constituentCategory = ParserTags::TOPTAG;
        leftEntry.headConstituent = ParserTags::nullSymbol;
        leftEntry.headWord = ParserTags::TOPWORD;
        leftEntry.headTag = ParserTags::TOPTAG;
        leftEntry.leftEdge = ParserTags::adjSymbol;
        leftEntry.rightEdge = ParserTags::adjSymbol;
        leftEntry.leftChild = 0;
        leftEntry.rightChild = 0;
        leftEntry.insideScore = 0;
        leftEntry.rankingScore = 0;
        leftEntry.leftCapScore = 0;
        leftEntry.rightCapScore = 0;
		leftEntry.leftToken = 0;
		leftEntry.rightToken = 0;
		leftEntry.isPreterminal = true;

		for (int i = 0; chart[start][endChartIndex][i] != 0; i++) {
            ChartEntry* rightEntry = chart[start][endChartIndex][i];
            addKernelTheories(&leftEntry, rightEntry);;
        }
	}
// NOTE: It is possible to let getBestFragmentedParse select the top whole-sentence
// parse as well as the fragmented one. But there isn't multiple-parses support for it, so
// for now, the below code is disabled. In the future, it may be turned back on.

/*	// new style now uses the theories array, as well
	if (_frag_prob >= 0) {
		// this prevents any multiples, obviously
		ParseNode *return_tree = getBestFragmentedParse(score, start, endChartIndex);
		// cleanup from the previous initialization
		if (chart[start][endChartIndex][0] != 0 && numTheories > 0) {
			for (int l = 0; l < numTheories; l++)
				delete theories[l];
		}
		return return_tree;
	}
*/
    if (chart[start][endChartIndex][0] != 0) {
		if (numTheories > 0) {
			ParseNode *return_tree = 
				getMultipleParses(theories, numTheories, true);

			// delete theories
			for (int l = 0; l < numTheories; l++) {
				delete theories[l];
			}
			
			return return_tree;

        } else if (!topRequired) {

			int index;
			for (index = 0; chart[start][endChartIndex][index] != 0; index++)
			{}
			
			ParseNode *return_tree = 
				getMultipleParses(chart[start][endChartIndex], index, false);

			return return_tree;
       }
    }
	float fragmented_parse_score;
	highest_scoring_final_theory = 0;
	return getBestFragmentedParse(fragmented_parse_score, start, endChartIndex);
		
 }

#if 0
static int
compare_chart_entry (const void *p1, const void *p2)
{
  ChartEntry *const *entry1 = static_cast<ChartEntry *const*>(p1);
  ChartEntry *const *entry2 = static_cast<ChartEntry *const*>(p2);
  int rc = 0;
       if ((*entry1)->insideScore < (*entry2)->insideScore) rc = -1;
  else if ((*entry1)->insideScore > (*entry2)->insideScore) rc = 1;
  else if ((*entry1)->rankingScore < (*entry2)->rankingScore) rc = -1;
  else if ((*entry1)->rankingScore > (*entry2)->rankingScore) rc = 1;
  else if ((*entry1)->leftCapScore > (*entry2)->leftCapScore) rc = -1;
  else if ((*entry1)->leftCapScore < (*entry2)->leftCapScore) rc = 1;
  else if ((*entry1)->rightCapScore > (*entry2)->rightCapScore) rc = -1;
  else if ((*entry1)->rightCapScore < (*entry2)->rightCapScore) rc = 1;
  return rc;
}
#endif

ParseNode* ChartDecoder::getMultipleParses(ChartEntry **possibleTrees, 
										   int numPossibleTrees,
										   bool only_right_child) 
{
	int good_theories[MAX_TAGS_PER_WORD];
	
	/*MRF 2-22-2004
		int good_theories[MAX_ENTRIES_PER_CELL];
		if you are dealing with a one word sentence, 
		the numPossibleTrees will be at most MAX_TAGS_PER_WORD
		not MAX_ENTRIES_PER_CELL
	*/
#if 0
	qsort (possibleTrees, numPossibleTrees, sizeof (ChartEntry *), 
	       compare_chart_entry);
#endif

	// initialize good_theories
	for (int r = 0; r < numPossibleTrees; r++)
		good_theories[r] = 0;
	
	// set good_theories
	for (int m = 0; m < numPossibleTrees; m++) {
		if (good_theories[m] == -1) {
			//cout << "Theory " << m << " deleted\n";
			continue;
		}
		//cout << "Node   " << m << ": ";
		bool flag = true;
		for (int n = m + 1; n < numPossibleTrees; n++) {
			//if(!only_right_child)std::cerr<<"\t\t\tlook for sub :" <<n<<std::endl;

			if (good_theories[n] == -1)
				continue;
			if (*(possibleTrees[m]->significantConstitNode) == 
				*(possibleTrees[n]->significantConstitNode)) {
				if (
#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
				    possibleTrees[m]->insideScore <
					possibleTrees[n]->insideScore
#else
				    __fcmp (possibleTrees[m]->insideScore,
					    possibleTrees[n]->insideScore) < 0
#endif
				    ) {
					//cout << " XXX" << endl;
					good_theories[m] = -1;
					flag = false;
					break;
				} else {
					good_theories[n] = -1;
				}
			}
		}
		if (flag) {
			good_theories[m] = 1;
			//cout << endl;
		} 
		
	}
	
	
	ParseNode* return_tree = 0;
	ParseNode* tree = 0;
	float highestScore = -10000;
	int highestScoring = -1;
	
	// find highestScore
	for (int j = 0; j < numPossibleTrees; j++) {
		if (good_theories[j] == 1 &&
#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
			possibleTrees[j]->insideScore > highestScore
#else
		    __fcmp (possibleTrees[j]->insideScore, highestScore) > 0
#endif
		    )
		{
			highestScore = possibleTrees[j]->insideScore;
			highestScoring = j;						
		}
	}

	if (highestScoring == -1)
		return 0;
	
	// get trees
	int score_index = 0;
	for (int k = 0; k < numPossibleTrees; k++) {
		if (good_theories[k] == 1 &&
#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
			possibleTrees[k]->insideScore > highestScore - 10
#else
		    __fcmp (possibleTrees[k]->insideScore, 
			    highestScore - 10.) > 0
#endif
		    )
		{
			ParseNode* subtree;
			if (only_right_child){
				subtree = possibleTrees[k]->rightChild->toParseNode();
			}
			else{
				subtree = possibleTrees[k]->toParseNode();
			}
			theory_scores[score_index] = possibleTrees[k]->rankingScore;
			theory_sc_strings[score_index] = possibleTrees[k]->significantConstitNode->toString();
			if (highestScoring == k)
				highest_scoring_final_theory = score_index;
			score_index++;
			
			if (tree == 0) {
				return_tree = subtree;
				tree = subtree;
				tree->next = 0;
			} else {
				tree->next = subtree;
				tree = tree->next;
				tree->next = 0;
			}
			
		} else {
			//cout << "discarded due to score: " << k << endl;
		}
	}
	
	//cout << "****************\n";
	return return_tree;

}

// note: the former implementation of this method is very different, but accomplishes
// more or less the same thing, with some limitations
ParseNode* ChartDecoder::getBestFragmentedParse(float &score, int start, int end)
{

	int size = end-start+1;
	ParseNode** bestRoutes = _new ParseNode*[size];
	float* routeScores = _new float[size];
	int i;
	// initialize all scores to log of 0 and all routes to empty pointers
	for (i = 0; i < size; i++) {
		routeScores[i] = -10000;
		bestRoutes[i] = 0;
	}

	// for each starting point in the chart, create a node for the best way
	// to get to a later point in the chart, and store that parsenode in the 
	// bestroutes array. score is score of existing node in the start position 
	// in the bestRoutes array (except at the beginning) + score of the new node
	// + log(the frag penalty), except at the beginning.

	for (i = 0; i < size; i++) {
		if (i != 0 && bestRoutes[i-1] == 0)
			continue;
		int j;
		for (j = i; j < size; j++) {
			// find the best way from start+i to start+j
			if (chart[start+i][start+j][0] != 0) {
				int highestScoring = -1;
				ChartEntry* highestEntry = 0;
				// changes the way the parse node is acquired
				bool hasTopTag = false;

// DISABLED: this manner of getting fragments can be extended to get sentences or fragments,
// depending on which score is higher. This is disabled until multiple parses are handled here,
// but uncommenting the below code will allow the special case of validated whole-sentence
// theories to be captured.
				
				// for the case of whole sentences, we first check the theories array 
				// for validated sentences and use them if possible
				//if (i == 0 && j == size-1 && numTheories > 0) {
				//	for (int l = 0; l < numTheories; l++) {
				//		if (highestScoring < 0 ||
				//			(theories[l]->rankingScore >
				//			 theories[highestScoring]->rankingScore))
				//		{
				//			highestScoring = l;
				//		}
				//	}
				//	if (highestScoring >=0) {
				//		highestEntry = theories[highestScoring];
				//		hasTopTag = true;
				//	}
				//}
				

				// non-whole-sentence cases, and if there's no valid theories
				if (highestScoring < 0) {
					for (int l = 0; chart[start+i][start+j][l] != 0; l++) {
						if (highestScoring < 0 ||
#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
							(chart[start+i][start+j][l]->rankingScore >
							chart[start+i][start+j][highestScoring]->rankingScore)
#else
						    __fcmp (chart[start+i][start+j][l]->rankingScore,
							    chart[start+i][start+j][highestScoring]->rankingScore)
						    > 0
#endif
						    )
						{
							highestScoring = l;
						}
					}
					if (highestScoring < 0)
						continue;
					highestEntry = chart[start+i][start+j][highestScoring];
				}

				// if the route to start+j is better than all the other routes, keep just that
				// route in bestRoutes[j]

				// the score is the penalty * p(getting right before here)*p(getting what is here) 
				// if we started at start, no penalty, and (obviously) no previous score
				float currScore;
				if (i == 0)
					currScore = highestEntry->rankingScore;
				else
					// for frag probability of 0, we actually substitute the 
					// log of 1x10^-100 - this differentiates from -10000
					currScore = (_frag_prob
									? static_cast<float>(log(_frag_prob))
									: -100.0f) + 
								routeScores[i-1] + 
								highestEntry->rankingScore;

				if (currScore > routeScores[j]) {
					if (bestRoutes[j] != 0)
						delete bestRoutes[j];
					// the frag will be under a fragments tag 
					// unless it extends to the end of the sentence.
					ParseNode* newNode;
					if (j == size-1) {
						newNode = hasTopTag ? highestEntry->rightChild->toParseNode() : highestEntry->toParseNode();
					} else {
						newNode = _new ParseNode(ParserTags::FRAGMENTS, start+i, start+j);
						newNode->headNode = highestEntry->toParseNode();
					}
					if (i == 0)
						bestRoutes[j] = newNode;
					else {
						// copy the frag up to the last word into this new frag,
						// and add the current frag as the last postmod
						bestRoutes[j] = _new ParseNode(bestRoutes[i-1]);
						bestRoutes[j]->chart_end_index = newNode->chart_end_index;
						ParseNode* posts = bestRoutes[j]->postmods;
						if (posts == 0)
							bestRoutes[j]->postmods = newNode;
						else {
							while (posts->postmods != 0) {
								posts->chart_end_index = newNode->chart_end_index;
								posts = posts->postmods;
							}
							posts->postmods = newNode;
							posts->chart_end_index = newNode->chart_end_index;
						}
					}
					routeScores[j] = currScore;
				}
			}
		}
	}
	score = routeScores[size-1];
	ParseNode* retNode;
	if (bestRoutes[size-1] == 0)
		retNode = static_cast<ParseNode*>(0);
	else
		retNode = bestRoutes[size-1];

	// hacks to mimic what getMultipleParses does...almost
	// since the theory isn't necessarily from a single chart entry, I can't
	// store the sc string
	highest_scoring_final_theory = 0;
	theory_scores[0] = score; 

	// clean up all but the last node

	for (int j = 0; j < size-1; j++)
		if (bestRoutes[j] != 0)
			delete bestRoutes[j];
	delete [] bestRoutes;
	delete [] routeScores;

	return retNode;
}

int ChartDecoder::getTreeDepth(ParseNode* node) {
	if (node == 0)
		return 0;	
	ParseNode *iter = node->headNode;
	int depth = getTreeDepth(iter);
	iter = node->premods;
	while (iter != 0) {
		depth = std::max(depth, getTreeDepth(iter));
		iter = iter->next;
	}
	iter = node->postmods;
	while (iter != 0) {
		depth = std::max(depth, getTreeDepth(iter));
		iter = iter->next;
	}
	return depth + 1;
}


void ChartDecoder::cleanupChart(int length)
{
    for (int i = 0; i < length; i++) {
        for (int j = i; j < length; j++) {
			int k = 0;
            for (ChartEntry** p = chart[i][j]; *p; p++, k++) {
				if (k >= maxEntriesPerCell && i != j) {
					continue;
				}
				delete *p;
            }
			chart[i][j][0] = 0;
        }
    }
}

void ChartDecoder::initPunctuationUpperBound(Symbol* sentence, int length)
{
    punctuationUpperBound[length - 1] = length;
    for (int i = (length - 2); i >= 0; i--) {
        if (LanguageSpecificFunctions::isNoCrossPunctuation(sentence[i + 1]))
        {
            punctuationUpperBound[i] = i + 1;
        } else {
            punctuationUpperBound[i] = punctuationUpperBound[i + 1];
        }
    }
}

bool ChartDecoder::punctuationCrossing(size_t i, size_t j, size_t length)
{
    size_t l = punctuationUpperBound[i];
    if (l == length) {
        return false;
    }
    if (l >= (j - 1)) {
        return false;
    }
    if ((j == length) ||
        (possiblePunctuationOrConjunction[j] ||
         possiblePunctuationOrConjunction[j - 1]))
    {
        return false;
    }
    return true;
}

bool ChartDecoder::crossingConstraintViolation(int start, int end, std::vector<Constraint> & constraints)
{
	BOOST_FOREACH(Constraint constraint, constraints) {
		const int &left = constraint.left;
		const int &right = constraint.right;
		if (start < left) {
			if ((left < end) && (end <= right))
				return true;
		} else if (left < start) {
			if ((start <= right) && (right < end - 1))
				return true;
		}
	}
	return false;
}

void ChartDecoder::replaceWords(ParseNode* node,
									int& currentPosition,
									Symbol *sentence)
{
	// EMB 8/6/04: 
	// Often namefinding misses the first word of a sentence being a name. 
	// If the parser finds it as a one-word NPA and _really_ doesn't know anything
	// about the headword (so, here, we test to make sure it's not even in wordnet), 
	// we ought to change it to an NPP. The mentions stage will take care of turning
	// it into a name and typing it for us.

	//MRF 2/13/05
	//we don't want to do this in stand alone versions of the the parser, since NPP don't
	//occur in treebank
	if (currentPosition == 0 &&
		LanguageSpecificFunctions::isCoreNPLabel(node->label) &&
		node->label != LanguageSpecificFunctions::getNameLabel() &&
		node->headNode != 0 &&
		node->headNode->headNode != 0 &&
		node->headNode->headNode->headNode == 0 &&
		node->postmods == 0 &&
		node->premods == 0 &&
		!vocabularyTable->find(sentence[currentPosition]) &&
		LanguageSpecificFunctions::isTrulyUnknownWord(sentence[currentPosition]) &&
		!LanguageSpecificFunctions::isStandAloneParser())
	{
		const Symbol &lcHW = LanguageSpecificFunctions::getSymbolForParseNodeLeaf(sentence[currentPosition]);
		if (!vocabularyTable->find(lcHW)) {
			node->label = LanguageSpecificFunctions::getNameLabel();
		}
	}

    if (node->headNode == 0) {
		node->label = LanguageSpecificFunctions::getSymbolForParseNodeLeaf(sentence[currentPosition++]);
    } else {		
        replaceWordsInPremods(node->premods, currentPosition, sentence);
        replaceWords(node->headNode, currentPosition, sentence);
        replaceWordsInPostmods(node->postmods, currentPosition, sentence);
   }
}

void ChartDecoder::replaceWordsInPremods(ParseNode* premod, 
									  int& currentPosition,
									  Symbol *sentence)
{
    if (premod) {
       replaceWordsInPremods(premod->next, currentPosition, sentence);
       replaceWords(premod, currentPosition, sentence);
    }
}

void ChartDecoder::replaceWordsInPostmods(ParseNode* postmod, 
									   int& currentPosition,
									   Symbol *sentence)
{
    if (postmod) {
       replaceWords(postmod, currentPosition, sentence);
       replaceWordsInPostmods(postmod->next, currentPosition, sentence);
    }
}

void ChartDecoder::postprocessParse(ParseNode* node,
									std::vector<Constraint> & constraints,
									bool collapseNPlabels)
{
    if (node->headNode != 0) {
		LanguageSpecificFunctions::modifyParse(node);
		insertNestedNameNodes(node, constraints);
		if (collapseNPlabels &&	LanguageSpecificFunctions::isNPtypeLabel(node->label)) {
			node->label = LanguageSpecificFunctions::getNPlabel();
        }
        postprocessPremods(node->premods, constraints, collapseNPlabels);
        postprocessParse(node->headNode, constraints, collapseNPlabels);
        postprocessPostmods(node->postmods, constraints, collapseNPlabels);
   }
}

void ChartDecoder::postprocessPremods(ParseNode* premod,
									  std::vector<Constraint> & constraints,
									  bool collapseNPlabels)
{
    if (premod) {
       postprocessPremods(premod->next, constraints, collapseNPlabels);
       postprocessParse(premod, constraints, collapseNPlabels);
    }
}

void ChartDecoder::postprocessPostmods(ParseNode* postmod,
									   std::vector<Constraint> & constraints,
									   bool collapseNPlabels)
{
    if (postmod) {
       postprocessParse(postmod, constraints, collapseNPlabels);
       postprocessPostmods(postmod->next, constraints, collapseNPlabels);
    }
}

void ChartDecoder::insertNestedNameNodes(ParseNode* node, 
									  std::vector<Constraint> & constraints) 
{	
	if (node->chart_start_index == -1 || node->chart_end_index == -1) {
		SessionLogger::dbg("insertNestedNameNodes") << node->toDebugString() << "(chart_start_index = " << node->chart_start_index << ", chart_end_index = " << node->chart_end_index << ")\n";
		throw InternalInconsistencyException("ChartDecoder::insertNestedNameNodes()", "Node's chart_start_index and/or chart_end_index not initialized properly.");
	}

	if (node->isName && node->label != ParserTags::LIST) {
		BOOST_FOREACH(Constraint constraint, constraints) {
			if (constraint.type == ParserTags::NESTED_NAME_CONSTRAINT &&
				node->chart_start_index <= constraint.left &&
				node->chart_end_index >= constraint.right)
			{
				ParseNode* headWord = node->headNode->headNode;
				if (constraint.right == headWord->chart_end_index) {
					// last token included in nested name 
					// create a new node for the nested name
					ParseNode* nestedNode = _new ParseNode(LanguageSpecificFunctions::getNameLabel());
					nestedNode->headNode = node->headNode;
					nestedNode->chart_start_index = node->headNode->chart_start_index;
					nestedNode->chart_end_index = node->headNode->chart_end_index;
					node->headNode = nestedNode;
					if (constraint.left < headWord->chart_start_index) {
						ParseNode* iterator = node->premods;
						while (iterator != 0 && constraint.left < iterator->chart_start_index) {
							iterator = iterator->next;
						}
						if (iterator == 0) {
							// Bonan: this needs to be fixed
							cout << "fail-1\t" << node->toDebugString() 
								<< "(node->chart_start_index = " << node->chart_start_index << ", node->chart_end_index = " << node->chart_end_index 
								<< ", (nestedNode->chart_start_index = " << nestedNode->chart_start_index << ", nestedNode->chart_end_index = " << nestedNode->chart_end_index
								<< ", (constraint.left = " << constraint.left << ", constraint.right = " << constraint.right << "\n";
							continue;

							// throw InternalInconsistencyException(
							//	"ChartDecoder::insertNestedNameNodes()",
							//	"Nested name is not a child of an existing parse constituent.");
						}
						ParseNode* firstNestedToken = iterator;
						nestedNode->premods = node->premods;
						nestedNode->chart_start_index = firstNestedToken->chart_start_index;
						node->premods = firstNestedToken->next;
						firstNestedToken->next = 0;
					}

				} else {
					// walk through premods to find last token in the nested name
					// keep track of previous pointer too
					ParseNode* prevIterator = node;
					ParseNode* iterator =  node->premods;
					while (iterator != 0 && constraint.right < iterator->headNode->chart_end_index) {
						prevIterator = iterator;
						iterator = iterator->next;
					}
					if (iterator == 0) {
						// Bonan: this needs to be fixed
						cout << "fail-2\t" << node->toDebugString() 
							<< "(node->chart_start_index = " << node->chart_start_index << ", node->chart_end_index = " << node->chart_end_index
							<< ",(constraint.left = " << constraint.left << ", constraint.right = " << constraint.right << "\n";
						continue;

						// throw InternalInconsistencyException(
						//	"ChartDecoder::insertNestedNameNodes()",
						//	"Nested name is not a child of an existing parse constituent.");
					}
					ParseNode* lastNestedToken = iterator;
					// continue walking through premods to find first token in nested name
					while (iterator != 0 && constraint.left < iterator->headNode->chart_start_index) {
						iterator = iterator->next;	
					}
					if (iterator == 0) {
						// Bonan: this needs to be fixed
						cout << "fail-3\t" << node->toDebugString()
							<< "(node->chart_start_index = " << node->chart_start_index << ", node->chart_end_index = " << node->chart_end_index
							<< ", (constraint.left = " << constraint.left << ", constraint.right = " << constraint.right << "\n";
						continue;

						// throw InternalInconsistencyException(
						//	"ChartDecoder::insertNestedNameNodes()",
						//	"Nested name is not a child of an existing parse constituent.");
					}
					ParseNode* firstNestedToken = iterator;

					// create a new node for the nested name
					ParseNode* nestedNode = _new ParseNode(LanguageSpecificFunctions::getNameLabel());
					nestedNode->headNode = lastNestedToken;
					nestedNode->chart_start_index = firstNestedToken->chart_start_index;
					nestedNode->chart_end_index = lastNestedToken->chart_end_index;
					if (firstNestedToken != lastNestedToken) {
						nestedNode->premods = lastNestedToken->next;
						lastNestedToken->next = 0;
					}
					nestedNode->next = firstNestedToken->next;
					firstNestedToken->next = 0;

					// check whether we're replacing node->premod
					if (prevIterator == node) {
						node->premods = nestedNode;
					} else {
						prevIterator->next = nestedNode;
					}
				}
			}
		}
	}
}

bool ChartDecoder::chartHasAdverbPOS(int nindex)
{
	for (int i=0; chart[nindex][nindex][i]; i++){
			const Symbol &pos = chart[nindex][nindex][i]->headTag;
			if (LanguageSpecificFunctions::isAdverbPOSLabel(pos)){
			return true;
			}
	}	
	return false;
}
bool ChartDecoder::chartHasOnlyAdverbPOS(int nindex)
{
	bool hasAdv = false;
	for (int i=0; chart[nindex][nindex][i]; i++){
			const Symbol &pos = chart[nindex][nindex][i]->headTag;
			if (LanguageSpecificFunctions::isAdverbPOSLabel(pos))
				hasAdv = true;
			else return false;
	}	
	return hasAdv;
}
bool ChartDecoder::chartHasVerbPOS(int nindex)
{
	for (int i=0; chart[nindex][nindex][i]; i++){
		const Symbol &pos = chart[nindex][nindex][i]->headTag;
		if (LanguageSpecificFunctions::isVerbPOSLabel(pos)){
			return true;
		}
	}
	return false;
}
bool ChartDecoder::chartHasPronounPOS(int nindex)
{
	for (int i=0; chart[nindex][nindex][i]; i++){
		const Symbol &pos = chart[nindex][nindex][i]->headTag;
		if (LanguageSpecificFunctions::isPronounPOSLabel(pos)){
			return true;
		}
	}	
	return false;
}
bool ChartDecoder::chartHasGeneralPrepositionPOS(int nindex)
{
	for (int i=0; chart[nindex][nindex][i]; i++){
			const Symbol &pos = chart[nindex][nindex][i]->headTag;
			if (LanguageSpecificFunctions::isPreplikePOSLabel(pos)){
			return true;
			}
	}	
	return false;
}
bool ChartDecoder::chartHasOnlyGeneralPrepositionPOS(int nindex)
{
	bool hasPP = false;
	for (int i=0; chart[nindex][nindex][i]; i++){
			const Symbol &pos = chart[nindex][nindex][i]->headTag;
			if (LanguageSpecificFunctions::isPreplikePOSLabel(pos)){
				hasPP = true;;
			}else {
				return false;
			}
	}	
	return hasPP;
}
bool ChartDecoder::chartHasParticlePOS(int nindex)
{
	for (int i=0; chart[nindex][nindex][i]; i++){
			const Symbol &pos = chart[nindex][nindex][i]->headTag;
			if (LanguageSpecificFunctions::isParticlePOSLabel(pos)){
			return true;
			}
	}	
	return false;
}
void ChartDecoder::startClock() {
	_startTime = clock();
}

bool ChartDecoder::timedOut(Symbol *sentence, int length) {
	clock_t diff = clock() - _startTime;
	if (clock() - _startTime > MAX_CLOCKS) {
		std::wstringstream errMsg;
		errMsg << "The syntactic parser timed out, and a flat parse is being returned. "
			<< "The analysis for this sentence will contain named entities but not entity descriptions "
			<< "(e.g. 'the president') or most pronouns. Most relations and events will also be omitted. "
			<< "This behavior typically occurs when a sentence is either very long or contains an unusual arrangement "
			<< "of tokens (e.g. excessive punctuation), causing the parser to perform particularly inefficiently. "
			<< "The (tokenized) text of the timed-out sentence was:";
		for (int i = 0; i < length; i++) {
			errMsg << L" " << sentence[i];
		}
		SessionLogger::warn_user("parser_timeout") << errMsg.str();
		return true;
	} else return false;
}

ParseNode *ChartDecoder::getDefaultParse(Symbol* sentence, int length, std::vector<Constraint> & constraints) {
	if (length == 1) {
		BOOST_FOREACH(Constraint constraint, constraints) {
			const Symbol &type = constraint.type;
			if (type == ParserTags::HEAD_CONSTRAINT) 
				continue;

			// the constraint is a name constraint
			if (constraint.left != 0 || constraint.right != 0) 
				continue;

			// the one word in the sentence is a name
			ParseNode* tree = _new ParseNode(LanguageSpecificFunctions::getNameLabel(), constraint.left, constraint.right);
			tree->headNode = _new ParseNode(LanguageSpecificFunctions::getProperNounLabel(), constraint.left, constraint.right);
			tree->headNode->headNode = _new ParseNode(sentence[0], constraint.left, constraint.right);
			return tree;
		}
	}
	
	ParseNode* tree = _new ParseNode(ParserTags::FRAGMENTS, 0, length-1);
	int count = 0;
	
	// set tree head
	while (tree->headNode == 0 && count < length) {
		tree->headNode = getBestFragment(0,count);
		count++;
	}
	if (tree->headNode == 0){
		delete tree;
		return getCompletelyDefaultParse(sentence, length);
	}
	ParseNode* placeholder = tree;
	int start = count;
	count = 0;

	// set tree postmods
	if (start < length) {
		while (tree->postmods == 0 && start + count < length) {
			tree->postmods = getBestFragment(start,start + count);
			count++;
		}
		placeholder = tree->postmods;
	} else return tree;
	if (tree->postmods == 0){
		delete tree;
		return getCompletelyDefaultParse(sentence, length);
	}

	// do the rest
	start = start + count;
	for (int j = start; j < length; ) {
		count = 0;
		while (placeholder->next == 0) {

			// should never happen, if all is sane
			// (one thing that could cause insanity is hyphen constraints, however...)
			// BUG FIX EMB 1/31/06: this needs to be a >=
			if (j + count >= length) {
				delete tree;
				return getCompletelyDefaultParse(sentence, length);
			}

			placeholder->next = getBestFragment(j, j + count);
			count++;
		}
		j = j + count;
		placeholder = placeholder->next;
	}
	return tree;
}

ParseNode* ChartDecoder::getBestFragment(int start,
									     int end)
{
    if (chart[start][end][0] != 0) {
		int highestScoring = 0;
		for (int i = 1; chart[start][end][i] != 0; i++) {
			if (
#ifdef PARSER_FAST_FLOATING_POINT_COMPARISON
			    chart[start][end][i]->rankingScore >
				chart[start][end][highestScoring]->rankingScore
#else
			    __fcmp (chart[start][end][i]->rankingScore,
				    chart[start][end][highestScoring]->rankingScore) > 0
#endif
			    )
			{
				highestScoring = i;
			}
		}
		ParseNode* tree = chart[start][end][highestScoring]->toParseNode();

		return tree;
	}
	
	return static_cast<ParseNode*>(0);
}

ParseNode* ChartDecoder::getCompletelyDefaultParse(Symbol* sentence, int length) {
	ParseNode* tree = _new ParseNode(ParserTags::FRAGMENTS, 0, length-1);
	tree->headNode = _new ParseNode(ParserTags::FRAGMENTS, 0, length-1);
	tree->headNode->headNode = _new ParseNode(sentence[0], 0, 0);
	if (length > 1) {
		tree->postmods = _new ParseNode(ParserTags::FRAGMENTS, 1, 1);
		tree->postmods->headNode = _new ParseNode(sentence[1], 1, 1);
		ParseNode *placeholder = tree->postmods;
		for (int j = 2; j < length; j++) {
			placeholder->next = _new ParseNode(ParserTags::FRAGMENTS, j, j);
			placeholder->next->headNode = _new ParseNode(sentence[j], j, j);
			placeholder = placeholder->next;
		}
	}
	return tree;
}

//added as confidence measure

float ChartDecoder::getProbabilityOfWords(Symbol* sentence, int length)
{
	float total_score = 0;
	for (int i = 0; i < length; i++) {
		float score = wordProbTable->lookup(&sentence[i]);
		if (score == 0) {
			Symbol word = wordFeatures->features(sentence[i], i == 0);
			score = wordProbTable->lookup(&word);
			if (score == 0) {
				Symbol word = wordFeatures->reducedFeatures(sentence[i], i == 0);
				score = wordProbTable->lookup(&word);
			}
		}
		total_score += score;
    }

	return total_score;

}

void ChartDecoder::writeCaches() {
  const char* case_tag = decoderTypeString();
  headProbs->writeCache(case_tag);
  premodProbs->writeCache(case_tag);
  postmodProbs->writeCache(case_tag);
  leftLexicalProbs->writeCache(case_tag);
  rightLexicalProbs->writeCache(case_tag);
}

void ChartDecoder::readCaches() {
  const char* case_tag = decoderTypeString();
  headProbs->readCache(case_tag);
  premodProbs->readCache(case_tag);
  postmodProbs->readCache(case_tag);
  leftLexicalProbs->readCache(case_tag);
  rightLexicalProbs->readCache(case_tag);
}

void ChartDecoder::cleanup() {
	headProbs->clearCache();
	premodProbs->clearCache();
	postmodProbs->clearCache();
	leftLexicalProbs->clearCache();
	rightLexicalProbs->clearCache();
	for (int i=0; i<maxEntriesPerCell; ++i) 
		theory_sc_strings[i].clear(); // make sure memory is freed.
	simpleCache.clear();
#if !defined(_WIN32) && !defined(__APPLE_CC__)
	lruCache.clear();
#endif
}

const char* ChartDecoder::decoderTypeString() {
  switch (DECODER_TYPE) {
    case UPPER: return "upper";
    case LOWER: return "lower";
    default: return "mixed";
  }
}
                                                                                                                      
