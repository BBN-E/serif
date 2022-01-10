// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/SubtypeClassifier.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/theories/Proposition.h"
#include "Generic/relations/xx_RelationUtilities.h"

#include "Generic/parse/LanguageSpecificFunctions.h"

#include "Generic/theories/PartOfSpeechSequence.h"
#include <boost/scoped_ptr.hpp>

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

SubtypeClassifier::SubtypeClassifier()
	: _wordnetMap(0), _descHeadwordMap(0), _fullNameMap(0), 
	_nameWordMap(0), _wikiWordMap(0), _posSequence(0)
{
	if (!EntitySubtype::subtypesDefined())
		return;

	_wordnetMap = _new WordToSubtypeMap(5000);

	std::string filename = ParamReader::getParam("wordnet_subtypes");
	if (!filename.empty()) {
		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(filename.c_str()));
		UTF8InputStream& stream(*stream_scoped_ptr);
		Sexp *wordMappings = _new Sexp(stream);
		int nmappings = wordMappings->getNumChildren();
		for (int i = 0; i < nmappings; i++) {
			Sexp *mapping = wordMappings->getNthChild(i);
			Symbol word = mapping->getFirstChild()->getValue();
			Symbol subtype = mapping->getSecondChild()->getValue();
			EntitySubtype stype(subtype);

			EntitySubtypeItem *newItem = _new EntitySubtypeItem();
			newItem->entitySubtype = stype;
			newItem->next = NULL;

			if (_wordnetMap->get(word) != NULL) 
				newItem->next = *_wordnetMap->get(word);
				
			(*_wordnetMap)[word] = newItem;
		}
		delete wordMappings;
	}

	_descHeadwordMap = _new WordToSubtypeMap(5000);
	filename = ParamReader::getParam("desc_head_subtypes");	
	if (!filename.empty()) {
		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(filename.c_str()));
		UTF8InputStream& stream(*stream_scoped_ptr);
		Sexp *wordMappings = _new Sexp(stream);
		int nmappings = wordMappings->getNumChildren();
		for (int i = 0; i < nmappings; i++) {
			Sexp *mapping = wordMappings->getNthChild(i);
			int nkids = mapping->getNumChildren();
			Symbol subtype = mapping->getNthChild(nkids-1)->getValue();

			Symbol word = mapping->getFirstChild()->getValue();
			//Symbol subtype = mapping->getSecondChild()->getValue();
			EntitySubtype stype(subtype);

			EntitySubtypeItem *newItem = _new EntitySubtypeItem();
			newItem->entitySubtype = stype;
			newItem->next = NULL;

			if (_descHeadwordMap->get(word) != NULL)
				newItem->next = *_descHeadwordMap->get(word);
	
			(*_descHeadwordMap)[word] = newItem;
		}
		delete wordMappings;
	}

	_nameWordMap = _new WordToSubtypeMap(5000);
	filename = ParamReader::getParam("name_word_subtypes");	
	if (!filename.empty()) {
		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(filename.c_str()));
		UTF8InputStream& stream(*stream_scoped_ptr);
		Sexp *wordMappings = _new Sexp(stream);
		int nmappings = wordMappings->getNumChildren();
		for (int i = 0; i < nmappings; i++) {
			Sexp *mapping = wordMappings->getNthChild(i);
			Symbol word = mapping->getFirstChild()->getValue();
			Symbol subtype = mapping->getSecondChild()->getValue();
			EntitySubtype stype(subtype);

			EntitySubtypeItem *newItem = _new EntitySubtypeItem();
			newItem->entitySubtype = stype;
			newItem->next = NULL;

			if (_nameWordMap->get(word) != NULL) 
				newItem->next = *_nameWordMap->get(word);
				
			(*_nameWordMap)[word] = newItem;
		}
		delete wordMappings;
	}

	_fullNameMap = _new WordToSubtypeMap(5000);
	filename = ParamReader::getParam("full_name_subtypes");	
	if (!filename.empty()) {
		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(filename.c_str()));
		UTF8InputStream& stream(*stream_scoped_ptr);
		Sexp *wordMappings = _new Sexp(stream);
		int nmappings = wordMappings->getNumChildren();
		for (int i = 0; i < nmappings; i++) {
			Sexp *mapping = wordMappings->getNthChild(i);
			int nkids = mapping->getNumChildren();
			Symbol subtype = mapping->getNthChild(nkids-1)->getValue();

			std::wostringstream nameStr;
			for (int j = 0; j < nkids - 1; j++) {
				Symbol word = mapping->getNthChild(j)->getValue();
				nameStr << word.to_string();
				if (j < nkids - 2) nameStr << L" ";
			}
			Symbol nameSym(nameStr.str().c_str());
			EntitySubtype stype(subtype);

			EntitySubtypeItem *newItem = _new EntitySubtypeItem();
			newItem->entitySubtype = stype;
			newItem->next = NULL;

			if (_fullNameMap->get(nameSym) != NULL) 
				newItem->next = *_fullNameMap->get(nameSym);
				
			(*_fullNameMap)[nameSym] = newItem;
		}
		delete wordMappings;
	}
	//Wikipedia
	_wikiWordMap = _new WordToSubtypeMap(10000);
	filename = ParamReader::getParam("wikipedia_subtypes");	
	if (!filename.empty()) {
		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(filename.c_str()));
		UTF8InputStream& stream(*stream_scoped_ptr);
		Sexp *wordMappings = _new Sexp(stream);
		int nmappings = wordMappings->getNumChildren();
		for (int i = 0; i < nmappings; i++) {
			Sexp *mapping = wordMappings->getNthChild(i);
			int nkids = mapping->getNumChildren();
			Symbol subtype = mapping->getNthChild(nkids-1)->getValue();

			std::wostringstream nameStr;
			for (int j = 0; j < nkids - 1; j++) {
				Symbol word = mapping->getNthChild(j)->getValue();
				nameStr << word.to_string();
				if (j < nkids - 2) nameStr << L" ";
			}
			Symbol nameSym(nameStr.str().c_str());
			EntitySubtype stype(subtype);

			EntitySubtypeItem *newItem = _new EntitySubtypeItem();
			newItem->entitySubtype = stype;
			newItem->next = NULL;

			if (_wikiWordMap->get(nameSym) != NULL) 
				newItem->next = *_wikiWordMap->get(nameSym);
				
			(*_wikiWordMap)[nameSym] = newItem;
		}
		delete wordMappings;
	}
}

