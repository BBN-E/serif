// Copyright 2011 BBN Technologies
// All Rights Reserved

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/names/NameRecognizer.h"
#include "Generic/names/DefaultNameRecognizer.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/theories/DocTheory.h"

using namespace std;
class DefaultNameRecognizer;

Symbol DefaultNameRecognizer::_NONE_ST = Symbol(L"NONE-ST");
Symbol DefaultNameRecognizer::_NONE_CO = Symbol(L"NONE-CO");

DefaultNameRecognizer::DefaultNameRecognizer()
	: _debug_flag(false), _idfNameRecognizer(0), _pidfDecoder(0),
	  _tagSet(0), _num_names(0), _sent_no(0), _tokenSequence(0), _docTheory(0)
{
	/* 
		The xx_namefinder was originally set up to allow Serif to continue
		processing without any name model.  When using DefaultNameRecognizer, this 
		is still possible by having the "name_finder" parameter set to "none".

		It is also now possible to include a pidf name model with DefaultNameRecognizer.
		The IdFWordFeatures will still be language-independent, and therefore minimal.

		The methods here are copied from en_NameRecognizer.cpp with all(most) of the
		language-dependent bits removed.
	*/

	Symbol nameFinderParam = ParamReader::getParam(Symbol(L"name_finder"));
	if (nameFinderParam == Symbol(L"idf")) {
		_name_finder = IDF_NAME_FINDER;
	}
	else if (nameFinderParam == Symbol(L"pidf")) {
		_name_finder = PIDF_NAME_FINDER;
	}
	else if (nameFinderParam == Symbol(L"none")) {
		_name_finder = NONE;
	}
	else {
		throw UnexpectedInputException(
			"NameRecognizer::NameRecognizer()",
			"name_finder parameter must be set to 'idf', 'pidf' or 'none'");
	}
	if (_name_finder == IDF_NAME_FINDER) {
		// _idfNameRecognizer = _new IdFNameRecognizer();
		throw UnexpectedInputException(
			"DefaultNameRecognizer::DefaultNameRecognizer()",
			"name_finder parameter must be set to 'pidf' (not 'idf') to use DefaultNameRecognizer");
	}
	else if (_name_finder == PIDF_NAME_FINDER) {
		_pidfDecoder = _new PIdFModel(PIdFModel::DECODE);
		_tagSet = _pidfDecoder->getTagSet();
	}
	 

	if (ParamReader::isParamTrue("debug_name_recognizer")) {
		_debug_flag = true;
	}
	DISALLOW_EMAILS = true;

}


DefaultNameRecognizer::~DefaultNameRecognizer()
{
	if (_name_finder == PIDF_NAME_FINDER)
		delete _pidfDecoder;
}

void DefaultNameRecognizer::resetForNewSentence(const Sentence *sentence)
{
	if (_name_finder == PIDF_NAME_FINDER) {
		_force_lowercase_sentence = false;
		int sent_case = sentence->getSentenceCase();
		if (sent_case == Sentence::MIXED)
			_pidfDecoder->useDefaultDecoder();
		else {
			_pidfDecoder->useLowercaseDecoder();
			}
		/*if (sent_case == Sentence::UPPER) {
			_force_lowercase_sentence = true;
			}*/
	}
}

void DefaultNameRecognizer::resetForNewDocument(class DocTheory *docTheory)
{
	if (_name_finder == PIDF_NAME_FINDER) {
		_docTheory = docTheory;
		_pidfDecoder->resetForNewDocument(docTheory);
	}
}

void DefaultNameRecognizer::cleanUpAfterDocument()
{
	// copied from old name recognizer -- SRS

	// heap-allocated word lengths need cleanup.
	// everything else can persist, with just the counter reset
	int i;
	for (i=0; i < _num_names; i++)
		delete [] _nameWords[i].words;
#ifdef ENABLE_LEAK_DETECTION
	for (i=0; i < _num_names; i++)
		_names[i].swap(std::wstring()); // Acutally delete memory.
#endif
	_num_names = 0;

}

