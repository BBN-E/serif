// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/edt/ar_PronounLinker.h"

//#include "Arabic/edt/ar_PronounLinker.h"
#include "Arabic/parse/ar_STags.h"
#include "Generic/edt/HobbsDistance.h"
#include "Arabic/common/ar_WordConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
//#include "Generic/theories/EntityType.h"
#include "Generic/edt/discmodel/DTPronounLinker.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

#ifdef _WIN32
#define snprintf _snprintf
#endif


//#include "Arabic/edt/ar_PronounLinker.h"
//DebugStream ArabicPronounLinker::_debugOut;

ArabicPronounLinker::ArabicPronounLinker() :_mascSing(0), _femSing(0), _discard_pron(false)
, _dtPronounLinker(0), MODEL_TYPE(0)
{
	HobbsDistance::initialize();
	_doLink = ParamReader::isParamTrue("do_pron_link");

//this should be implemented for better name pronoun linking, but we need mentions
	_femSing = _new SymbolHash(1000);
	_mascSing = _new SymbolHash(1000);
	std::string fn_buffer = ParamReader::getParam("name_gender");
	if (fn_buffer != "") {
		std::string masc_sing_file = fn_buffer + "/mascsing.txt";
		loadSymbolHash(_mascSing, masc_sing_file.c_str());
		std::string fem_sing_file = fn_buffer + "/femsing.txt";
		loadSymbolHash(_femSing, fem_sing_file.c_str());
	}

	_discard_pron = ParamReader::getRequiredTrueFalseParam("discard_pronouns");

	std::string link_mode = ParamReader::getParam("pronoun_link_mode");
	MODEL_TYPE = PM;
	if (link_mode == "PM" || link_mode == "pm") {
		MODEL_TYPE = PM;
	}	
	else if (link_mode == "DT" || link_mode == "dt") {
		MODEL_TYPE = DT;
		_dtPronounLinker = _new DTPronounLinker();
	}
	else if (link_mode != "") {
		throw UnexpectedInputException(
			"ArabicPronounLinker::ArabicPronounLinker()",
			"Parameter 'pronoun_link_mode' must be set to 'PM' or 'DT'");
	}

}


ArabicPronounLinker::~ArabicPronounLinker(){
//	delete _pmPronounLinker;
	delete _dtPronounLinker;
}

int ArabicPronounLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID, EntityType linkType, 
								LexEntitySet *results[], int max_results) 
{ 	

	if (MODEL_TYPE == DT)
		return _dtPronounLinker->linkMention(currSolution, currMentionUID,
										linkType, results, max_results);
	else
//		return _pmPronounLinker->linkMention(currSolution, currMentionUID,
//										linkType, results, max_results);
//	std::cout<<"mrf 3-28 linking: "<<currMentionUID <<" max_results: "<<max_results<<std::endl;
	if(!_doLink){
		Mention *currMention = currSolution->getMention(currMentionUID);
		EntityGuess* guess = 0;

		guess = _guessNewEntity(currMention, linkType);
			// only one branch here - desc linking is deterministic
		LexEntitySet* newSet = currSolution->fork();
		if (guess->id == EntityGuess::NEW_ENTITY) {
			newSet->addNew(currMention->getUID(), guess->type);
		}
		results[0] = newSet;

		// MEMORY: a guess was definitely created above, and is removed now.
		delete guess;
		return 1;
	}

	if(_discard_pron){
		results[0] = currSolution->fork();
		return 1;
	}
	//else

	Mention *currMention = currSolution->getMention(currMentionUID);
	const SynNode *pronounNode = currMention->node;
	LinkGuess linkGuesses[64];

	max_results = max_results>64 ? 64 : max_results;

	int i, nResults = 0;
	bool alreadyProcessedNullDecision = false;
	//these will be used to determine entity linking for this pronoun
	const SynNode *guessedNode;
	Mention *guessedMention;
	Entity *guessedEntity;
	int nGuesses = _getClosestHeadMatch(currMention, linkGuesses, max_results, 
		currSolution->getMentionSet(currMention->getSentenceNumber()));

	for(i=0; i<nGuesses; i++) {
		guessedNode = linkGuesses[i].guess;
		//determine mention from node
		if(guessedNode != NULL)
			guessedMention = currSolution->getMentionSet(linkGuesses[i].sentence_num)
				->getMentionByNode(guessedNode);
		else guessedMention = NULL;

		//determine Entity from Mention
		if(guessedMention != NULL)
			guessedEntity = currSolution->getEntityByMention(guessedMention->getUID());
		else guessedEntity = NULL;

		//many different pronoun resolution decisions can lead to a "no-link" result.
		//therefore, only fork the LexEntitySet for the top scoring no-link guess.
		if(guessedEntity != NULL || !alreadyProcessedNullDecision) {
			LexEntitySet *newSet = currSolution->fork();

			if(guessedMention != NULL) {
				newSet->getMention(currMention->getUID())->setEntityType(guessedMention->getEntityType());
				_debugStream << "chose mention " << guessedMention->getUID() << " type " << guessedMention->getMentionType() << "\n";
			} else {
				_debugStream << "NULL mention\n";
			}

			if(guessedEntity == NULL) {
				alreadyProcessedNullDecision = true;
				_debugStream << "NULL entity\n";
			} 
			else {
				/*std::cout<<"\t\tLink pron to entity "<<std::endl;
				std::cout<<"\n\t\tpronoun:   ";
				currMention->getNode()->dump(std::cout);
				std::cout<<"\n\t\tmention:   ";
				guessedMention->dump(std::cout);
				guessedMention->getNode()->dump(std::cout);
				std::cout<<std::endl;
				*/

				newSet->add(currMention->getUID(), guessedEntity->getID());
				_debugStream << "linked to entity " << guessedEntity->getID() << "\n";
			}
			results[nResults++] = newSet;

		}
		//otherwise we ignore this pronoun
		
	}
	return nResults;

	

}
int ArabicPronounLinker::_getNodeAncestors(const SynNode* node, const SynNode* ancestors[],  int childNum[], int max){
	const SynNode* temp = node;
	int nAnc=0;
	while(temp->getParent() != 0 && nAnc < max ){
		ancestors[nAnc] = temp->getParent();
		childNum[nAnc] = -1;
		for(int k =0; k<temp->getParent()->getNChildren(); k++){
			childNum[nAnc] = -1;
			if(temp->getParent()->getChild(k) == temp){
				childNum[nAnc] = k;
				break;
			}
		}
		nAnc++;
	}
	return nAnc;
}
EntityGuess* ArabicPronounLinker::_guessNewEntity(Mention *ment, EntityType linkType)
{
	// MEMORY: linkMention, the caller of this method, is guaranteed to delete this
	EntityGuess* guess = _new EntityGuess();
	guess->id = EntityGuess::NEW_ENTITY;
	guess->score = 1;
	guess->type = linkType;
	return guess;
}

