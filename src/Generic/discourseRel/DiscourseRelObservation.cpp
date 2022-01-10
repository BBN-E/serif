// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // must be first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/discourseRel/DiscourseRelObservation.h"
#include "Generic/theories/NodeInfo.h"

#include "boost/regex.hpp"

const Symbol DiscourseRelObservation::_className(L"discourse-relation");

DTObservation *DiscourseRelObservation::makeCopy() {
	DiscourseRelObservation *copy = _new DiscourseRelObservation();
	copy->populate(this);
	return copy;
}

void DiscourseRelObservation::populate(){
	_sent2IsLedbyConn = false;
	_shareMentions = false;
	_shareNPargMentions = 0;
}

void DiscourseRelObservation::populate(DiscourseRelObservation *other) {
	_word = other->getWord();
	_nextWord = other->getNextWord();
	_secondNextWord = other->getSecondNextWord();
	_prevWord = other->getPrevWord();
	_secondPrevWord = other->getSecondPrevWord();
	_lcWord = other->getLCWord();
	_stemmedWord = other->getStemmedWord();
	_pos = other->getPOS();

	_pPos = other->getParentPOS();
	_leftSiblingHead = other->getLeftSiblingHead();
	_leftSiblingPOS = other->getLeftSiblingPOS();
	_rightSiblingHead = other->getRightSiblingHead();
	_rightSiblingPOS = other->getRightSiblingPOS();
		
	_leftNeighborHead = other->getLeftNeighborHead();
	_leftNeighborPOS = other->getLeftNeighborPOS();
	_rightNeighborHead = other->getRightNeighborHead();
	_rightNeighborPOS = other->getRightNeighborPOS();
	_commonParentPOSforLeftNeighbor = other->getCommonParentPOSforLeftNeighbor();
	_commonParentPOSforRightNeighbor = other->getCommonParentPOSforRightNeighbor();

	_commonParentforLeftNeighbor = other->getCommonParentforLeftNeighbor();
	_commonParentforRightNeighbor = other->getCommonParentforRightNeighbor();
	
	_root = other->getRootOfContextTree ();

	_currentSent = other->getCurrentSent();

	_nextSent = other->getNextSent();

	_bagsOfWordPairs = other->getWordPairs();

	_synsetIdsInCurrentSent = other->getSynsetIdsInCurrentSent();

	_synsetIdsInNextSent = other->getSynsetIdsInNextSent();

	_sent2IsLedbyConn = other->sent2IsLedbyConn();

	_shareMentions = other->shareMentions();
	
	_connLeadingSent2 = other->getConnLeadingSent2();

	_shareNPargMentions = other->getNSharedargMentions();

}


void DiscourseRelObservation::populateWithWords(int sentIndex, TokenSequence** _tokens, 
		const Parse** _parses)
{
	// select context words
	// use stop list in the future
	TokenSequence* tokens1 = _tokens[sentIndex];
	TokenSequence* tokens2 = _tokens[sentIndex+1];
	const Parse* parses1 = _parses[sentIndex];
	const Parse* parses2 = _parses[sentIndex+1];
	_currentSent = tokens1->toString();
	_nextSent = tokens2->toString();
	
	int _num_tokens1= tokens1->getNTokens();
	int _num_tokens2= tokens2->getNTokens();

	map<Symbol, int> *_wordDict1 = _new map<Symbol, int>;
	map<Symbol, int> *_wordDict2 = _new map<Symbol, int>;
	boost::regex expression("^(NN|VB|JJ|RB)");
    boost::match_results<string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default;		
	char pos_str[51];
	map<Symbol, int>::iterator mapIter;

	for (int i=0; i < _num_tokens1; i++){
		Symbol word = tokens1->getToken(i)->getSymbol();
		Symbol lcword = SymbolUtilities::lowercaseSymbol(word);
		const SynNode *node = parses1->getRoot()->getNthTerminal(i)->getParent();
		Symbol pos = node->getTag();
		Symbol stemmedWord = SymbolUtilities::stemWord(lcword, pos);
		//cout << lcword << ", " << pos << endl; 
		StringTransliterator::transliterateToEnglish(pos_str, pos.to_string(), 50);
		string posStr = pos_str;
		string::const_iterator start, end; 
		start = posStr.begin(); 
		end = posStr.end(); 
		if (regex_search(start, end, what, expression, flags)) { 
			// what[0] contains the string pattern matching the whole reg expression
			// what[1] contains the POS	
			if (posStr != "NNP"){
				mapIter = _wordDict1->find(stemmedWord);
				if(mapIter == _wordDict1->end()){
					(*_wordDict1)[stemmedWord]=1;
				}else{
					(*_wordDict1)[stemmedWord]+=1;
				}
				SessionLogger::info("SERIF") << "select " << lcword << ", " << pos << endl; 	
			}	
		}
	}

	for (int i=0; i < _num_tokens2; i++){
		Symbol word = tokens2->getToken(i)->getSymbol();	
		Symbol lcword = SymbolUtilities::lowercaseSymbol(word);
		const SynNode *node = parses2->getRoot()->getNthTerminal(i)->getParent();
		Symbol pos = node->getTag();
		Symbol stemmedWord = SymbolUtilities::stemWord(lcword, pos);
		//cout << lcword << ", " << pos << endl; 
		StringTransliterator::transliterateToEnglish(pos_str, pos.to_string(), 50);
		string posStr = pos_str;
		string::const_iterator start, end; 
		start = posStr.begin(); 
		end = posStr.end(); 
		if (regex_search(start, end, what, expression, flags)) { 
			// what[0] contains the string pattern matching the whole reg expression
			// what[1] contains the POS	
			if (posStr != "NNP"){
				mapIter = _wordDict2->find(stemmedWord);
				if(mapIter == _wordDict2->end()){
					(*_wordDict2)[stemmedWord]=1;
				}else{
					(*_wordDict2)[stemmedWord]+=1;
				}
				SessionLogger::info("SERIF") << "select " << lcword << ", " << pos << endl; 	
			}	
		}
	}

	// generate word pair
	SessionLogger::info("SERIF") << "generate word pairs ..." << endl;	
	map<Symbol, int>::iterator iter1, iter2;
	Symbol word1, word2, wordpair;
	for (iter1=(*_wordDict1).begin(); iter1 != (*_wordDict1).end(); iter1++){
		for (iter2=(*_wordDict2).begin(); iter2 != (*_wordDict2).end(); iter2++){
			word1=(*iter1).first;
			word2=(*iter2).first;
			cout << word1 << "_" << word2 << endl;
			wstring wsword1=word1.to_string();
			wstring wsword2=word2.to_string();
			wstring wspair=wsword1+L"%"+wsword2;
			wordpair=Symbol(wspair.c_str());
			//wordpair=Symbol(*(word1.to_string())+L"_"+*(word2.to_string()));
			SessionLogger::info("SERIF") << wordpair << endl;
			_bagsOfWordPairs.push_back(wordpair);
		}
	}

	delete _wordDict1;
	delete _wordDict2;
	_wordDict1=0;
	_wordDict2=0;
	
}

