// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/DebugStream.h"
#include "Spanish/common/es_WordConstants.h"
#include "Generic/common/hash_set.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/edt/SimpleQueue.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/SynNode.h"
#include "Spanish/edt/es_Guesser.h"
#include "Spanish/parse/es_STags.h"

#include <boost/scoped_ptr.hpp>

const int MAX_MENTION_NAME_SYMS_PLUS = 20;

DebugStream & SpanishGuesser::_debugOut = DebugStream::referenceResolverStream;

Symbol::HashSet *SpanishGuesser::_femaleNames;
Symbol::HashSet *SpanishGuesser::_maleNames;
SymbolArraySet *SpanishGuesser::_gpeNames;

bool SpanishGuesser::_initialized = false;

void SpanishGuesser::initialize() {
	if (_initialized) return;

    boost::scoped_ptr<UTF8InputStream> femaleNamesFile_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& femaleNamesFile(*femaleNamesFile_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> maleNamesFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& maleNamesFile(*maleNamesFile_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> gpeNamesFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& gpeNamesFile(*gpeNamesFile_scoped_ptr);

	std::string filename = ParamReader::getRequiredParam("guesser_female_names");
	femaleNamesFile.open(filename.c_str());
	filename = ParamReader::getRequiredParam("guesser_male_names");
	maleNamesFile.open(filename.c_str());
	filename = ParamReader::getRequiredParam("guesser_country_names");
	gpeNamesFile.open(filename.c_str());

	_femaleNames = _new Symbol::HashSet(1024);
	_maleNames = _new Symbol::HashSet(1024);
	_gpeNames = _new SymbolArraySet(300);

	loadNames(femaleNamesFile, _femaleNames);
	loadNames(maleNamesFile, _maleNames);
	femaleNamesFile.close();
	maleNamesFile.close();
	loadMultiwordNames(gpeNamesFile, _gpeNames);
	gpeNamesFile.close();
	_initialized = true;
}
void SpanishGuesser::loadNames(UTF8InputStream &names_file, Symbol::HashSet * names_set)
{
	Symbol name;
	wchar_t buffer[256];
	while(!names_file.eof()) {
		names_file.getLine(buffer, 256);
		names_set->insert(Symbol(buffer));
	}
}
void SpanishGuesser::loadMultiwordNames(UTF8InputStream& uis, SymbolArraySet * names_set) {
	UTF8Token tok;
	int lineno = 0;
	Symbol toksyms[MAX_MENTION_NAME_SYMS_PLUS];
	while(!uis.eof()) {
		uis >> tok;	// this should be the opening '('
		if(uis.eof()){
			break;
		}
		uis >> tok;	
		int ntoks = 0;
		int over_toks = 0;
		while(tok.symValue() != Symbol(L")")){
			if (ntoks < MAX_MENTION_NAME_SYMS_PLUS) {
				toksyms[ntoks++] = tok.symValue();
			} else {
				over_toks++;
			}
			uis >> tok;
		}
		lineno++;
		if (over_toks > 0) {
			continue; // ignore names that are too long
		}
		SymbolArray * sa = _new SymbolArray(toksyms, ntoks);
		names_set->insert(sa);
	}// end of read loop
}

void SpanishGuesser::destroy() {
	// if not initialized yet, just return
	if (!_initialized) return;

	delete _femaleNames;
	delete _maleNames;
	delete _gpeNames;
	_initialized = false;
}

Symbol SpanishGuesser::guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames) {
	if(mention == NULL || mention->mentionType == Mention::LIST)
		return Guesser::UNKNOWN;
	else if(! NodeInfo::isOfNPKind(node))
		return Guesser::UNKNOWN;
	else if(mention->getEntityType().matchesORG())
		return Guesser::NEUTRAL;
	else if(mention->getEntityType().matchesGPE())
		return guessGpeGenderForName(node, mention);
	else if(mention->mentionType == Mention::PRON) {
		Symbol word = mention->getHead()->getSingleWord();
		if (SpanishWordConstants::isMasculinePronoun(word))
			return Guesser::MASCULINE;
		else if (SpanishWordConstants::isFemininePronoun(word))
			return Guesser::FEMININE;
		else if (SpanishWordConstants::isNeuterPronoun(word))
			return Guesser::NEUTER;
		else
			return Guesser::UNKNOWN;
	}
	else if(mention->getEntityType().matchesPER())
		return guessPersonGender(node, mention);
	else return Guesser::UNKNOWN;
}

Symbol SpanishGuesser::guessPersonGender(const SynNode *node, const Mention *mention) {
    if (mention->getMentionType() == Mention::NAME)
		return guessPersonGenderForName(node, mention);
	SimpleQueue <const SynNode *> q;
	int nChildren = node->getNChildren();
	int i;
	for (i = 0; i < nChildren; i++) 
		//add kids to queue for breadth first search
		q.add(node->getChild(i));

	//now perform breadth first search
	while (!q.isEmpty()) {
		const SynNode *thisNode = q.remove();
		//if(thisNode->getTag() == SpanishSTags::NPPOS)
		//	continue;
		Symbol word = thisNode->getHeadWord();
		if (SpanishWordConstants::isMasculineTitle(word))
			return Guesser::MASCULINE;
		else if(SpanishWordConstants::isFeminineTitle(word))
			return Guesser::FEMININE;
		//visit all children
		if(!thisNode->isPreterminal()) {
			nChildren = thisNode->getNChildren();
			for(i=0; i<nChildren; i++)
				q.add(thisNode->getChild(i));
		}
	}//end while
	//if nothing specific found, return unknown  
	return Guesser::UNKNOWN;
}
Symbol SpanishGuesser::guessPersonGenderForName(const SynNode *node, const Mention *mention) {
			const SynNode *atomicHead = node->getHeadPreterm()->getParent();

	//From English person gender guesser new version of 2003
	// Inspect atomic head for male or female names	
	// When there is a single token, we do let this run, e.g. "Scott". This will pose
	//   some problems when the person's name is "Karen Scott", for example.	
	// When there is more than one token, we only look at the first non-title token, so as
	//   to avoid looking at second-part-of-first-names and family names. It's possible even 
	//   that this is a bad idea, if we want "Karen Scott" to link to "Scott", but...
	// we skip non-first names because "Jose Maria" is a guy and "Maria Jesus" is a gal
		int nChildren = atomicHead->getNChildren();
		if (nChildren > 0) {
			Symbol headword = atomicHead->getChild(0)->getHeadWord();
			bool isFemaleName = (_femaleNames->find(headword)!=_femaleNames->end());
			bool isMaleName = (_maleNames->find(headword)!=_maleNames->end());
			if (isFemaleName && !isMaleName)
				return Guesser::FEMININE;
			if (isMaleName && !isFemaleName)
				return Guesser::MASCULINE;
		}

		const SynNode *atomicHeadParent = atomicHead->getParent();
		if (atomicHeadParent != 0) {
			for (int i = 0; i < atomicHeadParent->getNChildren(); i++) {
				if (atomicHeadParent->getChild(i) == atomicHead)
					break;
				Symbol word = atomicHeadParent->getChild(i)->getHeadWord();
				if (SpanishWordConstants::isMasculineTitle(word)) {		
					return Guesser::MASCULINE;
				}
				if (SpanishWordConstants::isFeminineTitle(word)) {	
					return Guesser::FEMININE;
				}
			}
		}
		return Guesser::UNKNOWN;
}
Symbol SpanishGuesser::guessGpeGenderForName(const SynNode *node, const Mention *mention) {
    if (mention->getMentionType() != Mention::NAME)
		return Guesser::UNKNOWN;
	SymbolArray* ment_sa = CorefUtilities::getNormalizedSymbolArray(mention);
	bool is_gpe_name = _gpeNames->exists(ment_sa);
	size_t sa_len = ment_sa->getSizeTLength();
	const Symbol * ment_sa_sar = ment_sa->getArray();
	const Symbol last_gpe_symbol = ment_sa_sar[sa_len-1];
	std::wstring last_gpe_word = last_gpe_symbol.to_string();
	wchar_t last_gpe_letter = last_gpe_word.at(last_gpe_word.length()-1);
	delete ment_sa;
	if (is_gpe_name){ // "country" names are maculine UNLESS they end in unaccented 'a'
		if (last_gpe_letter == L'a') 
			return Guesser::FEMININE;
		else 
			return Guesser::MASCULINE;
	}
	// not a known country name so guess based on ending
	else if (last_gpe_letter == L'a') 
			return Guesser::FEMININE;
	else if (last_gpe_letter == L'o') 
			return Guesser::MASCULINE;

	return Guesser::UNKNOWN;

}
Symbol SpanishGuesser::guessType(const SynNode *node, const Mention *mention) {
	if (mention == NULL)
		return EntityType::getUndetType().getName();
	else if (mention->getEntityType().isRecognized())
		return mention->getEntityType().getName();
	else if (mention->mentionType == Mention::PRON) {
		Symbol word = mention->getHead()->getSingleWord();
		if (SpanishWordConstants::isPERTypePronoun(word))
			return EntityType::getPERType().getName();
		if (SpanishWordConstants::isLOCTypePronoun(word))
			return EntityType::getLOCType().getName();
		if (SpanishWordConstants::isNeuterPronoun(word))
			return EntityType::getABSType().getName();
	}
	return EntityType::getUndetType().getName();
}

Symbol SpanishGuesser::guessNumber(const SynNode *node, const Mention *mention) {
	if(mention == NULL || mention->getHead() == NULL)
		return Guesser::UNKNOWN;
	
	Symbol tag = mention->getHead()->getHeadPreterm()->getTag();

	if(mention->mentionType == Mention::PRON) {
		Symbol word = mention->getHead()->getHeadWord();
		if (SpanishWordConstants::isSingularPronoun(word))
			return Guesser::SINGULAR;
		if (SpanishWordConstants::isPluralPronoun(word))
			return Guesser::PLURAL;
		return Guesser::UNKNOWN; 
	}
	else if(!NodeInfo::isOfNPKind(node)) {
		_debugOut << "WARNING: Node not of NP type\n";
		return Guesser::UNKNOWN;
	}
	else if(mention->mentionType == Mention::LIST)
		return Guesser::PLURAL;
	else return Guesser::UNKNOWN;
}

Symbol SpanishGuesser::guessGender(const EntitySet *entitySet, const Entity* entity) {
	for (int i = 0; i < entity->getNMentions(); i++) {
		const Mention *ment = entitySet->getMention(entity->getMention(i));
		Symbol gender = guessGender(ment->getNode(), ment);
		if (gender != Guesser::UNKNOWN)
			return gender;
	}
	return Guesser::UNKNOWN; 
}

Symbol SpanishGuesser::guessNumber(const EntitySet *entitySet, const Entity* entity) { 
		return Guesser::UNKNOWN; }