int ArabicPronounLinker::_getClosestHeadMatch(Mention *currMention, LinkGuess* guesses, int max_guesses,
										const MentionSet* currMentionSet){
	const int MAX_CANDIDATES = 50;
	HobbsDistance::SearchResult candidates[MAX_CANDIDATES];
	

	const SynNode* pronNode = currMention->getNode();
	const SynNode* topNode = pronNode;
	while(topNode->getParent() !=0){
		topNode = topNode->getParent();
	}
	// pronounTNG is the (type, number, gender) tuple of the pronoun.
	// first, retrieve hobbs candidates
	bool matched = false;
	int pronNodeStartToken = pronNode->getStartToken();
	int distToClosetsACEMention = -1;
	const Mention* closestACEMention =  0;
	const Mention* closestMention  = 0;
	const Mention* closestMatchingACEMention =  0;
	const Mention* closestACEMentionUnmatched =  0;
	EntityType pronType = currMention->getEntityType();
	

	if(_isRelativePronoun(pronNode) || _isRegularPronoun(pronNode)){
		//ignore the mention type
		//pronType = EntityType::getUndetType();
		const SynNode* pAncestors[50];
		int pChildNum[50];
		int nPAnc = _getNodeAncestors(pronNode, pAncestors, pChildNum, 50);
		int nSkipped = 0;
		for(int i =0; i < nPAnc; i++){
			if((closestMention != 0) && (closestACEMention != 0)){
				break;
			}
			if(pAncestors[i] == 0){
//				std::cout << "skipping null ancestor!"<<std::endl;
				continue;
			}
			if(pAncestors[i] == 0){
//				std::cout << "skipping ancestor w/o childnum!"<<std::endl;
				continue;
			}
			for(int j = pChildNum[i] - 1; j >= 0; j--){
				const SynNode* node = pAncestors[i]->getChild(j);
				if(node->hasMention()){
					const Mention* ment = currMentionSet->getMentionByNode(node);
					if(ment->getMentionType() != Mention::PRON){
						if(closestMention == 0){
							closestMention = ment;
						}
						if(ment->getEntityType().isDetermined() && ment->getEntityType() != EntityType::getOtherType()){
							if(!_genderNumberClash(ment, pronNode, currMention->getSentenceNumber())){
								if(pronType == EntityType::getUndetType()){
									if(closestACEMention == 0){	
										closestACEMention = ment;
									}
								}
								else{
//									std::cout<<"typed pronoun: "<<pronType.getName().to_debug_string()
//										<<" "<<pronNode->getHeadWord().to_debug_string()<<std::endl;
									if(pronType == ment->getEntityType()){
										closestACEMention = ment;
									}
									else{
										nSkipped++;
										if(closestACEMentionUnmatched == 0){
											closestACEMentionUnmatched = ment;
										}
//										std::cout<<"ArabicPronounLinker: skipping incorrect EntityType mention"<<std::endl;
									}
								}

								
							}
							/*
							else if(closestACEMention == 0){
								std::cout<<"skipping clash mention: \nPrononoun"<<std::endl;

								int pronsent = currMention->getSentenceNumber();
								if(pronsent >= _previousPOSSeq.length()){
									std::cout<<"no posseq"<<std::endl;
								}
								else if(pronNode->isPreterminal() || pronNode->isTerminal()){
									(_previousPOSSeq[pronsent]->getPOS(pronNode->getStartToken()))->dump(std::cout, 0);
								}
								else{
									_previousPOSSeq[pronsent]->getPOS(pronNode->getHeadPreterm()->getStartToken())->dump(std::cout, 0);
								}

								std::cout<<"other mention: ";
								ment->dump(std::cout, 3);
								std::cout<<std::endl;
								if(ment->getSentenceNumber() >= _previousPOSSeq.length()){
									std::cout<<"no posseq"<<std::endl;
								}
								else if(ment->getMentionType() == Mention::Type::DESC){
									const PartOfSpeech* ment_pos = _previousPOSSeq[ment->getSentenceNumber()]->getPOS(ment->getNode()->getHeadPreterm()->getStartToken());
									ment_pos->dump(std::cout, 0);
								}
								else{
									if(_mascSing->lookup(ment->getNode()->getFirstTerminal()->getHeadWord())){
										std::cout<<"Masc Name Ment: "<<std::endl;
									}
									if(_femSing->lookup(ment->getNode()->getFirstTerminal()->getHeadWord())){
										std::cout<<"Fem Name Ment: "<<std::endl;
									}
								}



							}
							*/
						}
					}
				}
			}
		}
		if((nSkipped > 3) && (closestACEMentionUnmatched != 0)){
			closestACEMention = closestACEMentionUnmatched;
			matched = true;
		}

		if(closestACEMention != 0){
			matched = true;
		}
	}
	if(matched){
		/*std::cout<<"Found ACE MATCH"<<std::endl;
		closestACEMention->dump(std::cout, 5);
		std::cout<<std::endl;
		closestACEMention->getNode()->dump(std::cout, 5);
		std::cout<<std::endl;
		currMention->dump(std::cout, 5);
		std::cout<<std::endl;
		currMention->getNode()->dump(std::cout, 5);
		std::cout<<std::endl;
		*/
		guesses[0].guess = closestACEMention->getNode();
		guesses[0].score = 1;
		guesses[0].sentence_num = closestACEMention->getSentenceNumber();
		sprintf(guesses[0].debug_string,"Linked first matching node");
		return 1;
	}
	


	int nCandidates = _getCandidates(pronNode, _previousParses, candidates, MAX_CANDIDATES);
	int nSkipped = 0;
	const Mention* firstMent = 0;
	for(int i=0; i<nCandidates; i++){
		if(candidates[i].node != 0){
			//candidates[i].node
			const Mention* tment = 	currMentionSet->getMention(candidates[0].node->getMentionIndex());
			bool type_clash = false;
			if(pronType != EntityType::getUndetType()){
//				std::cout<<"other pronoun with type: "<<pronType.getName().to_debug_string()
//					<<" "<<pronNode->getHeadPreterm()->getTag().to_debug_string()<<" "
//					<<pronNode->getHeadWord().to_debug_string()<<std::endl;
				if(pronType != currMention->getEntityType()){
					type_clash  = true;
				}
				else{
					nSkipped++;
					if((firstMent == 0)
						&& !_genderNumberClash(tment, pronNode,  currMention->getSentenceNumber()))
					{
						firstMent = tment;
					}
//					std::cout<<"ArabicPronounLinker: skipping incorrect EntityType mention (oth pronoun type)"<<std::endl;
				}
			}
			//if(_PronounAndNounAgree(pronNode, candidates[i].node)){
			if(!type_clash && !_genderNumberClash(tment, pronNode,  currMention->getSentenceNumber())){
				guesses[0].guess =candidates[i].node;
				guesses[0].score = 1;
				guesses[0].sentence_num = candidates[i].sentence_number;
				sprintf(guesses[0].debug_string,"Linked first matching node");
				return 1;
			}
			if((nSkipped > 10)  && (firstMent != 0)){
				guesses[0].guess =candidates[i].node;
				guesses[0].score = 1;
				guesses[0].sentence_num = candidates[i].sentence_number;
				sprintf(guesses[0].debug_string,"Linked first matching node");
				return 1;
			}
		}
	}
	
	guesses[0].guess = 0;
	return 1;
}