void DiscourseRelObservation::populateWithNonStopWords(int sentIndex, TokenSequence** _tokens, 
		const Parse** _parses)
{
	// select context words
	// use stop list in the future
	TokenSequence* tokens1 = _tokens[sentIndex];
	TokenSequence* tokens2 = _tokens[sentIndex+1];
	const Parse* parses1 = _parses[sentIndex];
	const Parse* parses2 = _parses[sentIndex+1];
	_currentSent = tokens1->toString();
	_nextSent = tokens2->toString();
	
	int _num_tokens1= tokens1->getNTokens();
	int _num_tokens2= tokens2->getNTokens();

	map<Symbol, int> *_wordDict1 = _new map<Symbol, int>;
	map<Symbol, int> *_wordDict2 = _new map<Symbol, int>;
	boost::regex expression("^(NN|VB|JJ|RB)");
    boost::match_results<string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default;		
	char pos_str[51];
	char word_str[101];
	map<Symbol, int>::iterator mapIter;

	for (int i=0; i < _num_tokens1; i++){
		Symbol word = tokens1->getToken(i)->getSymbol();
		Symbol lcword = SymbolUtilities::lowercaseSymbol(word);
		const SynNode *node = parses1->getRoot()->getNthTerminal(i)->getParent();
		Symbol pos = node->getTag();
		Symbol stemmedWord = SymbolUtilities::stemWord(lcword, pos);
		//cout << lcword << ", " << pos << endl; 

		StringTransliterator::transliterateToEnglish(pos_str, pos.to_string(), 50);
		string posStr = pos_str;
		string::const_iterator start, end; 
		start = posStr.begin(); 
		end = posStr.end(); 
		if (regex_search(start, end, what, expression, flags)) { 
			// what[0] contains the string pattern matching the whole reg expression
			// what[1] contains the POS	
			if (posStr != "NNP"){
				StringTransliterator::transliterateToEnglish(word_str, stemmedWord.to_string(), 100);
				string wordStr = word_str;
				if (StopWordFilter::isInStopWordDict (wordStr)){
					StopWordFilter::recordFilteredWords (wordStr);
					continue;
				}

				mapIter = _wordDict1->find(stemmedWord);
				if(mapIter == _wordDict1->end()){
					(*_wordDict1)[stemmedWord]=1;
				}else{
					(*_wordDict1)[stemmedWord]+=1;
				}
				SessionLogger::info("SERIF") << "select " << lcword << ", " << pos << endl; 	
			}	
		}
	}

	for (int i=0; i < _num_tokens2; i++){
		Symbol word = tokens2->getToken(i)->getSymbol();	
		Symbol lcword = SymbolUtilities::lowercaseSymbol(word);
		const SynNode *node = parses2->getRoot()->getNthTerminal(i)->getParent();
		Symbol pos = node->getTag();
		Symbol stemmedWord = SymbolUtilities::stemWord(lcword, pos);
		//cout << lcword << ", " << pos << endl; 

		StringTransliterator::transliterateToEnglish(pos_str, pos.to_string(), 50);
		string posStr = pos_str;
		string::const_iterator start, end; 
		start = posStr.begin(); 
		end = posStr.end(); 
		if (regex_search(start, end, what, expression, flags)) { 
			// what[0] contains the string pattern matching the whole reg expression
			// what[1] contains the POS	
			if (posStr != "NNP"){
				StringTransliterator::transliterateToEnglish(word_str, stemmedWord.to_string(), 100);
				string wordStr = word_str;
				if (StopWordFilter::isInStopWordDict (wordStr)){
					StopWordFilter::recordFilteredWords (wordStr);
					continue;
				}

				mapIter = _wordDict2->find(stemmedWord);
				if(mapIter == _wordDict2->end()){
					(*_wordDict2)[stemmedWord]=1;
				}else{
					(*_wordDict2)[stemmedWord]+=1;
				}
				SessionLogger::info("SERIF") << "select " << lcword << ", " << pos << endl; 	
			}	
		}
	}

	// generate word pair
	SessionLogger::info("SERIF") << "generate word pairs ..." << endl;	
	map<Symbol, int>::iterator iter1, iter2;
	Symbol word1, word2, wordpair;
	for (iter1=(*_wordDict1).begin(); iter1 != (*_wordDict1).end(); iter1++){
		for (iter2=(*_wordDict2).begin(); iter2 != (*_wordDict2).end(); iter2++){
			word1=(*iter1).first;
			word2=(*iter2).first;
			SessionLogger::info("SERIF") << word1 << "_" << word2 << endl;
			wstring wsword1=word1.to_string();
			wstring wsword2=word2.to_string();
			wstring wspair=wsword1+L"%"+wsword2;
			wordpair=Symbol(wspair.c_str());
			//wordpair=Symbol(*(word1.to_string())+L"_"+*(word2.to_string()));
			SessionLogger::info("SERIF") << wordpair << endl;
			_bagsOfWordPairs.push_back(wordpair);
		}
	}

	delete _wordDict1;
	delete _wordDict2;
	_wordDict1=0;
	_wordDict2=0;
	
}