SubtypeClassifier::~SubtypeClassifier() {
	deleteItems(_fullNameMap);
	delete _fullNameMap;
	deleteItems(_nameWordMap);
	delete _nameWordMap;
	deleteItems(_descHeadwordMap);
	delete _descHeadwordMap;
	deleteItems(_wordnetMap);
	delete _wordnetMap;
	deleteItems(_wikiWordMap);
	delete _wikiWordMap;
}

void SubtypeClassifier::deleteItems(WordToSubtypeMap *map)
{
	WordToSubtypeMap::iterator iter;
	for (iter = map->begin(); iter != map->end(); ++iter) {
		EntitySubtypeItem *item = (*iter).second;
		
		while (item != 0) {
			EntitySubtypeItem *next = item->next;
			delete item;
			item = next;
		}
	}
}

int SubtypeClassifier::classifyMention (MentionSet *currSolution, Mention *currMention, 
										MentionSet *results[], 
										int max_results, bool isBranching)
{
	// this is deterministic anyway, but whether we fork or not depends on if we're isBranching
	if (isBranching) {
		results[0] = _new MentionSet(*currSolution);
		currMention = results[0]->getMention(currMention->getIndex());
	}
	else
		results[0] = currSolution;

	// no subtypes for OTH, obviously
	if (!currMention->getEntityType().isRecognized())
		return 1;

	// don't reclassify if a subtype is already present
	if (currMention->getEntitySubtype().isDetermined())
		return 1;

	switch (currMention->mentionType) {
		case Mention::NONE:
			break;
		case Mention::DESC:
			classifyDesc(currMention);
			break;
		case Mention::NAME:
		case Mention::NEST:
			classifyName(currMention);
			break;
		case Mention::PART:
			classifyPart(currMention);
			break;
		case Mention::APPO:
			classifyAppo(currMention);
			break;
		case Mention::LIST:
			classifyList(currMention);
			break;
		default:
			break;
	}
	return 1;
}

