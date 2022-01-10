// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/events/stat/en_EventModalityObservation.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "English/parse/en_STags.h"
#include "English/common/en_WordConstants.h"

namespace {
    static Symbol allegedly_sym(L"allegedly_sym");
}

bool EnglishEventModalityObservation::isLedbyAllegedAdverb(Proposition *prop) const {
	return (prop->getAdverb() && 
			prop->getAdverb()->getHeadWord() == allegedly_sym);
}

bool EnglishEventModalityObservation::isLedbyModalWord(Proposition *prop) const {
		// should, could, might, may
		const SynNode *modal = prop->getModal();
		if (!modal)
			return false;

		Symbol modalWord = modal->getHeadWord();
		return (modalWord == EnglishWordConstants::SHOULD ||
				modalWord == EnglishWordConstants::COULD ||
				modalWord == EnglishWordConstants::MIGHT ||
				modalWord == EnglishWordConstants::MAY);
}

bool EnglishEventModalityObservation::isFollowedbyIFWord(Proposition *prop) const {
	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
		if (arg->getType() == Argument::PROPOSITION_ARG &&
			(arg->getRoleSym() == EnglishWordConstants::IF || 
			 arg->getRoleSym() == EnglishWordConstants::WHETHER))
			return true;
	}
	return false;
}

bool EnglishEventModalityObservation::isLedbyIFWord(Proposition *prop) const {
	if (prop->getPredType() == Proposition::VERB_PRED) {
		// get "if..." unrepresented in propositions
		const SynNode* node = prop->getPredHead();
		while (node != 0) {
			if (node->getTag() != EnglishSTags::VP &&
				node->getTag() != EnglishSTags::S &&
				node->getTag() != EnglishSTags::SBAR &&
				!node->isPreterminal())
				break;
			if (node->getTag() == EnglishSTags::SBAR &&
				(node->getHeadWord() == EnglishWordConstants::IF ||
				 (node->getHeadIndex() == 0 &&
				  node->getNChildren() > 1 &&
				  node->getChild(1)->getTag() == EnglishSTags::IN &&
				  node->getChild(1)->getHeadWord() == EnglishWordConstants::IF)))
				{
					return true;
				}
			node = node->getParent();								
		}
	}
	return false;
}

bool EnglishEventModalityObservation::parentIsLikelyNonAsserted(Proposition *prop, const PropositionSet *propSet, const MentionSet *mentionSet) const {
	/* option 1  -- simplied version
	for (int l = 0; l < propSet->getNPropositions(); l++) {
	Proposition *prop2 = propSet->getProposition(l);

	Symbol predicate = prop2->getPredSymbol();
	if (predicate.is_null())
	continue;
	Symbol stemmed_predicate = WordNet::getInstance()->stem_verb(predicate);

	// [non-asserted prop] that: p# -- e.g. he might plan an attack
	// ... but only allow verbs, no modifiers, e.g. he might be happy that...
	// OR
	// [non-asserted word] that: p# -- he guessed that X

	for (int j = 0; j < prop2->getNArgs(); j++) {
	Argument *arg = prop2->getArg(j);
	if (arg->getType() == Argument::PROPOSITION_ARG &&
	(arg->getRoleSym() == Argument::OBJ_ROLE ||
	arg->getRoleSym() == EnglishWordConstants::THAT ||
	arg->getRoleSym() == Argument::IOBJ_ROLE) && 
	arg->getProposition() == prop)					  
	{
	if (_nonAssertedIndicators->lookup(stemmed_predicate)){
	return true;
	}

	if (isLikelyNonAsserted(prop2)  && prop->getPredType() == Proposition::VERB_PRED){
	return true;
	}
	}
	}
	}
	return false;
	*/

	// option 2
	std::vector<bool> isNonAsserted = identifyNonAssertedProps(propSet, mentionSet);
	return isNonAsserted[prop->getIndex()];
}

