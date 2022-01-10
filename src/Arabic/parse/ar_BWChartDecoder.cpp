// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/parse/ar_BWChartDecoder.h"
#include "Arabic/parse/ar_STags.h"
#include "Generic/parse/BridgeExtension.h"
#include "Generic/parse/BridgeKernel.h"
#include "Generic/parse/KernelKey.h"
#include "Generic/parse/ExtensionKey.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Entity.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/EntityType.h"

#include <boost/foreach.hpp>


/* return a parse that is (FRAG w w w w) except for Names, which have NPPs
*/
/*
ParseNode* BWArabicChartDecoder::_makeFlatParse(
	ArabicParseSeeder::SameSpanParseToken** init_segments, 
    int length, Constraint* _constraints, int _numConstraints)
{

	//Go through a procedure similar to init chart, to determine what words to use

		int split_sent_end_index[MAX_SENTENCE_LENGTH];
		ArabicParseSeeder::SameSpanParseToken* defaultParseTokens[MAX_SENTENCE_LENGTH];

		for(int i =0; i< length;  i++){
			split_sent_end_index[i] = -1;
			defaultParseTokens[i] =NULL;
		}
		// this only handles name type constraints! 
		int last_token = 0;
		for (int m = 0; m < length; m++) {
			int start = init_segments[m]->start;
			int end = init_segments[m]->end;
			if((split_sent_end_index[start] - end ) > 0){
				split_sent_end_index[start] = end;
				defaultParseTokens[start] = init_segments[m];
				if(start > last_token){
					last_token = start;
				}
			}
		}
		last_token++;
		//make the the flat parse from the defaultParseTokens
		ParseNode* flat_parse = new ParseNode(ArabicSTags::FRAG);
		int word = 0;
		int curr_const =0;
		flat_parse->headNode = new ParseNode();
		ParseNode* currNode = flat_parse->headNode;
		ParseNode* lastNode;
		//skip initial NULL words
		while((word<last_token) && defaultParseTokens[word]==NULL){
			word++;
		}
		if(_numConstraints >0){
			
			if(_constraints[0].left==defaultParseTokens[word]->start ){
				//add a NPP
				word = addNPPNode(currNode, _constraints[0], defaultParseTokens, last_token);
				word++;
				curr_const++;
			}
			else{
				addWordNode(currNode, defaultParseTokens[word]);
				word++;
			}
		}
		else{
			addWordNode(currNode, defaultParseTokens[word]);
			word++;
		}
		//ParseNode* currNode = flat_parse->postmods;
		flat_parse->postmods = new ParseNode();
		currNode = flat_parse->postmods;

		while(word<last_token){
			while((word<last_token) && defaultParseTokens[word]==NULL){
				word++;
			}
			if((curr_const < _numConstraints) && (_constraints[word].left == word)){
				word = addNPPNode(currNode, _constraints[curr_const], defaultParseTokens, last_token);
				word++;
				curr_const++;
			}
			else{
				addWordNode(currNode, defaultParseTokens[word]);
				word++;
			}
			currNode->next = new ParseNode();
			lastNode = currNode;
			currNode= currNode->next;

		}
		lastNode->next = NULL;
		delete currNode;
		highest_scoring_final_theory = 0;
		theory_scores[0] = static_cast<float>(.1);
		return flat_parse;
}





*/



	void BWArabicChartDecoder::addWordNode(ParseNode* currNode, ArabicParseSeeder::SameSpanParseToken* t){
	currNode->label = ArabicSTags::UNKNOWN;
	currNode->headNode = new ParseNode(t->nvString);
	currNode->headNode->chart_start_index = t->start;
	currNode->headNode->chart_end_index = t->end;
//	std::cout<<"Adding word Node: "<<t->nvString.to_debug_string()<<" "<<t->start<<" "<<t->end<<std::endl;
}
/*
int BWArabicChartDecoder::addNPPNode(ParseNode* currNode, Constraint constraint, 
									  ArabicParseSeeder::SameSpanParseToken** sent, int length)
									  throw (UnrecoverableException)
									  
{
	int start_word, end_word =0;
	int i =0;
	while((i < length) &&((sent[i]==NULL) ||(sent[i]->start != constraint.left))) i++;
	if(i == length){
		throw UnrecoverableException("BWArabicChartDecoder::addNPPNode()", 
			"Couldn't find words to match start of name constraint");
	}
	start_word = i;
	while((i < length)&&((sent[i]==NULL) || (sent[i]->start != constraint.right))) i++;
	if(i == length){
		throw UnrecoverableException("BWArabicChartDecoder::addNPPNode()", 
			"Couldn't find words to match end of name constraint");
	}
	end_word = i;
	

	currNode->label = ArabicSTags::NPP;
	currNode->headNode = new ParseNode(LanguageSpecificFunctions::getDefaultNamePOStag());
	ParseNode* p = currNode->headNode;
	p->headNode=new ParseNode(sent[start_word]->nvString);
	p->headNode->chart_start_index = sent[start_word]->start;
	p->headNode->chart_end_index = sent[start_word]->end;
	ArabicParseSeeder::SameSpanParseToken* t =sent[start_word];
	std::cout<<"Adding Name Node: "<<t->nvString.to_debug_string()<<" "<<t->start<<" "<<t->end<<std::endl;


	if(constraint.left != constraint.right){
		currNode->postmods = new ParseNode(LanguageSpecificFunctions::getDefaultNamePOStag());
		p = currNode->postmods;
	
		for(int i =start_word+1; i<end_word; i++){
			p->headNode = new ParseNode(sent[i]->nvString);
			p->headNode->chart_start_index = sent[i]->start;
			p->headNode->chart_end_index = sent[i]->end;
			t= sent[i];
			std::cout<<"\t Adding Modifier Name Node: "<<t->nvString.to_debug_string()
				<<" "<<t->start<<" "<<t->end<<std::endl;


			p->next =  new ParseNode(LanguageSpecificFunctions::getDefaultNamePOStag());
			p = p->next;
		}
		t= sent[end_word];
		std::cout<<"\t Adding Final Modifier Name Node: "<<t->nvString.to_debug_string()
			<<" "<<t->start<<" "<<t->end<<std::endl;

		p->headNode = new ParseNode(sent[end_word]->nvString);
		p->headNode->chart_start_index = sent[end_word]->start;
		p->headNode->chart_end_index = sent[end_word]->end;
	}
	return end_word;
}




*/
void BWArabicChartDecoder::transferTheoriesToChart(int start, int end)
{
	if (numTheories == 0) {
		//std::cout<<"NOTHING FROM "<<start<<" to "<<end<<"\n";
		return;
    }
	else if(DEBUG1){
			_debugStream<<"add theory from "<<start<<" to "<<end; 
	}
	
	int j = 0;
	/* //not sure why this is here.....
	// Skip preterminals already in chart 
	while (chart[start][end - 1][j] != 0) { 
		j++;
	}
	if(j > maxEntriesPerCell){
		std::cout<<"Too many entries in a cell!"<<start<<" "<<end<<"\n";
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
	*/
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


void BWArabicChartDecoder::postProcessChart(){
	if(DEBUG3) std::cout<<"Postprocess chart !"<<std::endl;
	int i;
    for (i = 0; i < MAX_SENTENCE_LENGTH; i++) {
		int k = 0;
		while((k < MAX_TAGS_PER_WORD) && (chart[i][i][k] != 0)){
			if((chart[i][i][k])&& 
				chart[i][i][k]->isPreterminal &&
				(chart[i][i][k]->nameType==ParserTags::nullSymbol)){
					if(DEBUG3) std::cout<<"Post process: "
						<<i<<" "<<i<<" : "
						<<chart[i][i][k]->headWord.to_debug_string()
						<<" -> "<<_cleanWords[i][i].to_debug_string()
						<<std::endl;
					chart[i][i][k]->headWord = _cleanWords[i][i];
				}
			k++;
		}
	}	
	for(i = 0; i< MAX_SENTENCE_LENGTH; i++){
		for (int j = i + 1; j < MAX_SENTENCE_LENGTH; j++) {
			int  k =0;
			while((k < MAX_TAGS_PER_WORD) && (chart[i][j][k] != 0)){
				if((chart[i][j][k])&& 
					chart[i][j][k]->isPreterminal &&
					(chart[i][j][k]->nameType==ParserTags::nullSymbol)){
					if(DEBUG3) std::cout<<"Post process: "
						<<i<<" "<<j<<" : "
						<<chart[i][j][k]->headWord.to_debug_string()
						<<" -> "<<_cleanWords[i][j].to_debug_string()
						<<std::endl;
						chart[i][j][k]->headWord = _cleanWords[i][j];
					}
					k++;
			}
		}
	}
}




void BWArabicChartDecoder::addNameToParse(ParseNode* node, int& currentPosition)
{	if(node->isName){
		addNameToNode(node, currentPosition);
	}
	if (node->headNode == 0) {
		node->label = LanguageSpecificFunctions::getSymbolForParseNodeLeaf(node->label);
		
    }
	else{
		addNameToPremods(node->premods, currentPosition);
		addNameToParse(node->headNode, currentPosition);
		addNameToPostmods(node->postmods, currentPosition);
	}
}
void BWArabicChartDecoder::addNameToPremods(ParseNode* premod, int& currentPosition)
{
    if (premod) {
       addNameToPremods(premod->next, currentPosition);
       addNameToParse(premod, currentPosition);
    }
}

void BWArabicChartDecoder::addNameToPostmods(ParseNode* postmod, int& currentPosition)
{
	 if (postmod) {
		addNameToParse(postmod, currentPosition);
		addNameToPostmods(postmod->next, currentPosition);
    }
}
void BWArabicChartDecoder::postprocessParse(ParseNode* node, std::vector<Constraint> & constraints, bool collapseNPlabels)
{

    if (node->headNode == 0) {
		node->label = LanguageSpecificFunctions::getSymbolForParseNodeLeaf(node->label);
		
    } else {
		LanguageSpecificFunctions::modifyParse(node);
		if (collapseNPlabels &&	LanguageSpecificFunctions::isNPtypeLabel(node->label)) {
			node->label = LanguageSpecificFunctions::getNPlabel();
        }
        postprocessPremods(node->premods, constraints, collapseNPlabels);
        postprocessParse(node->headNode, constraints, collapseNPlabels);
        postprocessPostmods(node->postmods, constraints, collapseNPlabels);
    }
}

void BWArabicChartDecoder::addNameToNode(ParseNode* node, int& currentPosition){
	
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
			if(std_dbg){
				std::cout<<"ADDED NAME Premods- currPos "<<currentPosition
				<<" +pmCounter "<<premod_counter<<" : "
				<<_nameWords[currentPosition+premod_counter].to_debug_string()
				<<" StartIndex: "<<hn->chart_start_index
				<<" EndIndex: "<<hn->chart_end_index<<std::endl;
			}
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
		if(std_dbg){
			std::cout<<"ADDED NAME-Head currPos "<<currentPosition<<" : "
				<<_nameWords[currentPosition].to_debug_string()
				<<" StartIndex: "<<hn->headNode->chart_start_index
				<<" EndIndex: "<<hn->headNode->chart_end_index
				<<std::endl;
		}
		hn->headNode->label = 
			LanguageSpecificFunctions::getSymbolForParseNodeLeaf(_nameWords[currentPosition++]);

	}
	else{
		std::cerr<<"PROBLEM WITH NAME WORD\n";
	}
}