void DiscourseRelObservation::populateWithWordNet(int sentIndex, TokenSequence** _tokens, 
		const Parse** _parses)
{
	// select context words
	// use stop list in the future
	TokenSequence* tokens1 = _tokens[sentIndex];
	TokenSequence* tokens2 = _tokens[sentIndex+1];
	const Parse* parses1 = _parses[sentIndex];
	const Parse* parses2 = _parses[sentIndex+1];
	_currentSent = tokens1->toString();
	_nextSent = tokens2->toString();
	
	int _num_tokens1= tokens1->getNTokens();
	int _num_tokens2= tokens2->getNTokens();

	map<string, int> *_synsetIdDict1 = _new map<string, int>;
	map<string, int> *_synsetIdDict2 = _new map<string, int>;
	boost::regex expression("^(NN|VB|JJ|RB)");
    boost::match_results<string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default;		
	char pos_str[51];
	map<string, int>::iterator mapIter;

	int _wordnetOffsets[ETO_MAX_WN_OFFSETS];
	int _n_offsets;
	

	for (int i=0; i < _num_tokens1; i++){
		Symbol word = tokens1->getToken(i)->getSymbol();
		Symbol lcword = SymbolUtilities::lowercaseSymbol(word);
		const SynNode *node = parses1->getRoot()->getNthTerminal(i)->getParent();
		Symbol pos = node->getTag();
		Symbol stemmedWord = SymbolUtilities::stemWord(lcword, pos);
		//cout << lcword << ", " << pos << endl; 
		StringTransliterator::transliterateToEnglish(pos_str, pos.to_string(), 50);
		string posStr = pos_str;
		string::const_iterator start, end; 
		start = posStr.begin(); 
		end = posStr.end(); 
		if (regex_search(start, end, what, expression, flags)) { 
			// what[0] contains the string pattern matching the whole reg expression
			// what[1] contains the POS	
			if (posStr != "NNP"){
				SessionLogger::info("SERIF") << "generate WordNet ids for " << lcword << ", " << pos << endl; 
			
				
				_n_offsets = SymbolUtilities::fillWordNetOffsets(stemmedWord, pos, _wordnetOffsets, 
					ETO_MAX_WN_OFFSETS);
				
				for (int i=0; i<=_n_offsets; i++){
					string offset;
					std::stringstream ss;
					ss << _wordnetOffsets[i];
					ss >> offset;
					
					mapIter = _synsetIdDict1->find(offset);
					if(mapIter == _synsetIdDict1->end()){
						(*_synsetIdDict1)[offset]=1;
					}else{
						(*_synsetIdDict1)[offset]+=1;
					}
				}			
			}	
		}
	}

	for (int i=0; i < _num_tokens2; i++){
		Symbol word = tokens2->getToken(i)->getSymbol();	
		Symbol lcword = SymbolUtilities::lowercaseSymbol(word);
		const SynNode *node = parses2->getRoot()->getNthTerminal(i)->getParent();
		Symbol pos = node->getTag();
		Symbol stemmedWord = SymbolUtilities::stemWord(lcword, pos);
		//cout << lcword << ", " << pos << endl; 
		StringTransliterator::transliterateToEnglish(pos_str, pos.to_string(), 50);
		string posStr = pos_str;
		string::const_iterator start, end; 
		start = posStr.begin(); 
		end = posStr.end(); 
		if (regex_search(start, end, what, expression, flags)) { 
			// what[0] contains the string pattern matching the whole reg expression
			// what[1] contains the POS	
			if (posStr != "NNP"){
				SessionLogger::info("SERIF") << "generate WordNet ids for " << lcword << ", " << pos << endl; 
				
				_n_offsets = SymbolUtilities::fillWordNetOffsets(stemmedWord, pos,
					_wordnetOffsets, ETO_MAX_WN_OFFSETS);
				
				for (int i=0; i<_n_offsets; i++){
					string offset;
					std::stringstream ss;
					ss << _wordnetOffsets[i];
					ss >> offset;
					mapIter = _synsetIdDict2->find(offset);
					if(mapIter == _synsetIdDict2->end()){
						(*_synsetIdDict2)[offset]=1;
					}else{
						(*_synsetIdDict2)[offset]+=1;
					}
				}
			}	
		}
	}


	// generate wordNet Id vectors
	SessionLogger::info("SERIF") << "generate WordNet Id vectors ..." << endl;	
	map<string, int>::iterator iter1, iter2;
	string synsetId1, synsetId2;
	Symbol synsetIdpair;
	for (iter1=(*_synsetIdDict1).begin(); iter1 != (*_synsetIdDict1).end(); iter1++){
		synsetId1=(*iter1).first;
		_synsetIdsInCurrentSent.push_back(atoi(synsetId1.c_str()));
	}
	for (iter2=(*_synsetIdDict2).begin(); iter2 != (*_synsetIdDict2).end(); iter2++){
		synsetId2=(*iter2).first;
		_synsetIdsInNextSent.push_back(atoi(synsetId2.c_str()));	
	}

	delete _synsetIdDict1;
	delete _synsetIdDict2;
	_synsetIdDict1=0;
	_synsetIdDict2=0;
	
}



