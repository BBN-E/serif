// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/relations/ar_PotentialRelationInstance.h"
#include "Generic/relations/RelationTypeSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/theories/Argument.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/RelationConstants.h"
#include "Arabic/BuckWalter/ar_FeatureValueStructure.h"
#include "Arabic/BuckWalter/ar_BWNormalizer.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/LexicalTokenSequence.h"

Symbol ArabicPotentialRelationInstance::TOO_LONG = Symbol(L":TOO_LONG");
Symbol ArabicPotentialRelationInstance::ADJACENT = Symbol(L":ADJACENT");
Symbol ArabicPotentialRelationInstance::CONFUSED = Symbol(L":CONFUSED");

// 0 left mention type
// 1 right mention type
// 2 words between mentions

ArabicPotentialRelationInstance::ArabicPotentialRelationInstance() {
	for (int i = 0; i < AR_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		_ngram[i] = NULL_SYM;
	}
}

void ArabicPotentialRelationInstance::setFromTrainingNgram(Symbol *training_instance)
{
	// call base class version
	PotentialRelationInstance::setFromTrainingNgram(training_instance);

	setLeftMentionType(training_instance[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE]);
	setRightMentionType(training_instance[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+1]);
	setWordsBetween(training_instance[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+2]);

}

Symbol* ArabicPotentialRelationInstance::getTrainingNgram() {
	
	for (int i = 0; i < POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		_ngram[i] = PotentialRelationInstance::_ngram[i];
	}
	return _ngram;
}

void ArabicPotentialRelationInstance::setStandardInstance(Mention *first, Mention *second, 
														  const TokenSequence *tokenSequence) 
{

	// set relation type to default (NONE)
	setRelationType(RelationConstants::NONE);

	// set mention indicies
	_leftMention = first->getUID();
	_rightMention = second->getUID();
	
	// set head words - make an exception for APPO - we want the head word to be the head word of the DESC
	if (first->getMentionType() == Mention::APPO) 
		setLeftHeadword(first->getChild()->getNode()->getHeadWord());
	else
		setLeftHeadword(first->getNode()->getHeadWord());
	if (second->getMentionType() == Mention::APPO) 
		setRightHeadword(second->getChild()->getNode()->getHeadWord());
	else
		setRightHeadword(second->getNode()->getHeadWord());

	// set the rest of the basics
	setNestedWord(NULL_SYM); 
	setLeftEntityType(first->getEntityType().getName());
	setRightEntityType(second->getEntityType().getName());
	setLeftRole(NULL_SYM);  // first->getRoleSym()
	setRightRole(NULL_SYM);	// second->getRoleSym()	
	setNestedRole(NULL_SYM); 
	setReverse(NULL_SYM); 
	setLeftMentionType(first->getMentionType());
	setRightMentionType(second->getMentionType());
	setWordsBetween(findWordsBetween(first, second, tokenSequence));

	const SynNode *firstChunk = findNPChunk(first->getNode());
	const SynNode *secondChunk = findNPChunk(second->getNode());
	// check to see if the mentions belong to the same NP Chunk
	if (firstChunk != 0 && secondChunk != 0) {
		if (firstChunk == secondChunk) {
			setPredicate(firstChunk->getHeadWord());
			setStemmedPredicate(findStem(firstChunk, tokenSequence));
			if (first->getNode()->getHeadWord() == firstChunk->getHeadWord()) {
				setLeftRole(Argument::REF_ROLE);
				// do something in here to handle CONJ?
				setRightRole(Argument::UNKNOWN_ROLE);
			}
			else {
				setLeftRole(Argument::UNKNOWN_ROLE);
				setRightRole(Argument::UNKNOWN_ROLE);
			}
		}
		else {
			setPredicate(firstChunk->getHeadWord());
			setStemmedPredicate(findStem(firstChunk, tokenSequence));
			if (firstChunk->getHeadWord() == first->getNode()->getHeadWord()) 
				setLeftRole(Argument::REF_ROLE);
			else
				setLeftRole(Argument::UNKNOWN_ROLE);

			// find right role
			const SynNode *top = firstChunk->getParent();
			int i = 0;
			while (top->getChild(i) != firstChunk)
				i++;
			// skip past first chunk
			i++;
			wchar_t buf[100];
			wcscpy(buf, L"");
			while (top->getChild(i) != secondChunk) {	
				if (!LanguageSpecificFunctions::isNPtypeLabel(top->getChild(i)->getTag())) {
					if (wcscmp(buf, L"") != 0)
						wcscat(buf, L"_");
					wcsncat(buf, top->getChild(i)->getHeadWord().to_string(), 10);
				}
				i++;
			}
			if (wcscmp(buf, L"") != 0) 
				setRightRole(Symbol(buf));
			else
				setRightRole(Argument::UNKNOWN_ROLE);

		}
	}

	if (getLeftRole() == Argument::REF_ROLE)
		setPredicationType(ONE_PLACE);
	else setPredicationType(MULTI_PLACE);



}


