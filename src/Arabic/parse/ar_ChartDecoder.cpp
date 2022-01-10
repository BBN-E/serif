// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Arabic/parse/ar_ChartDecoder.h"
#include "Arabic/parse/ar_WordSegment.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Arabic/parse/ar_ChartEntry.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include "Arabic/common/ar_StringTransliterator.h"

#include <stdio.h>
#include <iostream>
#include "math.h"
char str[10000];
int buf_len = 10000;

/* return a parse that is (FRAG w w w w) except for Names, which have NPPs
*/
ParseNode* ArabicChartDecoder::_makeFlatParse(Symbol* sentence, int length,
    Constraint* _constraints, int _numConstraints){
	
		ParseNode* flat_parse = new ParseNode(ArabicSTags::FRAG);
		int word = 0;
		int curr_const =0;
		flat_parse->headNode = new ParseNode();
		ParseNode* currNode = flat_parse->headNode;
		ParseNode* lastNode;
		if(_numConstraints >0){
			if(_constraints[0].left==0){
				//add a NPP
				addNPPNode(currNode, _constraints[0], sentence);
				word = word+_constraints[0].right-_constraints[0].left+1;
				curr_const++;
			}
			else{
				addWordNode(currNode, sentence[word]);
				word++;
			}
		}
		else{
			addWordNode(currNode, sentence[word]);
			word++;
		}
		//ParseNode* currNode = flat_parse->postmods;
		flat_parse->postmods = new ParseNode();
		currNode = flat_parse->postmods;

		while(word<length){
			if((curr_const < _numConstraints) && (_constraints[curr_const].left == word)){
				addNPPNode(currNode, _constraints[curr_const], sentence);
				word = word+_constraints[curr_const].right-_constraints[curr_const].left+1;
				curr_const++;
			}
			else{
				addWordNode(currNode, sentence[word]);
				word++;
			}
			currNode->next = new ParseNode();
			lastNode = currNode;
			currNode= currNode->next;

		}
		lastNode->next = NULL;
		delete currNode;
		highest_scoring_final_theory = 0;
		theory_scores[0] = .1f;
		return flat_parse;
	}
void ArabicChartDecoder::addWordNode(ParseNode* currNode, Symbol word){
	currNode->label = ArabicSTags::UNKNOWN;
	currNode->headNode = new ParseNode(word);
}

void ArabicChartDecoder::addNPPNode(ParseNode* currNode, Constraint constraint, Symbol* sentence){
	currNode->label = ArabicSTags::NPP;
	currNode->headNode = new ParseNode(LanguageSpecificFunctions::getDefaultNamePOStag(constraint.entityType));
	ParseNode* p = currNode->headNode;
	p->headNode=new ParseNode(sentence[constraint.left]);
	if(constraint.left != constraint.right){
		currNode->postmods = new ParseNode(LanguageSpecificFunctions::getDefaultNamePOStag(constraint.entityType));
		p = currNode->postmods;
	
		for(int i =constraint.left+1; i<constraint.right; i++){
			p->headNode = new ParseNode(sentence[i]);

			p->next =  new ParseNode(LanguageSpecificFunctions::getDefaultNamePOStag(constraint.entityType));
			p = p->next;
		}
		p->headNode = new ParseNode(sentence[constraint.right]);
	}
}