void DiscourseRelObservation::populateWithWordNetUsingStopWordLs(int sentIndex, TokenSequence** _tokens, 
		const Parse** _parses)
{
	// select context words
	// use stop list in the future
	TokenSequence* tokens1 = _tokens[sentIndex];
	TokenSequence* tokens2 = _tokens[sentIndex+1];
	const Parse* parses1 = _parses[sentIndex];
	const Parse* parses2 = _parses[sentIndex+1];
	_currentSent = tokens1->toString();
	_nextSent = tokens2->toString();
	
	int _num_tokens1= tokens1->getNTokens();
	int _num_tokens2= tokens2->getNTokens();

	map<string, int> *_synsetIdDict1 = _new map<string, int>;
	map<string, int> *_synsetIdDict2 = _new map<string, int>;
	boost::regex expression("^(NN|VB|JJ|RB)");
    boost::match_results<string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default;		
	char pos_str[51];
	char word_str[101];
	map<string, int>::iterator mapIter;

	int _wordnetOffsets[ETO_MAX_WN_OFFSETS];
	int _n_offsets;
	

	for (int i=0; i < _num_tokens1; i++){
		Symbol word = tokens1->getToken(i)->getSymbol();
		Symbol lcword = SymbolUtilities::lowercaseSymbol(word);
		const SynNode *node = parses1->getRoot()->getNthTerminal(i)->getParent();
		Symbol pos = node->getTag();
		Symbol stemmedWord = SymbolUtilities::stemWord(lcword, pos);
		//cout << lcword << ", " << pos << endl;

		StringTransliterator::transliterateToEnglish(pos_str, pos.to_string(), 50);
		string posStr = pos_str;
		string::const_iterator start, end; 
		start = posStr.begin(); 
		end = posStr.end(); 
		if (regex_search(start, end, what, expression, flags)) { 
			// what[0] contains the string pattern matching the whole reg expression
			// what[1] contains the POS	
			if (posStr != "NNP"){
				SessionLogger::info("SERIF") << "generate WordNet ids for " << lcword << ", " << pos << endl; 
						
				StringTransliterator::transliterateToEnglish(word_str, stemmedWord.to_string(), 100);
				string wordStr = word_str;
				if (StopWordFilter::isInStopWordDict (wordStr)){
					StopWordFilter::recordFilteredWords (wordStr);
					continue;
				}

				_n_offsets = SymbolUtilities::fillWordNetOffsets(stemmedWord, pos,
					_wordnetOffsets, ETO_MAX_WN_OFFSETS);
	
				for (int i=0; i<=_n_offsets; i++){
					string offset;
					std::stringstream ss;
					ss << _wordnetOffsets[i];
					ss >> offset;
					
					mapIter = _synsetIdDict1->find(offset);
					if(mapIter == _synsetIdDict1->end()){
						(*_synsetIdDict1)[offset]=1;
					}else{
						(*_synsetIdDict1)[offset]+=1;
					}
				}			
			}	
		}
	}

	for (int i=0; i < _num_tokens2; i++){
		Symbol word = tokens2->getToken(i)->getSymbol();	
		Symbol lcword = SymbolUtilities::lowercaseSymbol(word);
		const SynNode *node = parses2->getRoot()->getNthTerminal(i)->getParent();
		Symbol pos = node->getTag();
		Symbol stemmedWord = SymbolUtilities::stemWord(lcword, pos);
		//cout << lcword << ", " << pos << endl; 
		
		StringTransliterator::transliterateToEnglish(pos_str, pos.to_string(), 50);
		string posStr = pos_str;
		string::const_iterator start, end; 
		start = posStr.begin(); 
		end = posStr.end(); 
		if (regex_search(start, end, what, expression, flags)) { 
			// what[0] contains the string pattern matching the whole reg expression
			// what[1] contains the POS	
			if (posStr != "NNP"){
				SessionLogger::info("SERIF") << "generate WordNet ids for " << lcword << ", " << pos << endl; 
				
				StringTransliterator::transliterateToEnglish(word_str, stemmedWord.to_string(), 100);
				string wordStr = word_str;
				if (StopWordFilter::isInStopWordDict (wordStr)){
					StopWordFilter::recordFilteredWords (wordStr);
					continue;
				}

				_n_offsets = SymbolUtilities::fillWordNetOffsets(stemmedWord, pos,
					_wordnetOffsets, ETO_MAX_WN_OFFSETS);
				
				for (int i=0; i<_n_offsets; i++){
					string offset;
					std::stringstream ss;
					ss << _wordnetOffsets[i];
					ss >> offset;
					mapIter = _synsetIdDict2->find(offset);
					if(mapIter == _synsetIdDict2->end()){
						(*_synsetIdDict2)[offset]=1;
					}else{
						(*_synsetIdDict2)[offset]+=1;
					}
				}
			}	
		}
	}


	// generate wordNet Id vectors
	SessionLogger::info("SERIF") << "generate WordNet Id vectors ..." << endl;	
	map<string, int>::iterator iter1, iter2;
	string synsetId1, synsetId2;
	Symbol synsetIdpair;
	for (iter1=(*_synsetIdDict1).begin(); iter1 != (*_synsetIdDict1).end(); iter1++){
		synsetId1=(*iter1).first;
		_synsetIdsInCurrentSent.push_back(atoi(synsetId1.c_str()));
	}
	for (iter2=(*_synsetIdDict2).begin(); iter2 != (*_synsetIdDict2).end(); iter2++){
		synsetId2=(*iter2).first;
		_synsetIdsInNextSent.push_back(atoi(synsetId2.c_str()));	
	}

	delete _synsetIdDict1;
	delete _synsetIdDict2;
	_synsetIdDict1=0;
	_synsetIdDict2=0;
	
}