Symbol ArabicPotentialRelationInstance::getLeftMentionType() { return _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE]; }
Symbol ArabicPotentialRelationInstance::getRightMentionType() { return _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+1]; }
Symbol ArabicPotentialRelationInstance::getWordsBetween() { return _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+1]; }

void ArabicPotentialRelationInstance::setLeftMentionType(Symbol sym) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE] = sym; }
void ArabicPotentialRelationInstance::setRightMentionType(Symbol sym) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+1] = sym; }
void ArabicPotentialRelationInstance::setWordsBetween(Symbol sym) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+2] = sym; }

void ArabicPotentialRelationInstance::setLeftMentionType(Mention::Type type) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE] = convertMentionType(type); }
void ArabicPotentialRelationInstance::setRightMentionType(Mention::Type type) { _ngram[POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE+1] = convertMentionType(type); }

std::wstring ArabicPotentialRelationInstance::toString() {
	std::wstring result = PotentialRelationInstance::toString();
	for (int i = POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i < AR_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		result += _ngram[i].to_string();
		result += L" ";
	}
	return result;
}
std::string ArabicPotentialRelationInstance::toDebugString() {
	std::string result = PotentialRelationInstance::toDebugString();
	for (int i = POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i < AR_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		result += _ngram[i].to_debug_string();
		result += " ";
	}
	return result;
}

Symbol ArabicPotentialRelationInstance::convertMentionType(Mention::Type type) {
	if (type == Mention::NONE) 
		return Symbol(L"NONE");
	else if (type == Mention::NAME)
		return Symbol(L"NAME");
	else if (type == Mention::PRON)
		return Symbol(L"PRON");
	else if (type == Mention::DESC)
		return Symbol(L"DESC");
	else if (type == Mention::PART)
		return Symbol(L"PART");
	else if (type == Mention::APPO)
		return Symbol(L"APPO");
	else if (type == Mention::LIST)
		return Symbol(L"LIST");
	return Symbol();
}

const SynNode* ArabicPotentialRelationInstance::findNPChunk(const SynNode *node) {
	if (node->getParent() == 0)
		return 0;
	if (node->getParent()->getParent() == 0)  {
		if (LanguageSpecificFunctions::isNPtypeLabel(node->getTag()))
			return node;
		else
			return 0;
	}
	else {
		return findNPChunk(node->getParent());
	}
}