//////Functions that are different for BW

int BWArabicChartDecoder::initChart(ArabicParseSeeder::SameSpanParseToken** init_segments, Symbol* split_sentence,
									int length, std::vector<Constraint> & constraints){
	int name_word[MAX_SENTENCE_LENGTH];
	int constraint_added[MAX_SENTENCE_LENGTH];
	int split_sent_end_index[MAX_SENTENCE_LENGTH];
	ArabicParseSeeder::SameSpanParseToken* defaultParseTokens[MAX_SENTENCE_LENGTH];
	for(int i =0; i< length;  i++){
		split_sentence[i] = Symbol();
		split_sent_end_index[i] = -1;
		defaultParseTokens[i] =NULL;
	}
	_numNameWords = 0;
	int count =0;
	for (int k = 0; k < MAX_SENTENCE_LENGTH; k++) {
		name_word[k] = -1;
		constraint_added[k] = 1;
		possiblePunctuationOrConjunction[k] = false;
	}
	//debuging print constraints
	if(std_dbg) {
		std::cout<<"initChart constraints: "<<constraints.size()<<std::endl;
		BOOST_FOREACH(Constraint constraint, constraints){
			std::cout<<constraint.left<<" "<<constraint.right<<" "
				<<constraint.entityType.getName().to_debug_string()<<std::endl;
		}
	}
	/* this only handles name type constraints! */
	for (size_t j = 0; j < constraints.size(); ++j) {
		int left = constraints[j].left;
		int right = constraints[j].right;
		Symbol type = constraints[j].type;
		EntityType entityType = constraints[j].entityType;
		if (left >= MAX_SENTENCE_LENGTH ||
			  right >= MAX_SENTENCE_LENGTH) 
			  continue;
		if (type.is_null())
			continue;
		constraint_added[j] = -1;
		for (int k = left; k <= right; k++) {
			if(std_dbg){
				std::cout<<"set name_word: "<<k<<" to "<<j<<std::endl;
			}
			name_word[k] = static_cast<int>(j);
		}
	}
	for (int m = 0; m < length; m++) {
		Symbol word = init_segments[m]->nvString;
		int start = init_segments[m]->start;
		int end = init_segments[m]->end;
		if(end > _chartEnd){
			_chartEnd = end;
		}
		if((split_sent_end_index[start] - end ) > 0){
			split_sentence[start] = word;
			split_sent_end_index[start] = end;
			defaultParseTokens[start] = init_segments[m];
		}

		//std::cout<<start<<", "<<end<<" :"<<word.to_debug_string()<<std::endl;
		if (name_word[start]==-1){
		//	std::cout<<"Initialize Word: "<<start<<" "<<end<<" "
		//		<<word.to_debug_string()<<std::endl;
			initChartWord(init_segments[m],start, end, m == 0);
		}
		else{
			int c = name_word[end];
			_nameWords[_numNameWords++] = word;	//important: words that are not in a name occur only once!
			if(constraint_added[c] == 1){
				continue;
			}
			else{
				int left = constraints[c].left;
				int right = constraints[c].right;
				Symbol type = constraints[c].type;
				EntityType entityType = constraints[c].entityType;
				if(right == end){
				//std::cout<<"Initialize Name: "<<m<<" "<<left<<" "<<right<<" "
				//	<<word.to_debug_string()<<std::endl;

					addConstraintEntry(left, right, word, type, entityType);
					constraint_added[c] =1;
				}
				else{
					//std::cout<<"Skip Name Word: word: "<<m<<" left: "<<left<<" right: "<<right<<" "
					//	<<" start "<<start<<" end "<<end
					//"Map Lexical Entry	<<word.to_debug_string()<<std::endl;
				}

			}
		}
	}
	return count;

}
void BWArabicChartDecoder::initChartWord(ArabicParseSeeder::SameSpanParseToken* segment,int startIndex, 
										int endIndex, bool firstWord)
{
	int numTags = 0;    
	const Symbol* parserTags;
	const Symbol* tags;

	Symbol acc_tags[50];
	Symbol dictTags[50];
	int num_dict_tags = 0;
	bool found;
	for(size_t i=0; i <segment->postags.size(); i++){
		found =  false;
		for(int j=0; j<num_dict_tags; j++){
			if(dictTags[j] == segment->postags[i]){
				found = true;
				break;
			}
		}
		if(!found){
			dictTags[num_dict_tags++]= segment->postags[i];
		}
	}

	Symbol word = segment->nvString;
	Symbol originalWord = word;

	if (vocabularyTable->find(word)) {
		parserTags = partOfSpeechTable->lookup(word, numTags);
	} else {
		word = wordFeatures->features(originalWord, firstWord);
		parserTags = partOfSpeechTable->lookup(word, numTags);
		//TODO: prune parts of speech based on lexical info
		if (numTags == 0) {
			word = wordFeatures->reducedFeatures(originalWord, firstWord);
			parserTags = partOfSpeechTable->lookup(word, numTags);
		}
	}
	//add the segment to clean words for postprocessing
	_cleanWords[startIndex][endIndex] = originalWord;

	int num_acc_tags =0;
	if(num_dict_tags == 0){
		tags = parserTags;
	}
	else{
		for (int j = 0; j < numTags; j++) {
			Symbol tag = parserTags[j];
			found = false;
			for(int k=0; k<num_dict_tags; k++){
				if(dictTags[k] == tag){
					found = true;
					break;
				}
			}
			if(found){
				acc_tags[num_acc_tags++] = tag;
			}

		}
		if(num_acc_tags == 0){
			tags = parserTags;
		}
		else{
			numTags = num_acc_tags;
			tags = acc_tags;
		}
	}
	for (int j = 0; j < numTags; j++) {
		Symbol tag = tags[j];

		if (LanguageSpecificFunctions::isBasicPunctuationOrConjunction(tag))
		{
			possiblePunctuationOrConjunction[startIndex] = true;
		}
		
		ChartEntry *entry = _new ChartEntry();
		entry->nameType = ParserTags::nullSymbol;
		fillWordEntry(entry, tag, word, originalWord, startIndex, endIndex);
		//std::cout<<"initChartEntry()- "<<startIndex<<", "<<endIndex<<" "
		//	<<tag.to_debug_string()<<" "<<originalWord.to_debug_string()<<std::endl;
		chart[startIndex][endIndex][j] = entry;
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
			originalWord, startIndex, endIndex, false);
		chart[startIndex][endIndex][0] = entry;	
		numTags = 1;
	}

	chart[startIndex][endIndex][numTags] = 0;
}
void BWArabicChartDecoder::addConstraintEntry(int left, int right,
									  Symbol right_text, Symbol type, EntityType entityType)