void DiscourseRelObservation::populateWithRichFeatures(int sentIndex, TokenSequence** _tokens, EntitySet** _entitySets, MentionSet** _mentionSets, const Parse** _parses, const PropositionSet** _propSets, UTF8OutputStream& out)
{
	
	const Parse* parse1 = _parses[sentIndex];
	const Parse* parse2 = _parses[sentIndex+1];
	const SynNode* _root2 = parse2->getRoot();
	
	// for debug purpose
	TokenSequence* tokens1 = _tokens[sentIndex];
	TokenSequence* tokens2 = _tokens[sentIndex+1];
	_currentSent = tokens1->toString();
	_nextSent = tokens2->toString();
	Symbol sentOut = Symbol(_currentSent.c_str());
	out << sentIndex << ": " << sentOut << "\n";
	sentOut = Symbol(_nextSent.c_str());
	out << (sentIndex+1) << ": " << sentOut << "\n";

	// connective feature: whether the second sentence is led by a connective; if so, what is the connective
	const SynNode *firstNode = _root2->getNthTerminal(0)->getParent();
	Symbol connTag=Symbol(L"CC");
	if (firstNode->getTag() == connTag){
		_sent2IsLedbyConn=true;
		_connLeadingSent2=firstNode->getHeadWord();
	}else{
		_sent2IsLedbyConn=false;
	}

	// pronoun features : deictic, third person, "these" and "those"
	


	// entity features :  whether the two sentences shared mentions
	//EntitySet* entityset1 = _entitySets[sentIndex];
	EntitySet* entityset2 = _entitySets[sentIndex+1];
	// Note: the Entity set corresponding to one sentence includes mentions in 
	// this sentence and/or its previous sentence

	_shareMentions = false;
	//out << "entity checking ..." << "\n";
	for (int i = 0; i < entityset2->getNEntities(); i++){
		Entity *entity = entityset2 ->getEntity(i);	
		vector<const SynNode *> mentionInSent1, mentionInSent2;	
		bool mentInSent1 = false;
		bool mentInSent2 = false;
	
		for(int j = 0; j < entity->getNMentions(); j++){
			const Mention* ment = entityset2->getMention(entity->getMention(j));
			
			// for debug information
			/*SessionLogger::info("SERIF") << ment->getUID() << "\n";
			if (ment->getEntityType().isRecognized()){	
				SessionLogger::info("SERIF") << ment->getEntityType().getName() << "\n";
			}*/

			const SynNode* mentionNode = ment->getNode();
			//out << mentionNode->toPrettyParse(3) << "\n";
			int sentenceNum = Mention::getSentenceNumberFromUID(entity->getMention(j));
			if (sentenceNum == sentIndex){
				mentInSent1 = true;
				mentionInSent1.push_back(mentionNode);
				//out << "sent1: " << mentionNode->toPrettyParse(3) << "\n";
				//mentionNode->dump(cout, 3);
				//cout << endl;
			}else if (sentenceNum == sentIndex + 1){
				mentInSent2 = true;
				mentionInSent2.push_back(mentionNode);
				//out << "sent2: " << mentionNode->toPrettyParse(3) << "\n";
				//mentionNode->dump(cout, 3);
				//cout << endl;
			}
		}

		if (mentInSent1 && mentInSent2){
			_shareMentions = true;
			const SynNode* mentionNode;
			//Symbol sentOut = Symbol(_currentSent.c_str());
			//out << sentOut << "\n";
			//sentOut = Symbol(_nextSent.c_str());
			//out << sentOut << "\n";
			out << "\nentity: " << entity->getType().getName() << "\n";
			for (int i=0; i< (int)mentionInSent1.size(); i++){
				mentionNode = mentionInSent1[i];
				out << "sent1: " << mentionNode->toPrettyParse(3) << "\n";
			}

			for (int i=0; i< (int)mentionInSent2.size(); i++){
				mentionNode = mentionInSent2[i];
				out << "sent2: " << mentionNode->toPrettyParse(3) << "\n";
			}
			out << "\n\n";
			
		}

	}

	// proposition features + entity based features(
	// whether the subject/object in sentence 2 occur in sentence 1)
	vector<Proposition *> verbpredList;
	const PropositionSet* _propSet=_propSets[sentIndex+1];
	const char * posOfPredHead;
	if (_propSet != 0) {
		for (int i = 0; i < _propSet->getNPropositions(); i++) {
			Proposition * prop = _propSet->getProposition(i);
			if (prop->getPredHead() != 0 ){
				posOfPredHead = prop->getPredHead()->getHeadPreterm()->getTag().to_debug_string();
				if (posOfPredHead[0] == 'V' && posOfPredHead[1]=='B' ){
					//out << posOfPredHead << "\n";	
					verbpredList.push_back(prop);	
				}
			}
		}	
	}

	const Entity * argEntityInSent2[MAXENTITYINSENT2];
	int argEntityIndex=0;


	Symbol _objectOfTrigger = Symbol();
	Symbol _indirectObjectOfTrigger = Symbol();
	Symbol _subjectOfTrigger = Symbol();
	MentionSet* mentionSet=_mentionSets[sentIndex+1];
	for (int i=0; i< (int)verbpredList.size(); i++){
		Proposition * prop = verbpredList[i];
		out << prop->getPredHead()->getHeadPreterm()->toPrettyParse(3) << "\n";	
		for (int argnum = 0; argnum < prop->getNArgs(); argnum++) {
			Argument *arg = prop->getArg(argnum) ;
			if (arg->getType() != Argument::MENTION_ARG) 
				continue;
			if (arg->getRoleSym() == Argument::REF_ROLE ||
				arg->getRoleSym() == Argument::TEMP_ROLE ||
				arg->getRoleSym() == Argument::LOC_ROLE ||
				arg->getRoleSym() == Argument::MEMBER_ROLE ||
				arg->getRoleSym() == Argument::UNKNOWN_ROLE)
			{
				continue;
			}
	
			const Mention *ment = arg->getMention(mentionSet);
			Symbol mentionHead;
			Symbol mentionType;
			
			// the NP argument is an Entity, so there is coreference information about it in Brandy
			// NP arguments corresponding to the same entity are identified and used as a single entity later
			mentionHead = ment->getNode()->getHeadWord(); 
			if (ment->getEntityType().isRecognized()){
				mentionType = ment->getEntityType().getName();
				out << "NP argument is recognized entity: " << mentionType << "\n";
				const Entity* entity = entityset2->getEntityByMention(ment->getUID(),
ment->getEntityType());
				// it is a little strange that some mentions were not grouped into entities
				bool isNewEntity = true;
				if (entity !=  NULL){
					for (int i=0; i<argEntityIndex ; i++){
						if (argEntityInSent2[i] == entity){
							isNewEntity = false;
							break;
						}
					}
					if (isNewEntity){
						argEntityInSent2[argEntityIndex]=entity;
						argEntityIndex++;
					}

					if (argEntityIndex == MAXENTITYINSENT2){
						break;
					}
				
					/*vector<const SynNode *> mentionInSent1, mentionInSent2;	
					int mentsInSent1 = 0;
					int mentsInSent2 = 0;
	
					for(int j = 0; j < entity->getNMentions(); j++){
						const Mention* ment2 = entityset2->getMention(entity->getMention(j));
						const SynNode* mentionNode = ment2->getNode();
						//out << mentionNode->toPrettyParse(3) << "\n";
						int sentenceNum = Mention::getSentenceNumberFromUID(entity->getMention(j));
						if (sentenceNum == sentIndex){
							mentsInSent1 ++;
							mentionInSent1.push_back(mentionNode);
							//out << "sent1: " << mentionNode->toPrettyParse(3) << "\n";
							//mentionNode->dump(cout, 3);
							//cout << endl;
						}else if (sentenceNum == sentIndex + 1){
							mentsInSent2 ++;
							mentionInSent2.push_back(mentionNode);
							//out << "sent2: " << mentionNode->toPrettyParse(3) << "\n";
							//mentionNode->dump(cout, 3);
							//cout << endl;
						}
					}
		
					if (mentsInSent1 > 0  && mentsInSent2 > 0 ){
						_shareNPargMentions ++;
						const SynNode* mentionNode;
						//Symbol sentOut = Symbol(_currentSent.c_str());
						//out << sentOut << "\n";
						//sentOut = Symbol(_nextSent.c_str());
						//out << sentOut << "\n";
						
						out << " .. this NP arg has mentions in Sent1 \n";
						for (int i=0; i< (int)mentionInSent1.size(); i++){
							mentionNode = mentionInSent1[i];
							out << " ...... sent1: " << mentionNode->toPrettyParse(3) << "\n";
						}

						for (int i=0; i< (int)mentionInSent2.size(); i++){
							mentionNode = mentionInSent2[i];
							out << " ...... sent2: " << mentionNode->toPrettyParse(3) << "\n";
						}
						out << "\n\n";
					}*/
				}
			}

			if (arg->getRoleSym() == Argument::SUB_ROLE){
				_subjectOfTrigger = mentionHead;
				out << "subject: " << mentionHead << "\n";	
			}else if (arg->getRoleSym() == Argument::OBJ_ROLE){
				_objectOfTrigger = mentionHead;
				out << "object: " << mentionHead << "\n";	
			}else if (arg->getRoleSym() == Argument::IOBJ_ROLE){
				_indirectObjectOfTrigger = mentionHead;
				out << "indirect object: " << mentionHead << "\n";	
			}
			
		}
		out << "\n";
	}
	
	for (int i=0; i<argEntityIndex ; i++){
		const Entity * entity = argEntityInSent2[i];
		vector<const SynNode *> mentionInSent1, mentionInSent2;	
		int mentsInSent1 = 0;
		int mentsInSent2 = 0;
	
		for(int j = 0; j < entity->getNMentions(); j++){
			const Mention* ment2 = entityset2->getMention(entity->getMention(j));
			const SynNode* mentionNode = ment2->getNode();
			//out << mentionNode->toPrettyParse(3) << "\n";
			int sentenceNum = Mention::getSentenceNumberFromUID(entity->getMention(j));
			if (sentenceNum == sentIndex){
				mentsInSent1 ++;
				mentionInSent1.push_back(mentionNode);
				//out << "sent1: " << mentionNode->toPrettyParse(3) << "\n";
				//mentionNode->dump(cout, 3);
				//cout << endl;
			}else if (sentenceNum == sentIndex + 1){
				mentsInSent2 ++;
				mentionInSent2.push_back(mentionNode);
				//out << "sent2: " << mentionNode->toPrettyParse(3) << "\n";
				//mentionNode->dump(cout, 3);
				//cout << endl;
			}
		}
		
		if (mentsInSent1 > 0  && mentsInSent2 > 0 ){
			_shareNPargMentions ++;
			const SynNode* mentionNode;
			//Symbol sentOut = Symbol(_currentSent.c_str());
			//out << sentOut << "\n";
			//sentOut = Symbol(_nextSent.c_str());
			//out << sentOut << "\n";
					
			out << " .. this NP arg has mentions in Sent1 \n";
			for (int i=0; i< (int)mentionInSent1.size(); i++){
				mentionNode = mentionInSent1[i];
				out << " ...... sent1: " << mentionNode->toPrettyParse(3) << "\n";
			}

			for (int i=0; i< (int)mentionInSent2.size(); i++){
				mentionNode = mentionInSent2[i];
				out << " ...... sent2: " << mentionNode->toPrettyParse(3) << "\n";
			}
			out << "\n\n";
		}

	}
	
}

