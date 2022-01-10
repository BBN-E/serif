// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/WordConstants.h"
#include "common/ParamReader.h"
#include "common/UnexpectedInputException.h"
#include "theories/DocTheory.h"
#include "discTagger/DTTagSet.h"
#include "names/discmodel/PIdFModel.h"
#include "names/KoreanNameRecognizer.h"


using namespace std;

Symbol KoreanNameRecognizer::_NONE_ST = Symbol(L"NONE-ST");
Symbol KoreanNameRecognizer::_NONE_CO = Symbol(L"NONE-CO");

KoreanNameRecognizer::KoreanNameRecognizer() : 
	 _pidfDecoder(0), _docTheory(0)
{
	_pidfDecoder = _new PIdFModel(PIdFModel::DECODE);
	_tagSet = _pidfDecoder->getTagSet();
}

KoreanNameRecognizer::~KoreanNameRecognizer() {
	delete _pidfDecoder;
}

void KoreanNameRecognizer::resetForNewDocument(DocTheory *docTheory) {
	_docTheory = docTheory;
	_pidfDecoder->resetForNewDocument(docTheory);
}

int KoreanNameRecognizer::getNameTheories(NameTheory **results, int max_theories, 
									TokenSequence *tokenSequence)
{
	_sent_no = tokenSequence->getSentenceNumber();

	// don't do name recognition on POSTDATE region (turned into a timex elsewhere)
	if (_docTheory->isPostdateSentence(_sent_no)) {
		results[0] = _new NameTheory();
		results[0]->n_name_spans = 0;
		return 1;
	}

	// make the contents of SPEAKER and POSTER regions automatically into person names
	if (_docTheory->isSpeakerSentence(_sent_no)) {

		// although not if there is a comma involved (e.g. "BOB JONES, REPORTER FOR CNN")
		bool found_comma = false;
		for (int i = 0; i < tokenSequence->getNTokens(); i++) {
			if (tokenSequence->getToken(i)->getSymbol() == WordConstants::ASCII_COMMA ||
				tokenSequence->getToken(i)->getSymbol() == WordConstants::FULL_COMMA) 
			{
				found_comma = true;
				break;
			}
		}

		if (!found_comma) {

			results[0] = _new NameTheory();
			results[0]->n_name_spans = 1;
			results[0]->nameSpans = _new NameSpan*[1];
			results[0]->nameSpans[0] = _new NameSpan(0, tokenSequence->getNTokens() - 1,
				EntityType::getPERType());

			return 1;

		}
	}

	PIdFSentence sentence(_pidfDecoder->getTagSet(), *tokenSequence);
	_pidfDecoder->decode(sentence);

	results[0] = makeNameTheory(sentence);

	return 1;
}

NameTheory *KoreanNameRecognizer::makeNameTheory(PIdFSentence &sentence) {
	int NONE_ST_tag = _tagSet->getTagIndex(_NONE_ST);

	int n_name_spans = 0;
	for (int j = 0; j < sentence.getLength(); j++) {
		if (sentence.getTag(j) != NONE_ST_tag &&
			_tagSet->isSTTag(sentence.getTag(j)))
		{
			n_name_spans++;
		}
	}

	NameTheory *nameTheory = _new NameTheory();
	nameTheory->n_name_spans = n_name_spans;
	nameTheory->nameSpans = _new NameSpan*[n_name_spans];
	
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

		nameTheory->nameSpans[i] = _new NameSpan(tok_index, end_index,
			EntityType(_tagSet->getReducedTagSymbol(tag)));

		fixName(nameTheory->nameSpans[i], &sentence);
		
		tok_index = end_index + 1;
	}

	return nameTheory;
}

void KoreanNameRecognizer::fixName(NameSpan* span, PIdFSentence* sent) {

/*	Copied from en_NameRecognizer
	if (_sent_no == 0 && span->start == 0 && !span->type.matchesGPE() &&
		sent->getLength() > span->start + 1 &&
		sent->getWord(span->start + 1) == WordConstants::_COMMA_)
	{
		span->type = EntityType::getGPEType();
		return;
	}


	// first create the symbol of the entire name
	Symbol nameSym = getNameSymbol(span->start, span->end, sent);
	int span_length = (span->end+1 - span->start);
	EntityType type;
	int i;
	bool foundMatch = false;
	bool exactMatch = false;
	// exact match phase
	for (i=0; i < _num_names; i++) {
		if (_names[i] == nameSym) {
			type = _types[i];
			foundMatch = true;
			exactMatch = true;
			break;
		}
	}
	// inexact match phase
	if (!foundMatch) {
		for (i=0; i < _num_names; i++) {
			// name must be longer to be inexact
			if (_nameWords[i].num_words <= span_length)
				continue;

			// GPE/LOC/NATIONALITY matching to non-GPE/LOC/NATIONALITY can 
			// only match to PER, and can only match as a terminating substring
			if ((span->type.matchesGPE() || span->type.matchesLOC()
					|| span->type.isNationality()) &&
				(!_types[i].matchesGPE() && !_types[i].matchesLOC()
					&& !_types[i].isNationality()))
			{
				if (!_types[i].matchesPER())
					continue;
				int start_idx = _nameWords[i].num_words-span_length;
				Symbol subName = getNameSymbol(start_idx,
								_nameWords[i].num_words-1, _nameWords[i]);
				if (subName == nameSym) {
					type = _types[i];
					foundMatch = true;
					break;
				}
			}
			// for all others, we must check all substring-by-word combinations
			else {
				// consider each span of the proper length
				int j;
				for (j = 0; j <= _nameWords[i].num_words-span_length; j++) {
					Symbol subName = getNameSymbol(j, j+(span_length-1),
												   _nameWords[i]);
					if (subName == nameSym) {
						type = _types[i];
						foundMatch = true;
						break;
					}
				}
				if (foundMatch)
					break;
			}
		}
	}

	// matched the name - in some way
	if (foundMatch) {
		// fix name so it's consistent
		if (type != span->type) {
			if (_debug_flag) {
				SessionLogger &logger = *SessionLogger::logger;
				logger.beginMessage();
				logger << L"Changing " << nameSym.to_string() << L" from ";
				logger << span->type.getName().to_string() << L" to "; 
				logger << type.getName().to_string() << L"\n";							
			}
			span->type = type;
		}
	}
	// didn't match perfectly, so we have a new name
	if (!exactMatch) {
		// can't save any more names, so warn, but continue
		if (_num_names >= MAX_DOC_NAMES) {
			SessionLogger &logger = *SessionLogger::logger;
			logger.beginWarning();
			logger << "Document exceeds name limit of "
				   << MAX_DOC_NAMES << "\n";
		}
		else {
			_names[_num_names] = nameSym;
			_types[_num_names] = span->type;
			_nameWords[_num_names].num_words = span_length;
			_nameWords[_num_names].words = _new Symbol[span_length];
			int index = 0;
			for (i=span->start; i <= span->end; i++)
				_nameWords[_num_names].words[index++] = sent->getWord(i);
			_num_names++;
		}
	}
	return;
*/
}