int DefaultNameRecognizer::getNameTheories(NameTheory **results, int max_theories, 
								TokenSequence *tokenSequence)
{
	if (_name_finder == PIDF_NAME_FINDER) {
		_tokenSequence = tokenSequence;
		_sent_no = tokenSequence->getSentenceNumber();
		// don't do name recognition on POSTDATE region (turned into a timex elsewhere)
		if (_docTheory != 0 && _docTheory->isPostdateSentence(_sent_no)) {
			results[0] = _new NameTheory(tokenSequence, 0);
			return 1;
		}
		// make the contents of SPEAKER and RECEIVER regions automatically into person names
		if (_docTheory != 0 && 
			(_docTheory->isSpeakerSentence(_sent_no) || _docTheory->isReceiverSentence(_sent_no))) 
		{
			bool found_prompt = false;
			for (int i = 0; i < tokenSequence->getNTokens(); i++) {
				if (tokenSequence->getToken(i)->getSymbol() == Symbol(L"prompt")) {
					found_prompt = true;
					break;
				}
			}
			if (!found_prompt) {
				results[0] = _new NameTheory(tokenSequence);
				results[0]->takeNameSpan(_new NameSpan(0, tokenSequence->getNTokens() - 1,
					EntityType::getPERType()));

				PIdFSentence sentence(_pidfDecoder->getTagSet(), *tokenSequence);
				return 1;
			}
		}

		PIdFSentence sentence(_pidfDecoder->getTagSet(), *tokenSequence, _force_lowercase_sentence);

		_pidfDecoder->decode(sentence);

		results[0] = makeNameTheory(sentence);
		postProcessNameTheory(results[0], tokenSequence);

		return 1;
	}
	else {
		// no model, return empty theory
		results[0] = _new NameTheory(tokenSequence, 0, 0);
		return 1;
	}
}

NameTheory *DefaultNameRecognizer::makeNameTheory(PIdFSentence &sentence) {
	int NONE_ST_tag = _tagSet->getTagIndex(_NONE_ST);

	int n_name_spans = 0;
	for (int j = 0; j < sentence.getLength(); j++) {
		if (sentence.getTag(j) != NONE_ST_tag &&
			_tagSet->isSTTag(sentence.getTag(j)))
		{
			// EMB 10/17: manually disallow email addresses as names
			if (DISALLOW_EMAILS &&
				(j +1 == sentence.getLength() || _tagSet->isSTTag(sentence.getTag(j+1))) &&
				isEmailorURL(sentence.getWord(j))) 
			{
				if (j == 0 || !_tagSet->isNoneTag(sentence.getTag(j-1)))
					sentence.setTag(j, NONE_ST_tag);
				else sentence.setTag(j, _tagSet->getTagIndex(_NONE_CO));
				continue;
			}
			n_name_spans++;
		}
	}

	NameTheory *nameTheory = _new NameTheory(_tokenSequence);
	
	int tok_index = 0;
	for (int i = 0; i < n_name_spans; i++) {
		while (!(sentence.getTag(tok_index) != NONE_ST_tag &&
				 _tagSet->isSTTag(sentence.getTag(tok_index))))
		{ tok_index++; }

		int tag = sentence.getTag(tok_index);

		int end_index = tok_index;
		while (end_index+1 < sentence.getLength() &&
			   _tagSet->isCOTag(sentence.getTag(end_index+1)))
		{ end_index++; }

		nameTheory->takeNameSpan(_new NameSpan(tok_index, end_index,
			EntityType(_tagSet->getReducedTagSymbol(tag))));

		//fixName(nameTheory->getNameSpan(i), &sentence);
		

		tok_index = end_index + 1;
	}

	return nameTheory;
}

bool DefaultNameRecognizer::isEmailorURL(Symbol sym) 
{
	// copied from en_NameRecognizer::isEmailAddress(Symbol sym)
	// changed to also include URLs
	std::wstring str = sym.to_string();

	size_t find = str.find(L".com");	
	if (find < str.length())
		return true;
	find = str.find(L".org");	
	if (find < str.length())
		return true;
	find = str.find(L".edu");	
	if (find < str.length())
		return true;
	find = str.find(L".gov");	
	if (find < str.length())
		return true;
	find = str.find(L"http");
	if (find < str.length())
		return true;
	find = str.find(L"www");
	if (find < str.length())
		return true;

	return false;



}