/*void populateWithProposition(int sentIndex, TokenSequence** _tokens, const Parse** _parses, 
		MentionSet** _mentions, const PropositionSet** _propSet, bool use_wordnet)
{ 
	if (use_wordnet)
		_n_offsets = SymbolUtilities::fillWordNetOffsets(_stemmedWord, _pos,
							_wordnetOffsets, ETO_MAX_WN_OFFSETS);
	else _n_offsets = 0;
	_wordCluster = WordClusterClass(_lcWord, true);
	_documentTopic = Symbol();
	_is_nominal_premod = NodeInfo::isNominalPremod(parse->getRoot()->getNthTerminal(token_index)->getParent());
	_is_copula = false;	

	Proposition *triggerProp = 0;
	const SynNode *triggerNode = parse->getRoot()->getNthTerminal(token_index);
	triggerNode = triggerNode->getParent();
	if (propSet != 0) {
		for (int i = 0; i < propSet->getNPropositions(); i++) {
			if (propSet->getProposition(i)->getPredHead() != 0 &&
				propSet->getProposition(i)->getPredHead()->getHeadPreterm() == triggerNode) 
			{
				triggerProp = propSet->getProposition(i);
				break;
			}
		}	
	}
	_objectOfTrigger = Symbol();
	_indirectObjectOfTrigger = Symbol();
	_subjectOfTrigger = Symbol();
	for (int i = 0; i < _MAX_OTHER_ARGS; i++) {
		_otherArgsToTrigger[i] = Symbol();
	}
	int n_args = 0;
	if (triggerProp != 0) {
		if (triggerProp->getPredType() == Proposition::COPULA_PRED)
			_is_copula = true;
		for (int argnum = 0; argnum < triggerProp->getNArgs(); argnum++) {
			Argument *arg = triggerProp->getArg(argnum);
			if (arg->getType() != Argument::MENTION_ARG) 
				continue;
			if (arg->getRoleSym() == Argument::REF_ROLE ||
				arg->getRoleSym() == Argument::TEMP_ROLE ||
				arg->getRoleSym() == Argument::LOC_ROLE ||
				arg->getRoleSym() == Argument::MEMBER_ROLE ||
				arg->getRoleSym() == Argument::UNKNOWN_ROLE)
			{
				continue;
			}

			const Mention *ment = arg->getMention(mentionSet);
			Symbol sym;
			if (ment->getEntityType().isRecognized())
				sym = ment->getEntityType().getName();
			else sym = ment->getNode()->getHeadWord();
			
			if (arg->getRoleSym() == Argument::SUB_ROLE)
				_subjectOfTrigger = sym;
			else if (arg->getRoleSym() == Argument::OBJ_ROLE)
				_objectOfTrigger = sym;
			else if (arg->getRoleSym() == Argument::IOBJ_ROLE)
				_indirectObjectOfTrigger = sym;
			else if (arg->getRoleSym() == Argument::POSS_ROLE && n_args < _MAX_OTHER_ARGS) {
				std::wstring str = L"poss:";
				str += sym.to_string();
				_otherArgsToTrigger[n_args++] = Symbol(str.c_str());
			} else if (n_args < _MAX_OTHER_ARGS) {
				std::wstring str = arg->getRoleSym().to_string();
				str += L":";
				str += sym.to_string();
				_otherArgsToTrigger[n_args++] = Symbol(str.c_str());
			} 
		}
	}
}
*/