void ArabicPronounLinker::resetForNewDocument(DocTheory *docTheory) {
	if (MODEL_TYPE == PM)
		resetPartOfSpeechSequence();
//		_pmPronounLinker->resetForNewDocument(docTheory);
	else
		_dtPronounLinker->resetForNewDocument(docTheory);
}

void ArabicPronounLinker::addPreviousParse(const Parse *parse) {
	if (MODEL_TYPE == PM) {
		_previousParses.add(parse);
//		_pmPronounLinker->addPreviousParse(parse);
	}else
		_dtPronounLinker->addPreviousParse(parse);
}	

void ArabicPronounLinker::resetPreviousParses() {
	if (MODEL_TYPE == PM) {
		_previousParses.setLength(0);
//		_pmPronounLinker->resetPreviousParses();
	}else
		_dtPronounLinker->resetPreviousParses();
}

void ArabicPronounLinker::resetPartOfSpeechSequence() {
	_previousPOSSeq.setLength(0);
}
void ArabicPronounLinker::addPartOfSpeechSequence(const PartOfSpeechSequence* posSeq){
	if (MODEL_TYPE == PM) {
		_previousPOSSeq.add(posSeq);
//		_pmPronounLinker->addPartOfSpeechSequence();
	} else {
		// this needs to be implpemented
//		_dtPronounLinker->addPartOfSpeechSequence();
	}
}


bool ArabicPronounLinker::_isPossesivePronoun(const SynNode* pron){
	const SynNode* preterm = pron;
	while(!preterm->isPreterminal()){
		preterm = preterm->getHead();
	}
	Symbol prontag = preterm->getTag();
	if(prontag == ArabicSTags::PRP_POS){
		return true;
	}
	/*
	const wchar_t *tagstr = prontag.to_string();
	if(wcslen(tagstr) >=4){
		if((tagstr[0] == L'P') &&
			(tagstr[1] == L'O')&&
			(tagstr[2] == L'S')&&
			(tagstr[3] == L'S')){
				return true;
			}
	}
	*/
	return false;
}
bool ArabicPronounLinker::_isRelativePronoun(const SynNode* pron){
	const SynNode* preterm = pron;
	while(!preterm->isPreterminal()){
		preterm = preterm->getHead();
	}
	Symbol prontag = preterm->getTag();
	return prontag == ArabicSTags::WP;
}
bool ArabicPronounLinker::_isRegularPronoun(const SynNode* pron){
	const SynNode* preterm = pron;
	while(!preterm->isPreterminal()){
		preterm = preterm->getHead();
	}
	Symbol prontag = preterm->getTag();
	if(prontag == ArabicSTags::PRP){
		return true;
	}
	/*
	const wchar_t *tagstr = prontag.to_string();
	if(wcslen(tagstr) >=4){
		if((tagstr[0] == L'P') &&
			(tagstr[1] == L'R')&&
			(tagstr[2] == L'O')&&
			(tagstr[3] == L'N')){
				return true;
			}
	}
	*/
	return false;
}
		

	
	