void EnglishEventModalityObservation::findIndicators(int token_index,
													 const TokenSequence *tokens,
													 const Parse *parse,
													 MentionSet *mentionSet)
{
    const SynNode *currentNode = parse->getRoot()->getNthTerminal(token_index)->getParent();
    const SynNode *parentNode = currentNode->getParent() ;
	std::string POStxt(_pos.to_debug_string());
	std::string parentPOStxt(parentNode->getTag().to_debug_string());

	// identify indicative words in a window of the target word
	int startIndex = 0 ;
	int windowSize = WINDOW_SIZE_VERB;
	if (POStxt[0] == 'N'){
		windowSize = WINDOW_SIZE_NOUN;
	}

	if (token_index - windowSize > 0){
		startIndex = token_index - windowSize;
	}

	// option 1 -- use a mixed word list for indicative words nearby
	for (int i = startIndex; i < token_index; i++){
		Symbol word = tokens->getToken(i)->getSymbol();
		Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
		std::string POStxt = POS.to_debug_string();
		Symbol stemmed_word= word;


		if (POStxt[0] == 'V'){
			stemmed_word = WordNet::getInstance()->stem_verb(word);
		}else if (POStxt[0] == 'N'){
			stemmed_word = WordNet::getInstance()->stem_noun(word);
		}else if (POS == Symbol(L"JJ") || POS == Symbol(L"JJR")){
			stemmed_word = WordNet::getInstance()->stem(word, 3);
		}else if (POS == Symbol(L"RB") || POS == Symbol(L"RBR")){
			stemmed_word = WordNet::getInstance()->stem(word, 4);
		}else{  // refine in 2008.04.18
			// option 1
			stemmed_word = word;

			// option 2
			//stemmed_word = SymbolUtilities::lowercaseSymbol(word);
		}

		if (_nonAssertedIndicatorsNearby->lookup(stemmed_word)){
			_hasIndicatorNearby = true;
			_indicatorNearby[_numIndicatorNearby++] = stemmed_word;
			if (_numIndicatorNearby == MAX_INDICATORS){
				break;
			}
		}
	}


	// option 2 -- use separate word lists for indicative words nearby
	/*
	if (POStxt[0] == 'N'){
	for (int i = startIndex; i < token_index; i++){
	Symbol word = tokens->getToken(i)->getSymbol();
	Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
	//std::string tempPOStxt = POS.to_debug_string();
	Symbol stemmed_word = word;
	bool findIndicator = false;
	if (POS == Symbol(L"JJ") || POS == Symbol(L"JJR")){
	stemmed_word = WordNet::getInstance()->stem(word, 3);
	if (_nonAssertedIndicatorsAdj->lookup(stemmed_word)){
	_hasIndicatorNearby = true;
	_indicatorNearby[_numIndicatorNearby++] = stemmed_word;
	findIndicator = true;
	}
	}

	if (_numIndicatorNearby == MAX_INDICATORS){
	break;
	}
	}
	}else{
	for (int i = startIndex; i < token_index; i++){
	Symbol word = tokens->getToken(i)->getSymbol();
	Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
	std::string tempPOStxt = POS.to_debug_string();
	Symbol stemmed_word = word;
	bool findIndicator = false;
	if (tempPOStxt[0] == 'V'){
	stemmed_word = WordNet::getInstance()->stem_verb(word);
	if (_nonAssertedIndicatorsVerb->lookup(stemmed_word)){
	_hasIndicatorNearby = true;
	_indicatorNearby[_numIndicatorNearby++] = stemmed_word;
	findIndicator = true;
	}
	}else if (tempPOStxt[0] == 'N'){
	stemmed_word = WordNet::getInstance()->stem_noun(word);
	if (_nonAssertedIndicatorsNoun->lookup(stemmed_word)){
	_hasIndicatorNearby = true;
	_indicatorNearby[_numIndicatorNearby++] = stemmed_word;
	findIndicator = true;
	}
	}else if (POS == Symbol(L"JJ") || POS == Symbol(L"JJR")){
	stemmed_word = WordNet::getInstance()->stem(word, 3);
	if (_nonAssertedIndicatorsAdj->lookup(stemmed_word)){
	_hasIndicatorNearby = true;
	_indicatorNearby[_numIndicatorNearby++] = stemmed_word;
	findIndicator = true;
	}
	}else if (POS == Symbol(L"RB") || POS == Symbol(L"RBR")){
	stemmed_word = WordNet::getInstance()->stem(word, 4);
	if (_nonAssertedIndicatorsAdv->lookup(stemmed_word)){
	_hasIndicatorNearby = true;
	_indicatorNearby[_numIndicatorNearby++] = stemmed_word;
	findIndicator = true;
	}
	}
	if ( ! findIndicator){
	stemmed_word = SymbolUtilities::lowercaseSymbol(stemmed_word);
	if (_nonAssertedIndicatorsOther->lookup(stemmed_word)){
	_hasIndicatorNearby = true;
	_indicatorNearby[_numIndicatorNearby++] = stemmed_word;
	}
	}

	if (_numIndicatorNearby == MAX_INDICATORS){
	break;
	}

	}
	}
	*/
	// identify indicative words (n,v,adj,adv) on the parse tree of the target word
	const SynNode *previousNode = currentNode;
	_isPremodOfMention = false;
	_isPremodOfNP = false;
	if (POStxt[0] == 'N'){
		//target word is a noun
		parentNode = currentNode->getParent();
		parentPOStxt = parentNode->getTag().to_debug_string();

		//check whether the trigger word is premodifier of a mention
		for (int j = 0; j < mentionSet->getNMentions(); j++) {
			Mention *mention = mentionSet->getMention(j);

			const SynNode *mentionHead;
			if (mention->getHead() != NULL){
				mentionHead = mention->getHead();
			}else{
				mentionHead = mention->node->getHead();
			}


			if (parentNode->getStartToken() == mention->node->getStartToken() &&
				parentNode->getEndToken() == mention->node->getEndToken() && 
				(currentNode->getStartToken() != mentionHead->getStartToken() || currentNode->getEndToken() != mentionHead->getEndToken())){
					_isPremodOfMention = true;
					Mention::Type type = mention->mentionType;

					switch (type) {
					case Mention::NAME:
						_mentionType=Symbol(L"NAME");
						break;
					case Mention::DESC:
						_mentionType=Symbol(L"DESC");
						break;
					case Mention::PRON:
						_mentionType=Symbol(L"PRON");
						break;
					case Mention::PART:
					case Mention::APPO:
					case Mention::LIST:
					case Mention::NEST:
					default:
						_mentionType=Symbol(L"OTHER");
					}

					_entityType = mention->getEntityType().getName();
					break;
			}
		}

		//check whether the trigger word is premodifier of a base NP
		if (parentPOStxt[0] == 'N' && parentPOStxt[1] == 'P'){
			if (parentNode->getHead() != currentNode){
				_isPremodOfNP = true;
			}
		}

		while (parentPOStxt[0] == 'N' && parentPOStxt[1] == 'P'){
			//check whether the trigger word is part of a named entity
			//if (parentPOStxt[2] == 'P' && parentPOStxt[3] == '\0'){
			//	_isPartOfNamedEntity = true;  // undefined
			//}

			previousNode = parentNode;
			parentNode = parentNode->getParent();
			if (parentNode == NULL){
				break;
			}else{
				parentPOStxt = parentNode->getTag().to_debug_string();
			}
		}
		int start = previousNode->getStartToken();
		int end = currentNode->getStartToken();
		for (int i=start; i<end; i++){
			Symbol word = tokens->getToken(i)->getSymbol();
			Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
			if (POS == Symbol(L"JJ") || POS == Symbol(L"JJR")){
				Symbol stemmed_word = WordNet::getInstance()->stem(word, 3);
				if (_nonAssertedIndicatorsAdj->lookup(stemmed_word)){
					_hasIndicatorModifyingNoun = true;
					_indicatorModifyingNoun[_numIndicatorModifyingNoun++] = stemmed_word;
					if (_numIndicatorModifyingNoun == MAX_INDICATORS){
						break;
					}
				}
			}
		}
		if (parentPOStxt[0] == 'V' && parentPOStxt[1] == 'P'){
			Symbol headverb = WordNet::getInstance()->stem_verb(parentNode->getHeadWord());
			if (! _verbsCausingEvents->lookup(headverb)){
				parentPOStxt[0]='X';
			}
		}
	}else if (POStxt[0] == 'V'){
		//target word is a verb
		parentNode = currentNode->getParent();
		//const SynNode *previousNode = currentNode;
		parentPOStxt = parentNode->getTag().to_debug_string();
	}


	// this part deal with two cases: verb predicate; noun predicate as a direct object of verb 
	if (parentPOStxt[0] == 'V' && parentPOStxt[1] == 'P'){

		while (parentPOStxt[0] == 'V' && parentPOStxt[1] == 'P'){
			// added on 20080428
			// check 1. Modal verbs 
			// modified on 2008/05/07
			/*
			int start = parentNode->getStartToken();
			int end = previousNode->getStartToken();
			for (int i=start; i<end; i++){
			Symbol word = tokens->getToken(i)->getSymbol();
			Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
			if (POS == Symbol(L"MD")){
			Symbol stemmed_word = SymbolUtilities::lowercaseSymbol(word);
			if (_nonAssertedIndicatorsMD->lookup(stemmed_word)){
			_hasIndicatorMDAboveVP = true;
			_indicatorMDAboveVP[_numIndicatorMDAboveVP++] = stemmed_word;

			if (_numIndicatorMDAboveVP == MAX_INDICATORS){
			break;
			}
			}
			}
			}
			*/

			previousNode = parentNode;
			parentNode = parentNode->getParent();
			if (parentNode == NULL){
				break;
			}else{
				parentPOStxt = parentNode->getTag().to_debug_string();
			}
		}

		if (parentPOStxt[0] == 'S'){
			// find indicator words from the verb adjuncts
			int start = parentNode->getStartToken();
			int end = currentNode->getStartToken();
			for (int i=start; i<end; i++){
				Symbol word = tokens->getToken(i)->getSymbol();
				Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();

				// modified on 2008/05/05, adding "modal" word searching
				if (POS == Symbol(L"RB") || POS == Symbol(L"RBR")){
					Symbol stemmed_word = WordNet::getInstance()->stem(word, 4);
					if (_nonAssertedIndicatorsAdv->lookup(stemmed_word)){
						_hasIndicatorAboveVP = true;
						if (_numIndicatorAboveVP < MAX_INDICATORS){					
							_indicatorAboveVP[_numIndicatorAboveVP++] = stemmed_word;
						}
					}
				}else if (POS == Symbol(L"MD")){
					Symbol stemmed_word = SymbolUtilities::lowercaseSymbol(word);
					if (_nonAssertedIndicatorsMD->lookup(stemmed_word)){
						_hasIndicatorMDAboveVP = true;
						if (_numIndicatorMDAboveVP < MAX_INDICATORS){
							_indicatorMDAboveVP[_numIndicatorMDAboveVP++] = stemmed_word;
						}				
					}
				}
			}

			// continue to search high level 
			while (parentPOStxt[0] == 'S'){
				previousNode = parentNode;
				parentNode = parentNode->getParent();
				if (parentNode != NULL){
					parentPOStxt = parentNode->getTag().to_debug_string();
					if (parentNode->getTag() == Symbol(L"SBAR")){
						// added on 20080428
						// check 1. if, whether 
						int start = parentNode->getStartToken();
						int end = previousNode->getStartToken();
						for (int i=start; i<end; i++){
							Symbol word = tokens->getToken(i)->getSymbol();
							Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
							// modified on 2008/05/07   add modal word searching
							if (POS == Symbol(L"IN")){
								Symbol stemmed_word = SymbolUtilities::lowercaseSymbol(word);
								if (stemmed_word == Symbol(L"if") || stemmed_word == Symbol(L"whether")){
									_hasIndicatorAboveS = true;
									if (_numIndicatorAboveS < MAX_INDICATORS){
										_indicatorAboveS[_numIndicatorAboveS++] = stemmed_word;
									}
								}
							}else if (POS == Symbol(L"MD")){
								Symbol stemmed_word = SymbolUtilities::lowercaseSymbol(word);
								if (_nonAssertedIndicatorsMD->lookup(stemmed_word)){
									_hasIndicatorMDAboveVP = true;
									if (_numIndicatorMDAboveVP < MAX_INDICATORS){
										_indicatorMDAboveVP[_numIndicatorMDAboveVP++] = stemmed_word;
									}
								}
							}				
						}
					}

				}else{
					break;
				}
			}

			if (parentNode != NULL){
				if (parentPOStxt[0] == 'N' && parentPOStxt[1] == 'P'){
					int start = parentNode->getStartToken();
					// modified on 5/21/2008
					//int end = currentNode->getStartToken();
					int end = previousNode->getStartToken();
					for (int i=start; i<end; i++){
						Symbol word = tokens->getToken(i)->getSymbol();
						Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
						std::string POStxt = POS.to_debug_string();
						if (POStxt[0] == 'N'){
							Symbol stemmed_word = WordNet::getInstance()->stem_noun(word);
							if (_nonAssertedIndicatorsNoun->lookup(stemmed_word)){
								_hasIndicatorAboveS = true;
								_indicatorAboveS[_numIndicatorAboveS++] = stemmed_word;
								if (_numIndicatorAboveS == MAX_INDICATORS){
									break;
								}
							}
						}
					}
				}else if (parentNode->getTag() == Symbol(L"VP")){
					int start = parentNode->getStartToken();
					// modified on 5/21/2008
					//int end = currentNode->getStartToken();
					int end = previousNode->getStartToken();
					for (int i=start; i<end; i++){
						Symbol word = tokens->getToken(i)->getSymbol();
						Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
						std::string POStxt = POS.to_debug_string();
						if (POStxt[0] == 'V'){
							Symbol stemmed_word = WordNet::getInstance()->stem_verb(word);
							if (_nonAssertedIndicatorsVerb->lookup(stemmed_word)){
								_hasIndicatorAboveS = true;
								_indicatorAboveS[_numIndicatorAboveS++] = stemmed_word;
								if (_numIndicatorAboveS == MAX_INDICATORS){
									break;
								}
							}
						}
					} 
				}else if (parentNode->getTag() == Symbol(L"ADJP")){
					int start = parentNode->getStartToken();
					// modified on 5/21/2008
					//int end = currentNode->getStartToken();
					int end = previousNode->getStartToken();
					for (int i=start; i<end; i++){
						Symbol word = tokens->getToken(i)->getSymbol();
						Symbol POS = parse->getRoot()->getNthTerminal(i)->getParent()->getTag();
						std::string POStxt = POS.to_debug_string();
						if (POStxt[0] == 'J' && POStxt[1] == 'J'){
							Symbol stemmed_word = WordNet::getInstance()->stem(word, 3);
							if (_nonAssertedIndicatorsAdj->lookup(stemmed_word)){
								_hasIndicatorAboveS = true;
								_indicatorAboveS[_numIndicatorAboveS++] = stemmed_word;
								if (_numIndicatorAboveS == MAX_INDICATORS){
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}

bool EnglishEventModalityObservation::isLikelyNonAsserted (Proposition *prop) const {
	if (EventModalityObservation::isLikelyNonAsserted(prop))
		return true;

	// should, could, might, may
	const SynNode *modal = prop->getModal();
	if (modal != 0) {
		Symbol modalWord = modal->getHeadWord();
		if (modalWord == EnglishWordConstants::SHOULD ||
			modalWord == EnglishWordConstants::COULD ||
			modalWord == EnglishWordConstants::MIGHT ||
			modalWord == EnglishWordConstants::MAY)
			return true;
	}

	// if/whether: p#
	// not ALWAYS accurate, but close enough	
	for (int j = 0; j < prop->getNArgs(); j++) {
		Argument *arg = prop->getArg(j);
		if (arg->getType() == Argument::PROPOSITION_ARG &&
			(arg->getRoleSym() == EnglishWordConstants::IF || 
			arg->getRoleSym() == EnglishWordConstants::WHETHER))
		{
			return true;
		}
	}

	if (prop->getPredType() == Proposition::VERB_PRED) {
		// get "if..." unrepresented in propositions
		const SynNode* node = prop->getPredHead();
		while (node != 0) {
			if (node->getTag() != EnglishSTags::VP &&
				node->getTag() != EnglishSTags::S &&
				node->getTag() != EnglishSTags::SBAR &&
				!node->isPreterminal())
				break;
			if (node->getTag() == EnglishSTags::SBAR &&
				(node->getHeadWord() == EnglishWordConstants::IF ||
				(node->getHeadIndex() == 0 &&
				node->getNChildren() > 1 &&
				node->getChild(1)->getTag() == EnglishSTags::IN &&
				node->getChild(1)->getHeadWord() == EnglishWordConstants::IF)))
			{
				return true;
			}
			node = node->getParent();								
		}
	}

	return false;
}


std::vector<bool> EnglishEventModalityObservation::identifyNonAssertedProps(const PropositionSet *propSet, 
																			const MentionSet *mentionSet) const
{
	std::vector<bool> isNonAsserted(EventModalityObservation::identifyNonAssertedProps(propSet, mentionSet));

	for (int l = 0; l < propSet->getNPropositions(); l++) {
		Proposition *prop = propSet->getProposition(l);

		Symbol predicate = prop->getPredSymbol();
		if (predicate.is_null())
			continue;
		Symbol stemmed_predicate = WordNet::getInstance()->stem_verb(predicate);

		// [non-asserted prop] that: p# -- e.g. he might plan an attack
		// ... but only allow verbs, no modifiers, e.g. he might be happy that...
		// OR
		// [non-asserted word] that: p# -- he guessed that X

		if (_nonAssertedIndicators->lookup(stemmed_predicate) ||
			(isNonAsserted[prop->getIndex()] && prop->getPredType() == Proposition::VERB_PRED))
		{
			for (int j = 0; j < prop->getNArgs(); j++) {
				Argument *arg = prop->getArg(j);
				if (arg->getType() == Argument::PROPOSITION_ARG &&
					(arg->getRoleSym() == Argument::OBJ_ROLE ||
					arg->getRoleSym() == EnglishWordConstants::THAT ||
					arg->getRoleSym() == Argument::IOBJ_ROLE))					  
				{
					isNonAsserted[arg->getProposition()->getIndex()] = true;
				}
			}
		}
	}

	return isNonAsserted;
}