void DiscourseRelObservation::populate(int token_index, 
									   const TokenSequence *tokens, 
									   const Parse *parse, 
									   bool use_wordnet=false) 
{ 
	_root = parse->getRoot();
	
	const SynNode *targetNode = parse->getRoot()->getNthTerminal(token_index)->getParent();
	
	_word = tokens->getToken(token_index)->getSymbol();
	SessionLogger::info("SERIF") << _word << "\n";
	_lcWord = SymbolUtilities::lowercaseSymbol(_word);
	_pos = targetNode->getTag();
	_stemmedWord = SymbolUtilities::stemWord(_lcWord, _pos);
	if (tokens->getNTokens() - 1 == token_index)
		_nextWord = Symbol(L"END");
	else _nextWord = tokens->getToken(token_index+1)->getSymbol();
	if (tokens->getNTokens() - 2 <= token_index)
		_secondNextWord = Symbol(L"END");
	else _secondNextWord = tokens->getToken(token_index+2)->getSymbol();
	if (token_index == 0)
		_prevWord = Symbol(L"START");
	else _prevWord = tokens->getToken(token_index-1)->getSymbol();
	if (token_index <= 1)
		_secondPrevWord = Symbol(L"START");
	else _secondPrevWord = tokens->getToken(token_index-2)->getSymbol();
		
	
	const SynNode *parentNode = targetNode->getParent();
	_pPos = parentNode->getTag();
	const SynNode *leftSibling = NULL;
	const SynNode *rightSibling = NULL;
	int numOfChildren = parentNode->getNChildren();
	
	for (int i=0; i < numOfChildren; i++){
		const SynNode *childNode = parentNode->getChild(i);
		if (childNode == targetNode){
			if (i > 0){
				leftSibling=parentNode->getChild(i-1);
				_leftSiblingHead=SymbolUtilities::lowercaseSymbol(leftSibling->getHeadWord());
				_leftSiblingPOS=leftSibling->getTag();
			}else{
				_leftSiblingHead=Symbol(L"-EMPTY-");
				_leftSiblingPOS=Symbol(L"-EMPTY-");
			}
			if (i < numOfChildren-1){
				rightSibling=parentNode->getChild(i+1);
				_rightSiblingHead=SymbolUtilities::lowercaseSymbol(rightSibling->getHeadWord());
				_rightSiblingPOS=rightSibling->getTag();
			}else{
				_rightSiblingHead=Symbol(L"-EMPTY-");
				_rightSiblingPOS=Symbol(L"-EMPTY-");
			}
			break;
		}
	}

	
	const SynNode *LeftNeighbor;
	const SynNode *RightNeighbor;
	LeftNeighbor = targetNode->getPrevTerminal();
	RightNeighbor = targetNode->getNextTerminal();

	if (LeftNeighbor != 0){
		LeftNeighbor=LeftNeighbor->getParent();
		_leftNeighborPOS = LeftNeighbor->getTag();
		_leftNeighborHead = SymbolUtilities::lowercaseSymbol(LeftNeighbor->getHeadWord()); 
	}else{
		_leftNeighborPOS = Symbol(L"-EMPTY-");
		_leftNeighborHead = Symbol(L"-EMPTY-");
	}
	
	if (RightNeighbor != 0){
		RightNeighbor = RightNeighbor->getParent();
		_rightNeighborPOS = RightNeighbor->getTag();
		_rightNeighborHead = SymbolUtilities::lowercaseSymbol(RightNeighbor->getHeadWord()); 
	}else{
		_rightNeighborPOS = Symbol(L"-EMPTY-");
		_rightNeighborHead = Symbol(L"-EMPTY-");
	}
		
	_commonParentforLeftNeighbor = findCommonParent(LeftNeighbor, targetNode);
	if (_commonParentforLeftNeighbor != 0){
		_commonParentPOSforLeftNeighbor = _commonParentforLeftNeighbor->getTag();
	}else{
		_commonParentPOSforLeftNeighbor = Symbol(L"-EMPTY-");
	}

	_commonParentforRightNeighbor = findCommonParent(RightNeighbor, targetNode);
	if (_commonParentforRightNeighbor != 0){
		_commonParentPOSforRightNeighbor = _commonParentforRightNeighbor->getTag();
	}else{
		_commonParentPOSforRightNeighbor = Symbol(L"-EMPTY-");
	}

}