EntitySubtype* SubtypeClassifier::lookupSubtype(WordToSubtypeMap *map, Symbol sym, EntityType type)
{							 

	EntitySubtypeItem **subtypeItem_p = map->get(sym);
	if (subtypeItem_p == 0) return 0;
	EntitySubtypeItem *subtypeItem = *subtypeItem_p;

	while (subtypeItem != 0 && 
		   subtypeItem->entitySubtype.getParentEntityType() != type)
	{
		subtypeItem = subtypeItem->next;
	}

	if (subtypeItem == 0) {
		return 0;
	}
	else {
		return &(subtypeItem->entitySubtype);
	}
}

void SubtypeClassifier::classifyName(Mention *currMention) {
	Mention *tempMention = currMention;
	while (tempMention->getChild() != 0)
		tempMention = tempMention->getChild();


	const SynNode *nameNode = tempMention->getNode();
	Symbol nameSym = SymbolUtilities::getFullNameSymbol(nameNode);

	// can we classify the full name
	EntitySubtype *subtype = lookupSubtype(_fullNameMap, nameSym, currMention->getEntityType());

	if (subtype != 0) {
		currMention->setEntitySubtype(*subtype);
		//std::cout << "\n" << subtype->getName().to_debug_string() << " (full name): ";
		//std::cout << nameSym.to_debug_string() << "\n";
		return;
	}


	//try stemming the full name
	Symbol stemmedname =  SymbolUtilities::getStemmedFullName(nameNode);
	//std::cout<<"Stem name: "
	//	<<nameSym.to_debug_string()<<" -> "<<stemmedname.to_debug_string()<<std::endl;

	subtype = lookupSubtype(_fullNameMap, stemmedname, currMention->getEntityType());

	if (subtype != 0)
	{
		currMention->setEntitySubtype(*subtype);
		//std::cout << "\n" << subtype->getName().to_debug_string() << " (stemmed name): ";
		//std::cout << nameSym.to_debug_string() << "\n";
		return;
	}

	// is there a word in the name that might give us a clue
	for (int i = 0; i < nameNode->getNChildren(); i++) {
		Symbol word = nameNode->getChild(i)->getHeadWord();
		EntitySubtype *subtype = lookupSubtype(_nameWordMap, word, currMention->getEntityType());

		if (subtype != 0)
		{
			currMention->setEntitySubtype(*subtype);
			//std::cout << "\n" << subtype->getName().to_debug_string() << " (name word): ";
			//std::cout << nameSym.to_debug_string() << "\n";
			return;
		}
		//Try looking on wiki lists too
		subtype = lookupSubtype(_wikiWordMap, word, currMention->getEntityType());
		if (subtype != 0)
		{
			currMention->setEntitySubtype(*subtype);
			//std::cout << "\n" << subtype->getName().to_debug_string() << " (wiki word): ";
			//std::cout << nameSym.to_debug_string() << "\n";
			return;
		}
	}
	//try stemmed variants of word

	for (int k = 0; k < nameNode->getNChildren(); k++) {
		Symbol word = nameNode->getChild(k)->getHeadWord();
		Symbol variants[15];
		int var = SymbolUtilities::getStemVariants(word, variants, 15);
		for(int j = 0; j< var; j++){
			subtype = lookupSubtype(_descHeadwordMap, variants[j], currMention->getEntityType());
			if(subtype != 0)
			{
				currMention->setEntitySubtype(*subtype);
				return;
			}
		}
	}

}

