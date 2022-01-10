// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/descriptors/ar_CompoundMentionFinder.h"
#include "Generic/CASerif/correctanswers/CorrectAnswers.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "Generic/CASerif/correctanswers/CorrectEntity.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/ParamReader.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include "Arabic/common/ar_WordConstants.h"
#include <boost/scoped_ptr.hpp>


Symbol ArabicCompoundMentionFinder::_SYM_of;

int ArabicCompoundMentionFinder::_n_partitive_headwords;
Symbol ArabicCompoundMentionFinder::_partitiveHeadwords[1000];

ArabicCompoundMentionFinder::ArabicCompoundMentionFinder()
{
	initializeSymbols();
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
}

ArabicCompoundMentionFinder::~ArabicCompoundMentionFinder() { }

//Not looking for compound mentions yet, so always return 0?

Mention *ArabicCompoundMentionFinder::findPartitiveWholeMention(
	MentionSet *mentionSet, Mention *baseMention)
{

	const SynNode *node = baseMention->node;
	//partitive words are mentions, will have already been classified as partitives,
	//let the nested mention classifer handle this
	const SynNode *headNode = node->getHead();
	if(headNode->hasMention()){
		Mention* headMent = mentionSet->getMentionByNode(headNode);
		if(headMent->mentionType == Mention::PART)
			return 0;
	}



	//std::cout<<"CompundMenionFinder()-Check for Partitive Word: "<<node->getHeadWord().to_debug_string()<<std::endl;
	// see if head node is considered a partitive headword
	bool found_partitive = false;
	if ((isPartitiveHeadWord(node->getHeadWord()) || isNumber(node))){
		found_partitive = true;
	}
    	//do a looser check over similar words
	Symbol word = node->getHeadWord();
	Symbol word_variants[5];
	if(!found_partitive){
		word = ArabicWordConstants::removeAl(node->getHeadWord());
		if(isPartitiveHeadWord(word)){
			found_partitive = true;
		}
	}
	if(!found_partitive){
		int nvar = ArabicWordConstants::getFirstLetterAlefVariants(word, word_variants, 5);
		for(int i =0; i<nvar; i++){
			if(isPartitiveHeadWord(word_variants[i])){
				found_partitive = true;
				break;
			}
		}
	}
	if(!found_partitive){
		int nvar = ArabicWordConstants::getLastLetterAlefVariants(word, word_variants, 5);
		for(int i =0; i<nvar; i++){
			if(isPartitiveHeadWord(word_variants[i])){
				found_partitive = true;
				break;
			}
		}
	}
	if(!found_partitive){
		return 0;
	}
	//std::cout<<"\tFound a Partitive HeadWord: "<<node->getHeadWord().to_debug_string()<<std::endl;
	/* we're dealing with NP chunk parses here
	//partitives should look like
	//(.... (NP .... PART-WORD ...) (PREP mn) (NP ENT-Word ... )
	//
	*/
	const SynNode *ppNode = 0;

	const SynNode *parentNode = node->getParent();
	if(parentNode ==0 ){
		//std::cout<<"\tNode doesn't have parent: "<<std::endl;
		return 0;
	}

	int childnum =-1;
	for(int i =0; i< parentNode->getNChildren(); i++){
		if(parentNode->getChild(i) == node){
			childnum = i;
			break;
		}
	}
	if(childnum == -1){
		throw UnexpectedInputException(
			"ArabicCompoundMentionFinder::FindPartitiveWholeMention()",
			"can't find childs place in parent");
		return 0;
	}
	//look for partitive in parent of this partitive
	if((childnum+2) < parentNode->getNChildren() &&
		(parentNode->getChild(childnum+1)->getHeadWord() == _SYM_of))
	{
		const SynNode* nextNP = parentNode->getChild(childnum +2);
		if(nextNP->hasMention()){
			_dbgPrintPartitives(node, nextNP, mentionSet, "Partitive in Parent");
			return mentionSet->getMentionByNode(nextNP);
		}
		else if(nextNP->getHead()->hasMention()){
			_dbgPrintPartitives(node, nextNP, mentionSet, "Partitive in Parent's Head");
			return mentionSet->getMentionByNode(nextNP->getHead());
		}
	}
	const SynNode *grandParentNode = parentNode->getParent();
	if(grandParentNode == 0){
		return 0;
	}
	//look for partitive in grandparent- handles bad np chunks (NP Head ... Part) OF (.....)
	int grandchildnum = childnum;
	childnum =-1;
	for(int j =0; j< grandParentNode->getNChildren(); j++){
		if(grandParentNode->getChild(j) == parentNode){
			childnum = j;
			break;
		}
	}
	if(childnum == -1){
		throw UnexpectedInputException(
			"ArabicCompoundMentionFinder::FindPartitiveWholeMention()",
			"can't find parents place in grandparent");
		return 0;
	}
	if((childnum+2) < grandParentNode->getNChildren() &&
		(grandParentNode->getChild(childnum+1)->getHeadWord() == _SYM_of))
	{
		const SynNode* nextNP = grandParentNode->getChild(childnum +2);
		if(nextNP->hasMention()){
			_dbgPrintPartitives(node, nextNP, mentionSet, "Partitive in Grandparent");

			return mentionSet->getMentionByNode(nextNP);
		}
		else if(nextNP->getHead()->hasMention()){
			_dbgPrintPartitives(node, nextNP, mentionSet, "Partitive in Grandparent's Head");
			return mentionSet->getMentionByNode(nextNP->getHead());
		}
	}

	return 0;



//	return 0;
}
Mention **ArabicCompoundMentionFinder::findAppositiveMemberMentions(
	MentionSet *mentionSet, Mention *baseMention)
{
	//zero out the mention buffer
	for(int x = 0; x<MAX_RETURNED_MENTIONS+1; x++){
		_mentionBuf[x]=0;
	}

	//We check to see that the Mention has exactly two children,
	//one NAME and one DESCRIPTOR

	const SynNode *node = baseMention->node;

	//Check for exactly two children
	if(node->getNChildren() != 2){
		return 0;
	}

	//Grab the children
	const SynNode *firstChild = node->getChild(0);
	const SynNode *secondChild = node->getChild(1);

	//both children have to be mentions
	if (!(firstChild->hasMention() && secondChild->hasMention())){
		return 0;
	}

	//Get the mentions
	_mentionBuf[0] = mentionSet->getMentionByNode(firstChild);
	_mentionBuf[1] = mentionSet->getMentionByNode(secondChild);

	//One has to be a name and the other a descriptor
	if((_mentionBuf[0]->mentionType== Mention::NAME) && (_mentionBuf[1]->mentionType== Mention::DESC)){

		//check that the types of the two mentions match
		if(_mentionBuf[0]->getEntityType() == _mentionBuf[1]->getEntityType()){
			return _mentionBuf;
		}else{
			return 0;
		}

	}else if ((_mentionBuf[0]->mentionType== Mention::DESC) && (_mentionBuf[1]->mentionType== Mention::NAME)){

		//check that the types of the two mentions match
		if(_mentionBuf[0]->getEntityType() == _mentionBuf[1]->getEntityType()){
			return _mentionBuf;
		}else{
			return 0;
		}

	}else {
		return 0;
	}
	return 0;
}
Mention **ArabicCompoundMentionFinder::findListMemberMentions(
	MentionSet *mentionSet, Mention *baseMention)
{
	return 0;
}
Mention *ArabicCompoundMentionFinder::findNestedMention(
	MentionSet *mentionSet, Mention *baseMention)
{
	const SynNode *base = baseMention->node;

	const SynNode *nestedMentionNode = 0;
	if (base->getNChildren() > 0)
		nestedMentionNode = base->getHead();
	if (nestedMentionNode != 0 && nestedMentionNode->hasMention())
		return mentionSet->getMention(nestedMentionNode->getMentionIndex());
	else
		return 0;

}