const SynNode *DiscourseRelObservation::findCommonParent(const SynNode *node1, const SynNode *node2){
	map<const SynNode *, int> *parent1Sets;
	
	if (node1 == 0 || node2 == 0){
		return 0;
	}
	parent1Sets = _new map<const SynNode *, int>;
	const SynNode *parent1 = node1->getParent();
	while (parent1 != 0){
		(*parent1Sets)[parent1]=1;
		parent1=parent1->getParent();
	}

	const SynNode *parent2= node2->getParent();
	while (parent2 != 0){
		map<const SynNode *, int>::iterator iter1 = parent1Sets->find(parent2);
		if (iter1 != parent1Sets->end()){
			delete parent1Sets;
			parent1Sets=0;

			return (parent2);
		}
		parent2=parent2->getParent();
	}

	delete parent1Sets;
	parent1Sets=0;
	SessionLogger::warn("SERIF") << "error in finding common parent \n "; 
	return 0;
}

bool DiscourseRelObservation::isLastWord() { 	
	return (_nextWord == Symbol(L"END"));
}

bool DiscourseRelObservation::isFirstWord() { 
	return (_prevWord == Symbol(L"START"));
}	

Symbol DiscourseRelObservation::getWord() { 
	return _word;
}

Symbol DiscourseRelObservation::getPOS() { 
	return _pos;
}

Symbol DiscourseRelObservation::getStemmedWord() { 
	return _stemmedWord;
}

Symbol DiscourseRelObservation::getLCWord() { 
	return _lcWord;
}

Symbol DiscourseRelObservation::getNextWord() { 
	return _nextWord;
}

Symbol DiscourseRelObservation::getSecondNextWord() { 
	return _secondNextWord;
}

Symbol DiscourseRelObservation::getPrevWord() { 
	return _prevWord;
}

Symbol DiscourseRelObservation::getSecondPrevWord() { 
	return _secondPrevWord;
}

Symbol DiscourseRelObservation::getParentPOS(){
	return _pPos;
}

Symbol DiscourseRelObservation::getLeftSiblingHead(){
	return _leftSiblingHead;
}
	
Symbol DiscourseRelObservation::getLeftSiblingPOS(){
	return _leftSiblingPOS;
}

Symbol DiscourseRelObservation::getRightSiblingHead(){
	return _rightSiblingHead;
}

Symbol DiscourseRelObservation::getRightSiblingPOS(){
	return _rightSiblingPOS;
}

Symbol DiscourseRelObservation::getLeftNeighborHead(){
	return _leftNeighborHead;
}

Symbol DiscourseRelObservation::getLeftNeighborPOS(){
	return _leftNeighborPOS;
}

Symbol DiscourseRelObservation::getRightNeighborHead(){
	return _rightNeighborHead;
}

Symbol DiscourseRelObservation::getRightNeighborPOS(){
	return _rightNeighborPOS;
}

Symbol DiscourseRelObservation::getCommonParentPOSforLeftNeighbor(){
	return _commonParentPOSforLeftNeighbor;
}

Symbol DiscourseRelObservation::getCommonParentPOSforRightNeighbor(){
	return _commonParentPOSforRightNeighbor;
}

const SynNode *DiscourseRelObservation::getCommonParentforLeftNeighbor(){
	return _commonParentforLeftNeighbor;
}
const SynNode *DiscourseRelObservation::getCommonParentforRightNeighbor(){
	return _commonParentforRightNeighbor;
}

wstring DiscourseRelObservation::getSentPair(){
	return L"Sentence Pair: \n" + _currentSent + L"\n"+ _nextSent;
}


wstring DiscourseRelObservation::getCurrentSent(){
	return _currentSent;
}


wstring DiscourseRelObservation::getNextSent(){
	return _nextSent;
}


vector<Symbol> DiscourseRelObservation::getWordPairs(){
	return _bagsOfWordPairs;
}


vector<int> DiscourseRelObservation::getSynsetIdsInCurrentSent(){
	return _synsetIdsInCurrentSent;
}

vector<int> DiscourseRelObservation::getSynsetIdsInNextSent(){
	return _synsetIdsInNextSent;
}

bool DiscourseRelObservation::sent2IsLedbyConn(){
	return _sent2IsLedbyConn;
}

bool DiscourseRelObservation::shareMentions(){
	return _shareMentions;
}
	
Symbol DiscourseRelObservation::getConnLeadingSent2(){
	return _connLeadingSent2;
}

int DiscourseRelObservation::getNSharedargMentions(){
	return _shareNPargMentions;
}
bool DiscourseRelObservation::shareNPargMentions(){
	return (_shareNPargMentions > 0) ;
}

bool DiscourseRelObservation::share2NPargMentions(){
	return (_shareNPargMentions > 1) ;
}

/*

Symbol DiscourseRelObservation::getDocumentTopic() { 
	return _documentTopic;
}

void DiscourseRelObservation::setDocumentTopic(Symbol topic) { 
	_documentTopic = topic;
}

int DiscourseRelObservation::getReversedNthOffset(int n) {
	if (n < _n_offsets) {
		return _wordnetOffsets[_n_offsets - n - 1];
	} else return -1;	
}

int DiscourseRelObservation::getNthOffset(int n) {
	if (n < _n_offsets) {
		return _wordnetOffsets[n];
	} else return -1;	
}
int DiscourseRelObservation::getNOffsets() {
	return _n_offsets;
}

WordClusterClass DiscourseRelObservation::getWordCluster() {
	return _wordCluster;
}

Symbol DiscourseRelObservation::getObjectOfTrigger() {
	return _objectOfTrigger;
}
Symbol DiscourseRelObservation::getIndirectObjectOfTrigger() {
	return _indirectObjectOfTrigger;
}
Symbol DiscourseRelObservation::getSubjectOfTrigger() {
	return _subjectOfTrigger;
}
Symbol DiscourseRelObservation::getOtherArgToTrigger(int n) {
	if (n < 0 || n >= _MAX_OTHER_ARGS)
		return Symbol();
	return _otherArgsToTrigger[n];
}

bool DiscourseRelObservation::isNominalPremod() {
	return _is_nominal_premod;
}

bool DiscourseRelObservation::isCopula() {
	return _is_copula;
}

*/