bool ArabicPronounLinker::_PronounAndNounAgree(const SynNode* pron, const SynNode* np){
	return true;
	//agreement requires non PENN POS Tags
	/*
	Symbol gender = WordConstants::guessGender(np->getHeadWord());
	Symbol number  = WordConstants::guessNumber(np->getHeadWord());

	if(np->getTag() == ArabicSTags::NPP){
		if(_mascSing->lookup(SymbolUtilities::getFullNameSymbol(np)) && 
			_femSing->lookup(SymbolUtilities::getFullNameSymbol(np))){
				return true;
			}
		if(_mascSing->lookup(np->getHeadWord()) && _femSing->lookup(np->getHeadWord())){
			return true;
		}
		if(_mascSing->lookup(SymbolUtilities::getFullNameSymbol(np))){
			gender = ArabicWordConstants::MASC;	
			number = ArabicWordConstants::SINGULAR;
		}
		else if(_femSing->lookup(SymbolUtilities::getFullNameSymbol(np))){
			gender = ArabicWordConstants::FEM;	
			number = ArabicWordConstants::SINGULAR;
		}

		else if(_mascSing->lookup(np->getHeadWord())){
			gender = ArabicWordConstants::MASC;	
			number = ArabicWordConstants::SINGULAR;
		}
		else if(_femSing->lookup(np->getHeadWord())){
			gender = ArabicWordConstants::FEM;
			number = ArabicWordConstants::SINGULAR;
		}
		else{
			;	//probably a nationality
		}
	}


	const SynNode* preterm = pron;
	while(!preterm->isPreterminal()){
		preterm = preterm->getHead();
	}
	
	Symbol prontag = preterm->getTag();
	if(ArabicSTags::isFemalePronPOS(prontag)){
		if(gender != ArabicWordConstants::FEM) return false;
		if(number == ArabicWordConstants::PLURAL){
			if(ArabicSTags::isSingularPronPOS(prontag)){
				return false;
			}
			else if(ArabicSTags::isPluralPronPOS(prontag)){
				return true;
			}
		}
		else if((number == ArabicWordConstants::SINGULAR)){
			if(ArabicSTags::isSingularPronPOS(prontag)){
				return true;
			}
			else if(ArabicSTags::isPluralPronPOS(prontag)){
				return false;
			}
		}
	}	
	if(ArabicSTags::isMalePronPOS(prontag)){
		if(gender != ArabicWordConstants::MASC) return false;
		if(number == ArabicWordConstants::SINGULAR){
			if(ArabicSTags::isSingularPronPOS(prontag)){
				return true;
			}
			else return false;
		}
		if(number == ArabicWordConstants::PLURAL){
			if(ArabicSTags::isPluralPronPOS(prontag)){
				return true;
			}
			else return false;
		}
	}

	return true;
	
	
*/
	/*
	Symbol gender = WordConstants::guessGender(np->getHeadWord());
	Symbol number  = WordConstants::guessNumber(np->getHeadWord());

	const SynNode* preterm = pron;
	while(!preterm->isPreterminal()){
		preterm = preterm->getHead();
	}
	Symbol prontag = preterm->getTag();
	if(ArabicSTags::isFemalePronPOS(prontag)){
		if(gender != ArabicWordConstants::FEM) return false;
		if(number == ArabicWordConstants::SINGULAR){
			if(ArabicSTags::isSingularPronPOS(prontag)){
				return true;
			}
			else if(ArabicSTags::isPluralPronPOS(prontag)){
				return false;
			}
		}
		else if(number == ArabicWordConstants::PLURAL){
			if(ArabicSTags::isSingularPronPOS(prontag)){
				return false;
			}
			else if(ArabicSTags::isPluralPronPOS(prontag)){
				return true;
			}
		}
	}	
	return true;
	*/
}