/* decode a sentence using bw derived tokenization */
ParseNode* ArabicChartDecoder::decode(SplitTokenSequence* bw_tokens, bool collapseNPlabels)
{
	// make sure that the maximally segmented version still fits in the chart
	// -- SRS
	WordSegment seg;
//	std::cout<<"D 1"<<std::endl;
	int num_segments = 0;
	for (int i = 0; i < bw_tokens->getNTokens(); i++)
		num_segments += bw_tokens->getWordSegment(i)->maxLength();
//	std::cout<<"D 2"<<std::endl;

	if (num_segments >= MAX_SENTENCE_LENGTH) {
		SessionLogger::logger->beginWarning();
		*SessionLogger::logger << "Using a flat parse because there are"
								  "too many possible segments.\n";
		//Returning an empty parse here leaves us without a name theory
		//instead build a flat "FRAGMENT" parse with NPP for names
		throw UnexpectedInputException(
			"ArabicChartDecoder::Decode()","SplitTokenSequence has too many segments");

	}
	if(bw_tokens ==NULL){
		std::cerr<<"bw_tokens is NULL"<<std::endl;
	}
	if(bw_tokens->getWordSegment(bw_tokens->getNTokens()-1) ==NULL){
		std::cerr<<"bw_tokens last word is NULL"<<
			bw_tokens->getNTokens()<<" tokens "<<
			std::endl;
	}

	if(bw_tokens->getWordSegment(bw_tokens->getNTokens()-1)->getPossibility(0).segments ==NULL){
		std::cerr<<"bw_tokens last word first segment segments is NULL"
			<<bw_tokens->getNTokens()<< " tokens "<<std::endl;
	}

	bool lastIsPunct = LanguageSpecificFunctions::isSentenceEndingPunctuation(bw_tokens->getWordSegment(
		bw_tokens->getNTokens()-1)->getPossibility(0).segments[0]->getText());
//	std::cout<<"D 3"<<std::endl;

	Symbol split_sentence[MAX_SENTENCE_LENGTH];
	int chart_size = initChart(bw_tokens, split_sentence);
//	std::cout<<"D 4"<<std::endl;

	initPunctuationUpperBound(split_sentence, chart_size);
//	std::cout<<"D 5"<<std::endl;
//	std::cout<<"chart_size: "<<chart_size<<" \n";
	for (int span = 2; span <= chart_size; span++) {
		for (int start = 0; start <= (chart_size - span); start++) {
			int end = start + span;
			numTheories = 0;
			if(DEBUG1){
				_debugStream <<"---------start: "<<start<<" end: "<<end<<"\n";
				_debugStream.flush();
			}
			for (int mid = (start + 1); mid < end; mid++) {
				if ((end == chart_size) && lastIsPunct &&
					!((mid == (chart_size - 1)) && (start == 0))) {
					continue;
					}
				if (punctuationCrossing(start, end, chart_size)) {
					continue;
				}
				leftClosable = !crossingConstraintViolation(start, mid);
				rightClosable = !crossingConstraintViolation(mid, end);
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
	postProcessChart();
	ParseNode* returnValue = getBestParse(finalScore, 0, chart_size, false);
//	std::cout<<"D 6"<<std::endl;

	
	// diversity parsing iteration
	ParseNode* iterReturnValue = returnValue;
	while (iterReturnValue != 0) {
		int replacementPosition = 0;
		postprocessParse(iterReturnValue, collapseNPlabels);
		addNameToParse(iterReturnValue, replacementPosition);
		iterReturnValue = iterReturnValue->next;
	}
//	std::cout<<"D 7"<<std::endl;

	cleanupChart(chart_size);
//	std::cout<<"D 8"<<std::endl;
/*	if(returnValue == NULL){
		std::cout<<"EMPTY RETURN!!!\n";
	}
	else{
		std::cout<<"RETURN VALUE EXISTS !! "<<finalScore<<"\n";
	}
*/
	//if(DEBUG2){
	//	_parseStream << returnValue->toWString();
	//	_parseStream <<"\n\n";
	//}
	std::wstring resultNoSpace = returnValue->readLeaves();
//	std::cout<<"D 9"<<std::endl;

	std::wstring sentNoSpace = L"";
	for(int i =0; i< bw_tokens->getNTokens(); i++){
		sentNoSpace += bw_tokens->getWordSegment(i)->getOriginalWord().to_string();
	}
//	std::cout<<"D 10"<<std::endl;

	if(wcscmp(resultNoSpace.c_str(),sentNoSpace.c_str())!=0){
		std::cerr<<"\nToken Sequence After Parsing is wrong Because of Marjorie\n";
		Symbol sns = Symbol(sentNoSpace.c_str());
		Symbol rns = Symbol(resultNoSpace.c_str());
		std::cerr<<"Initial: "<<sns.to_debug_string()<<"\n";
		std::cerr<<"After:   "<<rns.to_debug_string()<<"\n";
	}


	return returnValue;
}
	




ParseNode* ArabicChartDecoder::decode(Symbol* sentence, int length,
    Constraint* _constraints, int _numConstraints,
	bool collapseNPlabels)
{
	// make sure that the maximally segmented version still fits in the chart
	// -- SRS
	
	WordSegment seg;
	int max_segments = 0;
	for (int i = 0; i < length; i++)
		max_segments += seg.getMaxSegments(sentence[i].to_string());
	if( (max_segments >= MAX_SENTENCE_LENGTH) ) {
	//if( (max_segments >=5)){
		SessionLogger::logger->beginWarning();
		*SessionLogger::logger << "Using a flat parse because there are"
								  "too many possible segments.\n";
		//Returning an empty parse here leaves us without a name theory
		//instead build a flat "FRAGMENT" parse with NPP for names
		ParseNode* flatParse =  _makeFlatParse(sentence, length, _constraints, _numConstraints);
		return flatParse;
	}
	constraints = _constraints;
	numConstraints = _numConstraints;
	bool lastIsPunct = LanguageSpecificFunctions::isSentenceEndingPunctuation(sentence[length - 1]);
	Symbol split_sentence[MAX_SENTENCE_LENGTH];
	int chart_size = initChart(sentence, split_sentence, length);
	initPunctuationUpperBound(split_sentence, chart_size);

	for (int span = 2; span <= chart_size; span++) {
		for (int start = 0; start <= (chart_size - span); start++) {
			int end = start + span;
			numTheories = 0;
			for (int mid = (start + 1); mid < end; mid++) {
				if ((end == chart_size) && lastIsPunct &&
					!((mid == (chart_size - 1)) && (start == 0))) {
					continue;
				}
				if (punctuationCrossing(start, end, chart_size)) {
					continue;
				}
				leftClosable = !crossingConstraintViolation(start, mid);
				rightClosable = !crossingConstraintViolation(mid, end);
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
	postProcessChart();
	ParseNode* returnValue = getBestParse(finalScore, 0, chart_size, false);
	if(returnValue == NULL){
		SessionLogger::logger->beginWarning();
		*SessionLogger::logger << "Using a flat parse because parser failed\n";
		ParseNode* flatParse = _makeFlatParse(sentence, length, _constraints, _numConstraints);
		return flatParse;
	}

		
	
	// diversity parsing iteration
	ParseNode* iterReturnValue = returnValue;
	while (iterReturnValue != 0) {
		int replacementPosition = 0;
		postprocessParse(iterReturnValue, collapseNPlabels);
		addNameToParse(iterReturnValue, replacementPosition);
		iterReturnValue = iterReturnValue->next;
	}
	
	cleanupChart(chart_size);
	//if(DEBUG2){
	//	_parseStream << returnValue->toWString();
	//	_parseStream <<"\n\n";
	//}

	std::wstring resultNoSpace = returnValue->readLeaves();
	std::wstring sentNoSpace = L"";
	for(int i =0; i< length; i++){
		sentNoSpace += sentence[i].to_string();
	}
	if(wcscmp(resultNoSpace.c_str(),sentNoSpace.c_str())!=0){
		std::cerr<<"\nToken Sequence After Parsing is wrong Because of Marjorie\n";
		Symbol sns = Symbol(sentNoSpace.c_str());
		Symbol rns = Symbol(resultNoSpace.c_str());
		std::cerr<<"Initial: "<<sns.to_debug_string()<<"\n";
		std::cerr<<"After:   "<<rns.to_debug_string()<<"\n";
	}


	return returnValue;
}



/* For now ignore constraints....
*  This is only used by the standalone parser when reading in BW input 
*
*/
int ArabicChartDecoder::initChart(SplitTokenSequence* words, Symbol* split_sentence){

	int m = 0;			//m is the index into the original sentence
	int chart_index = 0;  //chart index is the current position in the chart

	while(m < words->getNTokens()){


		
		WordSegment* word_parts = words->getWordSegment(m);
		//std::cout<<"Initialize: ";
		//word_parts->printSegment();
		//std::cout<<std::endl;
		WordAnalysis longword = word_parts->getLongest(); 
		for(int i = 0; i<word_parts->maxLength(); i++){
			split_sentence[chart_index+i] = longword.segments[i]->getText();
		}
		chart_index = initChartWordMultiple(word_parts, chart_index, m ==0);
		//chart_index+=word_parts->maxLength();
		m++;
		//delete word_parts;	//reset would be better!
	}
	return chart_index;
}

/* simultaneously add constraints and morph analysis 
	returns the maximum number of tokens in the chart
*/
int ArabicChartDecoder::initChart(Symbol* sentence,  Symbol* split_sentence, int length)
{	
	//initialize entire chart to 0 in preparation of post processing
	/*
	for(int i = 0; i < MAX_SENTENCE_LENGTH; i++){
		for(int j = 0; j< MAX_SENTENCE_LENGTH; j++){
			for(int k = 0; k<MAX_ENTRIES_PER_CELL; k++){
				chart[i][j][k] = 0;
			}
		}
	} 
	*/
	//Assume constraints are in increasing order ?
	int curr_const = 0;
	int left = -1, right = -1;
	Symbol type;
	EntityType entityType;
	if(numConstraints >0){
		left = constraints[curr_const].left;
		right = constraints[curr_const].right;
		type = constraints[curr_const].type;
		entityType = constraints[curr_const].entityType;
	}

	int chart_index = 0;  //chart index is the current position in the chart
	_numNameWords = 0;

	int m = 0;			//m is the index into the original sentence
	while(m < length){
		if(m ==left){
			int name_len = right - left;
			if(DEBUG1)
				_debugStream<<"Adding Name Tokens: ";
			int i = left;
			int k = 0;
			for(; i<=right; i++, k++){
				//for postprocessing
				_nameWords[_numNameWords++] =sentence[i];
				split_sentence[chart_index+k] = sentence[i];
				//debugging
				if(DEBUG1){
					Symbol debugSym = ArabicSymbol::BWSymbol(sentence[i].to_string());
					_debugStream<<debugSym.to_debug_string()<<" ";
				}
			}
			if(DEBUG1){
				_debugStream<<"\n";
			}

			addConstraintEntry(left, right,chart_index, chart_index + name_len, sentence, type, entityType);
			m = right +1;
			curr_const++;
			if(curr_const < numConstraints){
				left = constraints[curr_const].left;
				right = constraints[curr_const].right;
				type = constraints[curr_const].type;
			}
			chart_index = chart_index+name_len +1;
		}
		else{
			if(DEBUG1){
				Symbol debugSym = ArabicSymbol::BWSymbol(sentence[m].to_string());
				_debugStream<<"Adding Token: "<<m<<" "<<debugSym.to_debug_string()<<"\n";
			}

			WordSegment* word_parts = new WordSegment();
			word_parts->getSegments(sentence[m]);
			WordAnalysis longword = word_parts->getLongest(); 
			for(int i = 0; i<word_parts->maxLength(); i++){
				split_sentence[chart_index+i] = longword.segments[i]->getText();
			}
			chart_index = initChartWordMultiple(word_parts, chart_index, m ==0);
			//chart_index+=word_parts->maxLength();
			m++;
			delete word_parts;	//reset would be better!
		}
	}
	return chart_index;
}
int ArabicChartDecoder::initChartWordMultiple(const WordSegment* word, 
											   int chart_index, bool firstWord){


	const int maxSegs = word->maxLength();
	int currentEntries[MAX_ENTRIES_PER_CELL][MAX_ENTRIES_PER_CELL];
	for (int a = 0; a < maxSegs+1; a++) {
		possiblePunctuationOrConjunction[chart_index + a] = false;
		for (int b = 0; b < maxSegs+1; b++) {
			currentEntries[a][b] = 0;
		}
	}
	int next_chart_index = chart_index+1;
	for(int j = 0; j< word->getNumPossibilities(); j++){
		//std::cout<<"InitChartMultiple word: "<<j<<std::endl;
		WordAnalysis segs = word->getPossibility(j);
		int numSegs = segs.numSeg;
		Segment* thisSeg;
		for(int k = 0; k< numSegs; k++){
			thisSeg = segs.segments[k];
			Symbol seg_word = thisSeg->getText();
			Symbol original_seg = seg_word;
			int numTags = 0;    
			const Symbol* tags;
			if (vocabularyTable->find(seg_word)) {
				tags = partOfSpeechTable->lookup(seg_word, numTags);
			} else {
				seg_word = wordFeatures.features(original_seg, firstWord);
				tags = partOfSpeechTable->lookup(seg_word, numTags);
				if (numTags == 0) {
					seg_word = wordFeatures.reducedFeatures(original_seg, firstWord);
					tags = partOfSpeechTable->lookup(seg_word, numTags);
				}
			}
			bool foundValid = false;
			for(int m = 0; m <numTags; m++){
				if(thisSeg->isValidPOS(tags[m])){
					foundValid = true;
					break;
				}
			}
			//WARNING: This is very much a hack, if we separate a pronoun
			//that we have not seen in training, we will be unable to give it
			//a pronoun POS tag, b/c pronouns are not open class.
			//This is only a problem for Pronouns! we see enough of all of the 
			//prefixes in training
			//In this case switch Tags = 3MascSing pron suffix tags
			//and print an error msg to STD::ERR
			bool substitution = false;
			if(!foundValid){
				tags = thisSeg->validPOSList();
				numTags = thisSeg->posListSize();
				substitution = true;
				if(numTags == 4){
					std::cerr<<"Substituting pron taglist for unknown pronoun"<<std::endl;
				}
				else{
					std::cerr<<"Substituting taglist for non pronoun"<<std::endl;
				}
			}
			//add the segment to clean words for postprocessing
			_cleanWords[chart_index+thisSeg->getStart()][chart_index+thisSeg->getEnd()] = 
				thisSeg->getText();

			for (int m = 0; m < numTags; m++) {
				Symbol tag = tags[m];
				//ignore invalid POS tags
				if(thisSeg->isValidPOS(tag)){
					if (LanguageSpecificFunctions::isBasicPunctuationOrConjunction(tag))
					{
						possiblePunctuationOrConjunction[chart_index+k] = true;
					}				
					ChartEntry *entry = _new ChartEntry();
					entry->nameType = ParserTags::nullSymbol;
					int start = thisSeg->getStart();
					int end = thisSeg->getEnd();
				
	///				FOR DEBUGGING MSG
					if(DEBUG1){
						Symbol debugSym = ArabicSymbol::BWSymbol(original_seg.to_string());
						_debugStream<<"segment "<<debugSym.to_debug_string();
						_debugStream<<" tag "<<tag.to_debug_string();
						int p = start+ chart_index;
						int q = end+chart_index;
						int r = currentEntries[start][end];
						_debugStream<<"Chart Place "<<p<<", "<<q<<", "<<r<<"\n";
						_debugStream.flush();
						//std::cout<<"segment "<<debugSym.to_debug_string();
						//std::cout<<" tag "<<tag.to_debug_string();
						//std::cout<<"Chart Place "<<p<<", "<<q<<", "<<r<<"\n";
					}				
	///				END DEBUGGING MSG
					fillWordEntry(entry, tag, seg_word, original_seg, chart_index+start-1, chart_index+end+1);
					//The nasty pronoun hack continues, if we set the prononun substitution, also need to set the ranking
					//score to 1/numposs
					if(substitution)
						entry->rankingScore= log(1.0f / numTags);
					//debugging
					/*
						Symbol debugSym = ArabicSymbol::BWSymbol(original_seg.to_string());
						//_debugStream<<"segment "<<debugSym.to_debug_string();
						//_debugStream<<" tag "<<tag.to_debug_string();
						int p = start+ chart_index;
						int q = end+chart_index;
						int r = currentEntries[start][end];


						std::cout<<"segment "<<debugSym.to_debug_string();
						std::cout<<" tag "<<tag.to_debug_string();
						std::cout<<"Chart Place "<<p<<", "<<q<<", "<<r<<"\n";
						std::cout<<"RankingScore "<<entry->rankingScore<<std::endl;
					*/
					//end debugging

					//end hack
					chart[chart_index+start][chart_index+end][currentEntries[start][end]++] = entry;
					//always make the next entry 0?
					chart[chart_index+start][chart_index+end][currentEntries[start][end]] = 0;
					if(( chart_index+end +1) >next_chart_index){
						next_chart_index =  chart_index+end+1;
					}
				}
			}
		}
	}
	for (int k = 0; k < maxSegs; k++) {
	   chart[chart_index+k][chart_index+k][currentEntries[k][k]] = 0;
	}
	return next_chart_index;
}


//This is mostly the same as the generic method, 
//left, right_word refer to locations in sentence
//left, right_chart refer to placement in the chart
//in the generic version these are the same
void ArabicChartDecoder::addConstraintEntry(int left_word, int right_word,
									 int left_chart, int right_chart,
									 Symbol* sentence, Symbol type, EntityType entityType) {
	
	Symbol word = sentence[right_word];
	const Symbol* tags;
	int numTags;
	bool is_unknown_word = false;
	if (vocabularyTable->find(word)) {
		tags = partOfSpeechTable->lookup(word, numTags);
	} else {
		is_unknown_word = true;
		word = wordFeatures.features(sentence[right_word], 0);
		tags = partOfSpeechTable->lookup(word, numTags);
		if (numTags == 0) {
			word = wordFeatures.reducedFeatures(sentence[right_word], 0);
			tags = partOfSpeechTable->lookup(word, numTags);
		}
	}
	
	unsigned index = 0;
	ChartEntry *entry;
	
	bool found_primary_tag = false;
	for (int k = 0; k < numTags; k++) {
		Symbol tag = tags[k];
		if (index < maxEntriesPerCell && 
			(!entityType.isRecognized() ||
			 LanguageSpecificFunctions::isPrimaryNamePOStag(tag, entityType)))
		{
			entry = _new ChartEntry();
			entry->nameType = type;
			fillWordEntry(entry, tag, word, sentence[right_word], left_chart, right_chart);
			if(DEBUG1){	
				_debugStream<<"Constarained Entry: "<<word.to_debug_string()<<": ";
				_debugStream<<left_chart<<", "<<right_chart<<", "<<index<<"\n";
			}
			chart[left_chart][right_chart][index++] = entry;
			found_primary_tag = true;
		}
	}
	if (index == 0) {
		ChartEntry *entry = _new ChartEntry();
		entry->nameType = type;
		// don't bother calculating real ranking score, as we're making it up
		fillWordEntry(entry, LanguageSpecificFunctions::getDefaultNamePOStag(entityType), 
			word, sentence[right_word], left_chart, right_chart, false);
		if(DEBUG1){	
			_debugStream<<"Constarained Entry: "<<word.to_debug_string()<<": ";
			_debugStream<<left_chart<<", "<<right_chart<<", "<<index<<"\n";
		}
		chart[left_chart][right_chart][index++] = entry;
	}

	// So -- now we let word vectors also pick up secondary tags.... but sometimes 
	// that leads to worse parses. So, if an unknown word vector has a primary tag 
	// in the POS table, use ONLY that.
	//
	// If we continue to expand the set of things we want to use this for (in
	// particular, non-named entities), perhaps we should make the
	// LanguageSpecificFunctions functions used here be particular to the type
	// of tag: clearly, nuclear substances should have different primary/
	// secondary/default tags than person names. For now, though, we just 
	// hope for the best.
	if (found_primary_tag && is_unknown_word)
		numTags = 0;

	for (int j = 0; j < numTags; j++) {
		Symbol tag = tags[j];
		if (LanguageSpecificFunctions::isSecondaryNamePOStag(tag, entityType) &&
			index < maxEntriesPerCell)
		{
			entry = _new ChartEntry();
			entry->nameType = type;
			fillWordEntry(entry, tag, word, sentence[right_word], left_chart, right_chart);
			if(DEBUG1){	
				_debugStream<<"Constarained Entry: "<<word.to_debug_string()<<": ";
				_debugStream<<left_chart<<", "<<right_chart<<", "<<index<<"\n";
			}
			chart[left_chart][right_chart][index++] = entry;
			break;
		}
	}
	
	chart[left_chart][right_chart][index] = 0;

}


void ArabicChartDecoder::transferTheoriesToChart(int start, int end)
{
	if (numTheories == 0) {
		//std::cout<<"NOTHING FROM "<<start<<" to "<<end<<"\n";
		return;
    }
	else if(DEBUG1){
			_debugStream<<"add theory from "<<start<<" to "<<end; 
	}
	size_t j = 0;
	// Skip preterminals already in chart 
	while (chart[start][end - 1][j] != 0) { 
		j++;
	}
	if(j > maxEntriesPerCell){
//		std::cout<<"Too many entries in a cell!"<<start<<" "<<end<<"\n";
	}

    if (numTheories == 0) {
        chart[start][end - 1][j] = 0;
        return;
    }
	
	// Remove extra theories that will not fit in chart
	while (j + numTheories > maxEntriesPerCell) {
		int lowestScoring = 0;
		for (int l = 1; l < numTheories; l++) {
			if (theories[l]->rankingScore < theories[lowestScoring]->rankingScore) {
				lowestScoring = l;
			}	
		}
		theories[lowestScoring] = theories[numTheories - 1];
		numTheories--;
	}
	
	// Find highest scoring theory to determine threshold
    int highestScoring = 0;
    for (int i = 1; i < numTheories; i++) {
        if (theories[i]->rankingScore > theories[highestScoring]->rankingScore)
            highestScoring = i;
    }
    float threshold = theories[highestScoring]->rankingScore + lambda;
    // Put theories in chart
	for (int k = 0; k < numTheories; k++) {
        if (theories[k]->rankingScore > threshold) {
            chart[start][end - 1][j++] = theories[k];
			if(DEBUG1){
				_debugStream<<"\t "<<theories[k]->constituentCategory.to_debug_string()
					<<"->"<<theories[k]->leftChild->constituentCategory.to_debug_string()
					<<" "<<theories[k]->rightChild->constituentCategory.to_debug_string()
					<<"\n";
				_debugStream.flush();
			}
			
        }else {
            delete theories[k];
        }
    }
    chart[start][end - 1][j] = 0;
}


void ArabicChartDecoder::postProcessChart(){
    for (int i = 0; i < MAX_SENTENCE_LENGTH; i++) {
		int k = 0;
		while((k < MAX_TAGS_PER_WORD) && (chart[i][i][k] != 0)){
			if((chart[i][i][k])&& 
				chart[i][i][k]->isPreterminal &&
				(chart[i][i][k]->nameType==ParserTags::nullSymbol)){
					chart[i][i][k]->headWord = _cleanWords[i][i];
				}
			k++;
		}
	}	
	for(int i = 0; i< MAX_SENTENCE_LENGTH; i++){
		for (int j = i + 1; j < MAX_SENTENCE_LENGTH; j++) {
			int  k =0;
			while((k < MAX_ENTRIES_PER_CELL) && (chart[i][j][k] != 0)){
				if((chart[i][j][k])&& 
					chart[i][j][k]->isPreterminal &&
					(chart[i][j][k]->nameType==ParserTags::nullSymbol)){
						chart[i][j][k]->headWord = _cleanWords[i][j];
					}
					k++;
			}
		}
	}
}




void ArabicChartDecoder::addNameToParse(ParseNode* node, int& currentPosition)
{	if(node->isName){
		addNameToNode(node, currentPosition);
	}
	if ((node->headNode == 0)) {
		node->label = LanguageSpecificFunctions::getSymbolForParseNodeLeaf(node->label);
		
    }
	else{
		addNameToPremods(node->premods, currentPosition);
		addNameToParse(node->headNode, currentPosition);
		addNameToPostmods(node->postmods, currentPosition);
	}
}
void ArabicChartDecoder::addNameToPremods(ParseNode* premod, int& currentPosition)
{
    if (premod) {
       addNameToPremods(premod->next, currentPosition);
       addNameToParse(premod, currentPosition);
    }
}

void ArabicChartDecoder::addNameToPostmods(ParseNode* postmod, int& currentPosition)
{
	 if (postmod) {
		addNameToParse(postmod, currentPosition);
		addNameToPostmods(postmod->next, currentPosition);
    }
}
void ArabicChartDecoder::postprocessParse(ParseNode* node, bool collapseNPlabels)
{

    if ((node->headNode == 0)) {
		node->label = LanguageSpecificFunctions::getSymbolForParseNodeLeaf(node->label);
		
    } else {
		LanguageSpecificFunctions::modifyParse(node);
		if (collapseNPlabels &&	LanguageSpecificFunctions::isNPtypeLabel(node->label)) {
			node->label = LanguageSpecificFunctions::getNPlabel();
        }
        postprocessPremods(node->premods, collapseNPlabels);
        postprocessParse(node->headNode, collapseNPlabels);
        postprocessPostmods(node->postmods, collapseNPlabels);
    }
}

void ArabicChartDecoder::addNameToNode(ParseNode* node, int& currentPosition){
	//node will be pre-preterminal name span
	//namewords need to be put in premods and headNode
	ParseNode* pm = node->premods;
	ParseNode* hn;
	//count premods
	int numPM = 0;
	while(pm){
		pm = pm->next;
		numPM++;
	}
	pm = node->premods;
	int premod_counter = numPM -1; 
	while(pm){
		hn = pm->headNode;
		if(hn->headNode==0){
			hn->label = 
			LanguageSpecificFunctions::getSymbolForParseNodeLeaf(_nameWords[currentPosition+premod_counter]);
			premod_counter--;
		}
		else{
			std::cerr<<"PROBLEM WITH NAME WORD- premods\n";
		}
		pm = pm->next;
	}
	currentPosition+=numPM;
	hn = node->headNode; //nameWords POS
	if(hn->headNode->headNode == 0){
		hn->headNode->label = 
			LanguageSpecificFunctions::getSymbolForParseNodeLeaf(_nameWords[currentPosition++]);
	}
	else{
		std::cerr<<"PROBLEM WITH NAME WORD\n";
	}
}



