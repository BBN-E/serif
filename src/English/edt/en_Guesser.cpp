// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/SymbolConstants.h"
#include "Generic/common/hash_set.h"
#include "Generic/common/ParamReader.h"
#include "English/common/en_WordConstants.h"
#include "English/common/en_NationalityRecognizer.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/edt/SimpleQueue.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/NodeInfo.h"
#include "English/edt/en_Guesser.h"
#include "English/parse/en_STags.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/DebugStream.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/edt/CorefUtilities.h"
#include <boost/scoped_ptr.hpp>

DebugStream & EnglishGuesser::_debugOut = DebugStream::referenceResolverStream;

Symbol::HashSet *EnglishGuesser::_femaleNames;
Symbol::HashSet *EnglishGuesser::_femaleDescriptors;
Symbol::HashSet *EnglishGuesser::_maleNames;
Symbol::HashSet *EnglishGuesser::_maleDescriptors;
Symbol::HashSet *EnglishGuesser::_pluralDescriptors;
Symbol::HashSet *EnglishGuesser::_pluralNouns;
Symbol::HashSet *EnglishGuesser::_singularNouns;
SymbolArraySet *EnglishGuesser::_xdocMaleNames;
SymbolArraySet *EnglishGuesser::_xdocFemaleNames;

bool EnglishGuesser::_initialized = false;

bool EnglishGuesser::guessUntyped;
bool EnglishGuesser::guessXDocNames;

const int MAX_MENTION_NAME_SYMS_PLUS = 20;