void SubtypeClassifier::classifyDesc(Mention *currMention) {
	Symbol headword = currMention->getNode()->getHeadWord();
	// do we know the actual headword
	EntitySubtype *subtype = lookupSubtype(_descHeadwordMap, headword, currMention->getEntityType());
	if (subtype == 0)
		subtype = 
			lookupSubtype(_descHeadwordMap, 
			              RelationUtilities::get()->stemPredicate(headword, Proposition::NOUN_PRED), 
						  currMention->getEntityType());
	if (subtype == 0){
		Symbol variants[15];
		int var = SymbolUtilities::getStemVariants(headword, variants, 15);
		for (int i = 0; i< var; i++){
			subtype = lookupSubtype(_descHeadwordMap, variants[i], currMention->getEntityType());
			if (subtype != 0)
			{
				currMention->setEntitySubtype(*subtype);
				return;
			}
		}
	}

	
	if (subtype != 0)
	{
		currMention->setEntitySubtype(*subtype);
		return;
	}

	//Warning this is Language and ACE2005 Specific
	if(currMention->getEntityType().matchesPER()){
		if(SymbolUtilities::isPluralMention(currMention)){
			try{
				EntitySubtype st =  EntitySubtype(Symbol(L"PER"), Symbol(L"Group"));
				currMention->setEntitySubtype(st);
			}
			catch(UnexpectedInputException){
				//probably not ACE2005, just ignore this
				;
			}
		}

		if(_posSequence != 0){
			int token = currMention->getNode()->getHeadPreterm()->getStartToken();
			if(token < _posSequence->getNTokens()){
				PartOfSpeech* tags = _posSequence->getPOS(token);
				if(LanguageSpecificFunctions::POSTheoryContainsNounPL(tags) &&
					!LanguageSpecificFunctions::POSTheoryContainsNounSG(tags)  &&
					!LanguageSpecificFunctions::POSTheoryContainsNounAmbiguousNumber(tags))
				{
					try{
						EntitySubtype st =  EntitySubtype(Symbol(L"PER"), Symbol(L"Group"));
						currMention->setEntitySubtype(st);
					}
					catch(UnexpectedInputException){
						//probably not ACE2005, just ignore this
						;
					}
				}
			}
		}
		

	}

	// speed test for Nominal Subtype Classificiation
	// trial-1 -- turned off WordNet

	
	Symbol results[100];
	int nhypernyms = SymbolUtilities::getHypernyms(headword, results, 100);

	// might we know a hypernym of the headword
	for (int i = 0; i < nhypernyms; i++) {
		//std::cout << "  " << results[i].to_debug_string() << "\n";
		EntitySubtype *subtype = lookupSubtype(_wordnetMap, results[i], currMention->getEntityType());
		if (subtype != 0)
		{
			currMention->setEntitySubtype(*subtype);
			//std::cout << "\n" << subtype->getName().to_debug_string() << " (wordnet): ";
			//std::cout << currMention->getNode()->toDebugTextString() << "\n";
			return;
		}
	}
	

	//look on wiki list
	subtype = lookupSubtype(_wikiWordMap, headword, currMention->getEntityType());
	if (subtype == 0)
		subtype = lookupSubtype(_wikiWordMap, RelationUtilities::get()->stemPredicate(headword, Proposition::NOUN_PRED), currMention->getEntityType());
	if(subtype == 0){
		Symbol variants[15];
		int var = SymbolUtilities::getStemVariants(headword, variants, 15);
		for(int i = 0; i< var; i++){
			subtype = lookupSubtype(_wikiWordMap, variants[i], currMention->getEntityType());
			if(subtype != 0)
			{
				currMention->setEntitySubtype(*subtype);
				return;
			}
		}
	}
	if (subtype != 0 && subtype->getParentEntityType() == currMention->getEntityType())
	{
		currMention->setEntitySubtype(*subtype);
		return;
	}
	
}

void  SubtypeClassifier::classifyList(Mention *currMention) {
	// ?? can't bring myself to care
}
void  SubtypeClassifier::classifyAppo(Mention *currMention) {
	if (currMention->getChild() != 0) {
		if (EntitySubtype::isValidEntitySubtype(currMention->getEntityType(),currMention->getChild()->getEntitySubtype())) {
			currMention->setEntitySubtype(currMention->getChild()->getEntitySubtype());
		}
	}
}
void  SubtypeClassifier::classifyPart(Mention *currMention) {
	if (currMention->getChild() != 0) {
		if (EntitySubtype::isValidEntitySubtype(currMention->getEntityType(),currMention->getChild()->getEntitySubtype())) {
			currMention->setEntitySubtype(currMention->getChild()->getEntitySubtype());
		}
	}
}