{
	Symbol word = right_text;
	const Symbol* tags;
	int numTags;
	bool is_unknown_word = false;
	if (vocabularyTable->find(word)) {
		tags = partOfSpeechTable->lookup(word, numTags);
	} else {
		is_unknown_word = true;
		word = wordFeatures->features(right_text, 0);
		tags = partOfSpeechTable->lookup(word, numTags);
		if (numTags == 0) {
			word = wordFeatures->reducedFeatures(right_text, 0);
			tags = partOfSpeechTable->lookup(word, numTags);
		}
	}
	
	size_t index = 0;
	ChartEntry *entry;
	
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
		Symbol tag = tags[k];
		if (index < static_cast<size_t>(maxEntriesPerCell) && 
			(entityType.isIdfDesc() ||
			 LanguageSpecificFunctions::isPrimaryNamePOStag(tag, entityType) ||
			 (!is_unknown_word &&
			  LanguageSpecificFunctions::isSecondaryNamePOStag(tag, entityType))))
		{
			entry = _new ChartEntry();
			entry->nameType = type;
			fillWordEntry(entry, tag, word, right_text, left, right);
			//std::cout<<"addConstraintEntry()- "<<left<<", "<<right<<" "
			//<<tag.to_debug_string()<<" "<<right_text.to_debug_string()<<std::endl;
			chart[left][right][index++] = entry;
		}
	}

	// if we can't get a tag we want for this word, we are going to set the word to an unknown
	// vector that will. This way the parse won't fragment on account of the name. 
	// Specifically, we set the word to the feature vector of getDefaultNameWord().
	if (index == 0 && !LanguageSpecificFunctions::getDefaultNameWord(DECODER_TYPE,entityType).is_null()) {
		word = wordFeatures->features(LanguageSpecificFunctions::getDefaultNameWord(DECODER_TYPE, entityType), false);
		tags = partOfSpeechTable->lookup(word, numTags);
		for (int k = 0; k < numTags; k++) {
			Symbol tag = tags[k];
			if (index < static_cast<size_t>(maxEntriesPerCell) &&
				(entityType.isIdfDesc() ||
				 LanguageSpecificFunctions::isPrimaryNamePOStag(tag, entityType)))
			{
				entry = _new ChartEntry();
				entry->nameType = type;
				fillWordEntry(entry, tag, word, right_text, left, right);
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
			right_text, left, right);
		chart[left][right][index++] = entry;
	}

	chart[left][right][index] = 0;
}