/* candidates are all nodes with mentions
assume np chunk parses-
*/
int ArabicPronounLinker::_getCandidates(const SynNode* pronNode,  GrowableArray <const Parse *> &prevSentences, 
								  HobbsDistance::SearchResult results[], int max_results){

	const SynNode* temp[500];
	int ncan = 0;
	const SynNode* root = pronNode;
	const SynNode* topChildOfPron = pronNode;
	while(root->getParent() !=0){
		topChildOfPron = root;
		root = root->getParent();
	}

	for(int i=0; i< root->getNChildren(); i++){
		if(root->getChild(i) == topChildOfPron){
			break;
		}

		if(root->getChild(i)->getTag() == ArabicSTags::NP){
			const SynNode* np = root->getChild(i);

			for(int j=1; j < np->getNChildren(); j++){
				if(np->getChild(j)->getTag() == ArabicSTags::NPP){
					temp[ncan++] = np->getChild(j);
				}
				if(ncan >=500){
					break;
				}
			}
			temp[ncan++] = root->getChild(i);
		}
		if(ncan >=100){
			break;
		}
	}
	//if its a possesive pronoun, eliminate the node next to it
	if(_isPossesivePronoun(pronNode)){
		if(ncan>0 && temp[ncan-1]->getEndToken() == pronNode->getStartToken()-1){
			ncan--;
		}
	}
	
	int j =0;
	for(int k=ncan-1; k>= 0; k--){
		results[j].node = temp[k];
		results[j].sentence_number = prevSentences.length();
		j++;
		if(j >= max_results){
			break;
		}
	}
	
	return j;


 }



void ArabicPronounLinker::loadSymbolHash(SymbolHash *hash, const char* file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	if (stream.fail()) {
		return;
	}

	std::wstring line;
	while (!stream.eof()) {
		stream.getLine(line);
		if (line.size() == 0 || line.at(0) == L'#')
			continue;
		std::transform(line.begin(), line.end(), line.begin(), towlower);
		Symbol lineSym(line.c_str());
		hash->add(lineSym);
	}

	stream.close();
}
bool ArabicPronounLinker::_genderNumberClash(const Mention* ment, const SynNode* pronNode, int pronsent){
	return false;
	if(pronsent >= _previousPOSSeq.length()){
		return false;
	}
	if(ment->getSentenceNumber() >= _previousPOSSeq.length()){
		return false;
	}
	const PartOfSpeech* pron_pos;
	if(pronNode->isPreterminal() || pronNode->isTerminal()){
		pron_pos = _previousPOSSeq[pronsent]->getPOS(pronNode->getStartToken());
	}
	else{
		pron_pos =  _previousPOSSeq[pronsent]->getPOS(pronNode->getHeadPreterm()->getStartToken());
	}
	if(ment->getMentionType() == Mention::DESC){
		const PartOfSpeech* ment_pos = _previousPOSSeq[ment->getSentenceNumber()]->getPOS(ment->getNode()->getHeadPreterm()->getStartToken());


		if(LanguageSpecificFunctions::POSTheoryContainsPronFem(pron_pos)){
			if(LanguageSpecificFunctions::POSTheoryContainsNounMasc(ment_pos))
			{
				return true;
			}
		}
		if(LanguageSpecificFunctions::POSTheoryContainsPronMasc(pron_pos)){
			if(LanguageSpecificFunctions::POSTheoryContainsNounFem(ment_pos))
			{
				return true;
			}
		}
		if(LanguageSpecificFunctions::POSTheoryContainsPronPl(pron_pos)){
			if(LanguageSpecificFunctions::POSTheoryContainsNounSG(ment_pos))
			{
				return true;
			}
		}
		if(LanguageSpecificFunctions::POSTheoryContainsPronSg(pron_pos)){
			if(LanguageSpecificFunctions::POSTheoryContainsNounPL(ment_pos))
			{
				return true;
			}
		}
	}
	if((ment->getMentionType() == Mention::NAME) && ment->getEntityType().matchesPER()){
		//if nationalities are PER NAME, you would need to handle it here!
		if(_mascSing->lookup(ment->getNode()->getFirstTerminal()->getHeadWord())){
			if(LanguageSpecificFunctions::POSTheoryContainsPronPl(pron_pos) ||
				LanguageSpecificFunctions::POSTheoryContainsPronFem(pron_pos))
			{
					return true;
			}
		}
		//if nationalities are PER NAME, you would need to handle it here!
		if(_femSing->lookup(ment->getNode()->getFirstTerminal()->getHeadWord())){
			if(LanguageSpecificFunctions::POSTheoryContainsPronPl(pron_pos) ||
				LanguageSpecificFunctions::POSTheoryContainsPronMasc(pron_pos))
			{
					return true;
			}
		}
	}
	return false;

}