void EnglishGuesser::initialize() {

	// if we're already initialized, just return
	if (_initialized) return;

    boost::scoped_ptr<UTF8InputStream> femaleNamesFile_scoped_ptr(UTF8InputStream::build());
    UTF8InputStream& femaleNamesFile(*femaleNamesFile_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> maleNamesFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& maleNamesFile(*maleNamesFile_scoped_ptr); 
	boost::scoped_ptr<UTF8InputStream> femaleDescsFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& femaleDescsFile(*femaleDescsFile_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> maleDescsFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& maleDescsFile(*maleDescsFile_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> pluralDescsFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& pluralDescsFile(*pluralDescsFile_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> pluralNounsFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& pluralNounsFile(*pluralNounsFile_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> singularNounsFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& singularNounsFile(*singularNounsFile_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> xdocMaleNamesFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& xdocMaleNamesFile(*xdocMaleNamesFile_scoped_ptr);
	boost::scoped_ptr<UTF8InputStream> xdocFemaleNamesFile_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& xdocFemaleNamesFile(*xdocFemaleNamesFile_scoped_ptr);

	guessUntyped = ParamReader::isParamTrue("utcoref_enable");

	guessXDocNames = ParamReader::isParamTrue("xdoc_pronoun_coref_enable");

	std::string filename = ParamReader::getRequiredParam("guesser_female_names");
	femaleNamesFile.open(filename.c_str());

	filename = ParamReader::getRequiredParam("guesser_male_names");
	maleNamesFile.open(filename.c_str());

	filename = ParamReader::getRequiredParam("guesser_female_descriptors");
	femaleDescsFile.open(filename.c_str());

	filename = ParamReader::getRequiredParam("guesser_male_descriptors");
	maleDescsFile.open(filename.c_str());

	filename = ParamReader::getRequiredParam("guesser_plural_descriptors");
	pluralDescsFile.open(filename.c_str());


	_femaleNames = _new Symbol::HashSet(1024);
	_maleNames = _new Symbol::HashSet(1024);
	_femaleDescriptors = _new Symbol::HashSet(1024);
	_maleDescriptors = _new Symbol::HashSet(1024);
	_pluralDescriptors = _new Symbol::HashSet(1024);

	if(guessUntyped) {
		filename = ParamReader::getRequiredParam("guesser_plural_nouns");
		pluralNounsFile.open(filename.c_str());

		filename = ParamReader::getRequiredParam("guesser_singular_nouns");
		singularNounsFile.open(filename.c_str());

		/* these are initialized to a few thousand more than the current
		sizes of the lists we're putting in them */
		_pluralNouns = _new Symbol::HashSet(250000);
		_singularNouns = _new Symbol::HashSet(80000);
	}
	else {
		_pluralNouns = 0;
		_singularNouns = 0;
	}

	if (guessXDocNames) 
	{
		filename = ParamReader::getRequiredParam("guesser_xdoc_male_names");
		xdocMaleNamesFile.open(filename.c_str());

		filename = ParamReader::getRequiredParam("guesser_xdoc_female_names");
		xdocFemaleNamesFile.open(filename.c_str());

	   	_xdocMaleNames = _new SymbolArraySet(250000);
		_xdocFemaleNames = _new SymbolArraySet(250000);
	} 
	else 
	{
		_xdocMaleNames = 0;
		_xdocFemaleNames = 0;
	}

	loadNames(femaleNamesFile, maleNamesFile, femaleDescsFile, maleDescsFile, pluralDescsFile,
		pluralNounsFile, singularNounsFile, xdocMaleNamesFile, xdocFemaleNamesFile);

	femaleNamesFile.close();
	maleNamesFile.close();
	femaleDescsFile.close();
	maleDescsFile.close();
	pluralDescsFile.close();

	if (guessUntyped) {
	   pluralNounsFile.close();
	   singularNounsFile.close();
	}

	if (guessXDocNames) {
		xdocMaleNamesFile.close();
		xdocFemaleNamesFile.close();
	}

	_initialized = true;
}

void EnglishGuesser::loadNames(UTF8InputStream &femaleNamesFile, UTF8InputStream &maleNamesFile,
						UTF8InputStream &femaleDescsFile, UTF8InputStream &maleDescsFile,
						UTF8InputStream &pluralDescsFile, UTF8InputStream &pluralNounsFile,
						UTF8InputStream &singularNounsFile, UTF8InputStream &xdocMaleNamesFile,
						UTF8InputStream &xdocFemaleNamesFile)
{
	Symbol name;
	wchar_t buffer[256];
	while(!femaleNamesFile.eof()) {
		femaleNamesFile.getLine(buffer, 256);
		_femaleNames->insert(Symbol(buffer));
	}
	while(!maleNamesFile.eof()) {
		maleNamesFile.getLine(buffer, 256);
		_maleNames->insert(Symbol(buffer));
	}
	while(!femaleDescsFile.eof()) {
		femaleDescsFile.getLine(buffer, 256);
		_femaleDescriptors->insert(Symbol(buffer));
	}
	while(!maleDescsFile.eof()) {
		maleDescsFile.getLine(buffer, 256);
		_maleDescriptors->insert(Symbol(buffer));
	}
	while(!pluralDescsFile.eof()) {
		pluralDescsFile.getLine(buffer, 256);
		_pluralDescriptors->insert(Symbol(buffer));
	}

	if(guessUntyped) {
	   while(!pluralNounsFile.eof()) {
		  pluralNounsFile.getLine(buffer, 256);
		  _pluralNouns->insert(Symbol(buffer));
	   }
	   while(!singularNounsFile.eof()) {
		  singularNounsFile.getLine(buffer, 256);
		  _singularNouns->insert(Symbol(buffer));
	   }
	}
	if (guessXDocNames) {
		loadMultiwordNames(xdocMaleNamesFile, _xdocMaleNames);
		loadMultiwordNames(xdocFemaleNamesFile, _xdocFemaleNames);
	}
}

void EnglishGuesser::loadMultiwordNames(UTF8InputStream& uis, SymbolArraySet * names_set) {
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

void EnglishGuesser::destroy() {

	// if not initialized yet, just return
	if (!_initialized) return;

	delete _femaleNames;
	delete _maleNames;
	delete _femaleDescriptors;
	delete _maleDescriptors;
	delete _pluralDescriptors;
	if (_xdocMaleNames != 0) {
		delete _xdocMaleNames;
	}
	if (_xdocFemaleNames != 0) {
		delete _xdocFemaleNames;
	}
	if (_singularNouns != 0) {
	   delete _singularNouns;
	}
	if (_pluralNouns != 0) {
	   delete _pluralNouns;
	}

	_initialized = false;
}

Symbol EnglishGuesser::guessGender(const EntitySet *entitySet, const Entity* entity) {
	for (int i = 0; i < entity->getNMentions(); i++) {
		const Mention *ment = entitySet->getMention(entity->getMention(i));
		Symbol gender = guessGender(ment->getNode(), ment);
		if (gender != Guesser::UNKNOWN)
			return gender;
	}
	return Guesser::UNKNOWN;
}

Symbol EnglishGuesser::guessNumber(const EntitySet *entitySet, const Entity* entity) {
	for (int i = 0; i < entity->getNMentions(); i++) {
		const Mention *ment = entitySet->getMention(entity->getMention(i));
		Symbol number = guessNumber(ment->getNode(), ment);
		if (number != Guesser::UNKNOWN)
			return number;
	}
	return Guesser::UNKNOWN;
}

 
Symbol EnglishGuesser::guessGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames) {
	if (mention == NULL || mention->mentionType == Mention::LIST)
		return Guesser::UNKNOWN;

	if (mention->mentionType == Mention::PRON) {
		Symbol word = mention->getHead()->getSingleWord();
		if(word == EnglishWordConstants::HE ||
			word == EnglishWordConstants::HIS ||
			word == EnglishWordConstants::HIM)
			return Guesser::MASCULINE;
		else if(word == EnglishWordConstants::SHE ||
			word == EnglishWordConstants::HER)
			return Guesser::FEMININE;
		else if(word == EnglishWordConstants::IT ||
			word == EnglishWordConstants::ITS)
			return Guesser::NEUTRAL;
		else if(word == EnglishWordConstants::THEY ||
			word == EnglishWordConstants::THEM ||
			word == EnglishWordConstants::THEIR)
			return Guesser::UNKNOWN;
		else
			return Guesser::UNKNOWN;
	}

	else if (!NodeInfo::isOfNPKind(node))
		return Guesser::UNKNOWN;
	else if (mention->getEntityType().getName() == Symbol(L"POG"))
		return Guesser::UNKNOWN;
	else if (mention->getEntityType().matchesLOC() ||
			 mention->getEntityType().matchesORG())
		return Guesser::NEUTRAL;
	else if (mention->getEntityType().matchesPER() && mention->mentionType != Mention::PRON)
		return guessPersonGender(node, mention, suspectedSurnames);
	else return Guesser::UNKNOWN;
}

Symbol EnglishGuesser::guessPersonGender(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames) {

	// LB 8/22/03: why would we assume this is a name?
	//   it could be an appositive or a descriptor, so let's do this all more sensibly...

    if (mention->getMentionType() == Mention::NAME) {
		if (guessXDocNames) {
			Symbol gender = guessPersonGenderForXDocName(node, mention);
			if (gender != Guesser::UNKNOWN)
				return gender;
		}
		return guessPersonGenderForName(node, mention, suspectedSurnames);

	} else if (mention->getMentionType() == Mention::DESC) {

		// like "wife" or "widow"
		bool isFemaleName = (_femaleDescriptors->find(node->getHeadWord())!=_femaleDescriptors->end());
		bool isMaleName = (_maleDescriptors->find(node->getHeadWord())!=_maleDescriptors->end());
		if(isFemaleName && !isMaleName)
			return Guesser::FEMININE;
		if(isMaleName && !isFemaleName)
			return Guesser::MASCULINE;

	} else if (mention->getMentionType() == Mention::APPO) {
		const Mention *child = mention->getChild();
		while (child != 0) {
			Symbol result = guessPersonGender(child->getNode(), child, suspectedSurnames);
			if (result != Guesser::UNKNOWN)
				return result;
			child = child->getNext();
		}
	}

	return Guesser::UNKNOWN;
}

Symbol EnglishGuesser::guessPersonGenderForXDocName(const SynNode *node, const Mention *mention) {
	SymbolArray* ment_sa = CorefUtilities::getNormalizedSymbolArray(mention);
	bool isMaleName = _xdocMaleNames->exists(ment_sa);
	bool isFemaleName = _xdocFemaleNames->exists(ment_sa);
	delete ment_sa;
	if(isFemaleName && !isMaleName) {
		return Guesser::FEMININE;
	}
	if(isMaleName && !isFemaleName) {
		return Guesser::MASCULINE;
	}
	return Guesser::UNKNOWN;
}

Symbol EnglishGuesser::guessPersonGenderForName(const SynNode *node, const Mention *mention, std::set<Symbol> suspectedSurnames = std::set<Symbol>()) {

	static bool init = false;
	static bool use_old_buggy_code;
	if (!init) {
		use_old_buggy_code = true;
		if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_fixed_gender_guesser", false))
			use_old_buggy_code = false;
	}		

	bool matches_suspected_surname = false;
	const SynNode *nameNode = mention->getEDTHead();
	if (nameNode->getNTerminals() == 1) {
		Symbol singleNameWord = nameNode->getNthTerminal(0)->getHeadWord();
		if (suspectedSurnames.find(singleNameWord) != suspectedSurnames.end())
			matches_suspected_surname = true;
	}

	if (use_old_buggy_code) {

		SimpleQueue <const SynNode *> q;
		int nChildren = node->getNChildren();
		int i;
		for(i=0; i<nChildren; i++)
			//add to queue for breadth first search
			q.add(node->getChild(i));

		//now inspect children for male or female names

		// if more than one child, we leave off the last child,
		//    under the assumption that this is a name
		if (nChildren > 1) nChildren--;
		for (i = 0; i<nChildren; i++) {
			if (matches_suspected_surname)
				break;
			const SynNode *child = node->getChild(i);
			bool isFemaleName = (_femaleNames->find(child->getHeadWord())!=_femaleNames->end());
			bool isMaleName = (_maleNames->find(child->getHeadWord())!=_maleNames->end());
			if(isFemaleName && !isMaleName) {
				return Guesser::FEMININE;
			}
			if(isMaleName && !isFemaleName)
				return Guesser::MASCULINE;
		}

		//now perform breadth first search
		while(!q.isEmpty()) {
			const SynNode *thisNode = q.remove();
			if(thisNode->getTag() == EnglishSTags::NPPOS)
				continue;
			Symbol word = thisNode->getHeadWord();
			if(word == EnglishWordConstants::MR ||
				word == EnglishWordConstants::MR_L
				)
				return Guesser::MASCULINE;
			else if(word == EnglishWordConstants::MRS ||
				word == EnglishWordConstants::MIS_L ||
				word == EnglishWordConstants::MS ||
				word == EnglishWordConstants::MS_L ||
				word == EnglishWordConstants::MISS ||
				word == EnglishWordConstants::MISS_L
				)
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

	} else {

		if (mention->getMentionType() != Mention::NAME &&
		    mention->getMentionType() != Mention::NEST)
			return Guesser::UNKNOWN;

		const SynNode *atomicHead = node->getHeadPreterm()->getParent();

		// Inspect atomic head for male or female names	
		// When there is a single token, we do let this run, e.g. "Scott". This will pose
		//   some problems when the person's name is "Karen Scott", for example.	
		// When there is more than one token, we only look at the first N-1 tokens, so as
		//   to avoid looking at last names. It's possible even that this is a bad idea, if
		//   we want "Karen Scott" to link to "Scott", but...
		int nChildren = atomicHead->getNChildren();
		if (nChildren > 1)
			nChildren--;
		for (int i = 0; i < nChildren; i++) {
			if (matches_suspected_surname)
				break;
			Symbol headword = atomicHead->getChild(i)->getHeadWord();
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
				if (word == EnglishWordConstants::MR || word == EnglishWordConstants::MR_L) {		
					return Guesser::MASCULINE;
				}
				else if (word == EnglishWordConstants::MRS ||
					word == EnglishWordConstants::MIS_L ||
					word == EnglishWordConstants::MS ||
					word == EnglishWordConstants::MS_L ||
					word == EnglishWordConstants::MISS ||
					word == EnglishWordConstants::MISS_L) 
				{
					return Guesser::FEMININE;
				}
			}
		}
		return Guesser::UNKNOWN;
	}
}


Symbol EnglishGuesser::guessType(const SynNode *node, const Mention *mention) {
	if (mention == NULL)
		return EntityType::getUndetType().getName();
	else if (mention->getEntityType().isRecognized())
		return mention->getEntityType().getName();
	else if(mention->mentionType == Mention::PRON) {
		Symbol word = mention->getHead()->getSingleWord();
		if (word == EnglishWordConstants::HE ||
			word == EnglishWordConstants::HIS ||
			word == EnglishWordConstants::HIM ||
			word == EnglishWordConstants::SHE ||
			word == EnglishWordConstants::HER)
		{
			return EntityType::getPERType().getName();
		}
	}
	return EntityType::getUndetType().getName();
}

Symbol EnglishGuesser::guessNumber(const SynNode *node, const Mention *mention) {
	/** PLEASE NOTE: this function is now called in eeml/en_GroupFnEnglishGuesser
	 *		If you make any changes to it, particularly any
	 *      that would make it depend on initialization of the EnglishGuesser,
	 *      please make sure en_GroupFnEnglishGuesser doesn't break.
	 */

    if (mention == NULL || mention->getHead() == NULL)
	   return Guesser::UNKNOWN;

	if (EnglishNationalityRecognizer::isRegionWord(node->getHeadWord()))
		return Guesser::UNKNOWN;

	if (mention->getEntityType() == EntityType::getGPEType() &&
		(mention->getMentionType() == Mention::NAME ||
		 mention->getMentionType() == Mention::NEST))
	{
		return Guesser::SINGULAR;
	}


	const SynNode *headPreterm = mention->getHead()->getHeadPreterm();
	Symbol tag = headPreterm->getTag();

	bool isPluralDescriptor = false;
	if (mention->getMentionType() == Mention::DESC) {
		isPluralDescriptor = _pluralDescriptors->find(node->getHeadWord()) != _pluralDescriptors->end();

	}

	Symbol word = mention->getHead()->getHeadWord();

	if(mention->mentionType == Mention::PRON) {

		if(word == EnglishWordConstants::HE ||
			word == EnglishWordConstants::HIS ||
			word == EnglishWordConstants::HIM ||
			word == EnglishWordConstants::SHE ||
			word == EnglishWordConstants::HER ||
			word == EnglishWordConstants::IT  ||
			word == EnglishWordConstants::ITS)
			return Guesser::SINGULAR;
		else if(word == EnglishWordConstants::THEY ||
			word == EnglishWordConstants::THEM ||
			word == EnglishWordConstants::THEIR)
			return Guesser::PLURAL;
		else {
			_debugOut << "WARNING: Unforeseen pronoun: " << word.to_debug_string() << "\n";
			return Guesser::UNKNOWN;
		}
	}
	else if(!NodeInfo::isOfNPKind(node)) {
		_debugOut << "WARNING: Node not of NP type\n";
		return Guesser::UNKNOWN;
	}

	else if(isPluralDescriptor || tag == EnglishSTags::NNPS || tag == EnglishSTags::NNS ||
		    tag == EnglishSTags::JJS || mention->mentionType == Mention::LIST ||
			(tag == EnglishSTags::CD && word != EnglishWordConstants::ONE && word != Symbol(L"1")))

	{
		return Guesser::PLURAL;
	}
	else if ((tag == EnglishSTags::NNP || tag == EnglishSTags::NN) && mention->mentionType != Mention::LIST)
		return Guesser::SINGULAR;



	if (guessUntyped) {
	   /* first see if we're a conjunction; all conjunctions are plural */
	   const SynNode *n = mention->getHead();
	   while (!n->isTerminal()) {
		  for (int i = 0 ; i < n->getNChildren() ; i++) {
			 if (n->getChild(i)->getTag() == EnglishSTags::CC) {
				return Guesser::PLURAL;
			 }
		  }
		  n = n->getHead();
	   }

	   /* numbers */
	   if (tag == EnglishSTags::CD) {
		  if (word == Symbol(L"1") || word == Symbol(L"one")) {
			 return Guesser::SINGULAR;
		  }
		  return Guesser::PLURAL;
	   }

	   /* determiners */
	   if (tag == EnglishSTags::DT) {
		  if (word == Symbol(L"these") || word == Symbol(L"those") || word == Symbol(L"all") ||
			  word == Symbol(L"both") || word == Symbol(L"niether") || word == Symbol(L"some") ||
			  word == Symbol(L"half") || word == Symbol(L"no"))
		  {
			 return Guesser::PLURAL;
		  }
		  return Guesser::SINGULAR;
	   }

	   /* use lists of nouns of known number */
	   bool isPluralNoun = (_pluralNouns->find(word) != _pluralNouns->end());
	   bool isSingularNoun = (_singularNouns->find(word) != _singularNouns->end());
	   if (isSingularNoun) {
		  return Guesser::SINGULAR;
	   }
	   if (isPluralNoun) {
		  return Guesser::PLURAL;
	   }

	   /* apply the wug test */
	   std::wstring str_word = word.to_string();
	   if (((str_word.size() > 1 && 
			 str_word[str_word.size()-1] == L's') /* ends with s */ || 

			(str_word.size() > 3 && 
			 str_word[str_word.size()-3] == L'm' &&
			 str_word[str_word.size()-2] == L'e' &&
			 str_word[str_word.size()-1] == L'n')) /* ends with men */ &&

		   !(str_word.size() > 4 &&
			 str_word[str_word.size()-4] == L'e' &&
			 str_word[str_word.size()-3] == L's' &&
			 str_word[str_word.size()-2] == L'e' &&
			 str_word[str_word.size()-1] == L's')) /* ends with eses */
	   {
		  return Guesser::PLURAL;
	   }
	   return Guesser::SINGULAR;
	}

	return Guesser::UNKNOWN;
}