ParseNode* BWArabicChartDecoder::decode(ArabicParseSeeder::SameSpanParseToken** init_segments, int num_seg, std::vector<Constraint> & constraints,
										bool collapseNPlabels)
{

	//TODO: Add too long flat parse!
	_chartEnd = 0;

	bool lastIsPunct = LanguageSpecificFunctions::isSentenceEndingPunctuation(init_segments[num_seg-1]->nvString);
	Symbol split_sentence[MAX_SENTENCE_LENGTH];
	//std::cout<<"Decode() num_seg: "<<num_seg<<" num constraints: "<<_numConstraints<<std::endl;
	int length = initChart(init_segments, split_sentence, num_seg, constraints);
	if((init_segments[num_seg-1]->end+1) != length){
		length = init_segments[num_seg-1]->end+1;
	}
	
	initPunctuationUpperBound(split_sentence, length);

	for (int span = 2; span <= _chartEnd; span++) {
		for (int start = 0; start <= (_chartEnd - span); start++) {
			int end = start + span;
			numTheories = 0;
			for (int mid = (start + 1); mid < end; mid++) {
				if ((end == _chartEnd) && lastIsPunct &&
					!((mid == (_chartEnd - 1)) && (start == 0))) {
					continue;
				}
				if (punctuationCrossing(start, end, _chartEnd)) {
					continue;
				}
				leftClosable = !crossingConstraintViolation(start, mid, constraints);
				rightClosable = !crossingConstraintViolation(mid, end, constraints);
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
	ParseNode* returnValue = getBestParse(finalScore, 0, _chartEnd+1, false);
	//TODO: add flat parse for null return value!
	
	// diversity parsing iteration
	ParseNode* iterReturnValue = returnValue;
	//print the name words for debugging
	if(std_dbg){
		for(int i =0; i<_numNameWords; i++){
			std::cout<<"NameWord "<<i<<": "<<_nameWords[i].to_debug_string()<<std::endl;
		}
	}
	while (iterReturnValue != 0) {
		int replacementPosition = 0;
		postprocessParse(iterReturnValue, constraints, collapseNPlabels);
		if(std_dbg){
			std::cout<<"********Adding Names*************"<<std::endl;
		}
		addNameToParse(iterReturnValue, replacementPosition);
		iterReturnValue = iterReturnValue->next;
	}
	
	
	cleanupChart(_chartEnd+1);

	return returnValue;
}
void BWArabicChartDecoder::cleanupChart(int length)
{
    for (int i = 0; i < length; i++) {
        for (int j = i; j < length; j++) {
			int k = 0;
            for (ChartEntry** p = chart[i][j]; *p; p++, k++) {
				delete *p;
            }
			chart[i][j][0] = 0;
        }
    }
}