//Don't coerce Arabic Appositive types, because type is the only clue we have about
//appositives
void ArabicCompoundMentionFinder::coerceAppositiveMemberMentions(Mention **mentions){
	return;
}

void ArabicCompoundMentionFinder::initializeSymbols(){
	_SYM_of = ArabicSymbol(L"mn");
	_n_partitive_headwords = 0;
	if (!ParamReader::isParamTrue("find_partitive_mentions")) {
//		std::cout<<"***Don't Find Partitive Mentions"<<std::endl;
		return;	//since no partitive head words are iniailized, partitives will never be found!
	}

	// now read in partitive head-words
	std::string param_file = ParamReader::getRequiredParam("partitive_headword_list");
	boost::scoped_ptr<UTF8InputStream> wordStream_scoped_ptr(UTF8InputStream::build(param_file.c_str()));
	UTF8InputStream& wordStream(*wordStream_scoped_ptr);

	while (!wordStream.eof() && _n_partitive_headwords < 1000) {
		wchar_t line[100];
		wordStream.getLine(line, 100);
		if (line[0] != '\0'){
			//std::cout<<"Add Partitive Word: "<<Symbol(line).to_debug_string()<<std::endl;
			_partitiveHeadwords[_n_partitive_headwords++] = Symbol(line);
		}
	}

	wordStream.close();
}

bool ArabicCompoundMentionFinder::isPartitiveHeadWord(Symbol sym) {
	for (int i = 0; i < _n_partitive_headwords; i++) {
		if (sym == _partitiveHeadwords[i])
			return true;
	}
	return false;
}
bool ArabicCompoundMentionFinder::isNumber(const SynNode *node) {
	static Symbol CD_sym(L"NUMERIC");

	if (node->isTerminal())
		return false;
	if (node->getTag() == CD_sym)
		return true;
	return isNumber(node->getHead());
}
void ArabicCompoundMentionFinder::_dbgPrintPartitives(const SynNode* part, const SynNode* whole, MentionSet* mentSet, const char* msg){
	if(!_printDBGPartitives){
		return;
	}
	std::cout<<"*** Found a partitive"<<std::endl;
	std::cout<<"\t"<<msg<<std::endl;
	std::cout<<"Part: \n\t";
	part->dump(std::cout, 8);
	std::cout<<"\n\t";
	mentSet->getMentionByNode(part)->dump(std::cout,8);
	std::cout<<std::endl;

	std::cout<<"Whole: \n\t";
	whole->dump(std::cout, 8);
	std::cout<<"\n\t";
	mentSet->getMentionByNode(whole)->dump(std::cout,8);
	std::cout<<std::endl;
}