Symbol ArabicPotentialRelationInstance::findWordsBetween(Mention *m1, Mention *m2,
														 const TokenSequence *tokenSequence) 
{
	int end1 = m1->getNode()->getEndToken();
	if (m1->getNode()->getEndToken() < m2->getNode()->getStartToken()) {
		return makeWBSymbol(m1->getNode()->getEndToken() + 1, m2->getNode()->getStartToken(), tokenSequence);
	} 
	
	const SynNode* node1 = m1->getNode()->getHeadPreterm();
	const SynNode* node2 = m2->getNode()->getHeadPreterm();
	if (node1 != m1->getNode())
		node1 = m1->getNode()->getHeadPreterm()->getParent();
	if (node2 != m2->getNode())
		node2 = m2->getNode()->getHeadPreterm()->getParent();

	if (node1->getEndToken() < node2->getStartToken()) {
		return makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken(), tokenSequence);
	} 
	
	const SynNode *temp = node2;
	node2 = m2->getNode()->getHeadPreterm();
	if (node1->getEndToken() < node2->getStartToken()) {
		return makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken(), tokenSequence);
	} 
	node2 = temp;

	node1 = m1->getNode()->getHeadPreterm();
	if (node1->getEndToken() < node2->getStartToken()) {
		return makeWBSymbol(node1->getEndToken() + 1, node2->getStartToken(), tokenSequence);
	} 

	//std::cerr << m1->getNode()->toDebugTextString() << " ~ " << m2->getNode()->toDebugTextString() << "\n";
	//std::cerr << node1->toDebugTextString() << " ~ " << node2->toDebugTextString() << "\n";
	//std::cerr << m1->getMentionType() << " " << m2->getMentionType() << "\n\n";

	return CONFUSED;
}


Symbol ArabicPotentialRelationInstance::makeWBSymbol(int start, int end, const TokenSequence *tokenSequence) {
	if (end - start > 10) {
		return TOO_LONG;
	}
	if (start == end) {
		return ADJACENT;
	}
	std::wstring str = L"";
	for (int i = start; i < end; i++) {
		str += tokenSequence->getToken(i)->getSymbol().to_string();
		if (i != end - 1)
			str += L"_";
	}
	return Symbol(str.c_str());
}

Symbol ArabicPotentialRelationInstance::findStem(const SynNode *node, const TokenSequence *tokenSequence) {
	const LexicalTokenSequence *lexicaTokenSequence = dynamic_cast<const LexicalTokenSequence*>(tokenSequence);
	if (!lexicaTokenSequence) throw InternalInconsistencyException("ArabicPotentialRelationInstance::findStem",
		"This ArabicPotentialRelationInstance requires a LexicalTokenSequence.");
	
	const SynNode *head = node->getHeadPreterm();
	int tok_num = head->getStartToken();
	const LexicalToken *token = lexicaTokenSequence->getToken(tok_num);
	LexicalEntry *lex = token->getLexicalEntry(0);
	
	Symbol stem = token->getSymbol();
	for (int i = 0; i < lex->getNSegments(); i++) {
		FeatureValueStructure *fvs = lex->getSegment(i)->getFeatures();
		if (fvs->getCategory() == Symbol(L"B")) {
			stem = fvs->getVoweledString();
		}
	}
	stem = BWNormalizer::normalize(stem);
	
	return stem;
	
}
void ArabicPotentialRelationInstance::setStandardInstance(RelationObservation *obs) {

	// set relation type to default (NONE)
	setRelationType(RelationConstants::NONE);

	// set mention indicies
	_leftMention = obs->getMention1()->getUID();
	_rightMention = obs->getMention2()->getUID();
	
	setLeftHeadword(obs->getMention1()->getNode()->getHeadWord());
	setRightHeadword(obs->getMention2()->getNode()->getHeadWord());
	setNestedWord(NULL_SYM); 
	setLeftEntityType(obs->getMention1()->getEntityType().getName());
	setRightEntityType(obs->getMention2()->getEntityType().getName());		
	setNestedRole(NULL_SYM); 
	setReverse(NULL_SYM); 

	if (obs->getPropLink()->isEmpty()) {
		setPredicate(NULL_SYM);
		setStemmedPredicate(NULL_SYM);
		setLeftRole(NULL_SYM);
		setRightRole(NULL_SYM);
		setPredicationType(MULTI_PLACE);
		return;
	}

}
