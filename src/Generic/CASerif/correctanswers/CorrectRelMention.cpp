// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/CASerif/correctanswers/CorrectRelMention.h"
#include "Generic/CASerif/correctanswers/CorrectRelation.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "Generic/CASerif/correctanswers/CorrectValue.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "common/UnexpectedInputException.h"
#include "common/InternalInconsistencyException.h"
#include "common/ParamReader.h"
#include "theories/MentionSet.h"

CorrectRelMention::CorrectRelMention(): _lhs(0), _rhs(0), _timeRole(L"NULL"), _timeArg(0) { 
	_sentence_number = -1;
}

CorrectRelMention::~CorrectRelMention() { }

void CorrectRelMention::loadFromSexp(Sexp* mentionSexp) 
{
	size_t num_children = mentionSexp->getNumChildren();
	if (num_children != 5 && num_children != 3) {
		std::stringstream ss;
		ss << "relMentionSexp must have exactly 3 or 5 children " << 
			mentionSexp->to_debug_string();

		throw UnexpectedInputException("CorrectRelMention::loadFromSexp()",
									   ss.str().c_str());
	}

	Sexp* lhsSexp = mentionSexp->getNthChild(0);
	Sexp* rhsSexp = mentionSexp->getNthChild(1);
	Sexp* idSexp = 0;
	if (num_children == 5) {
		Sexp* timeRoleSexp = mentionSexp->getNthChild(2);
		Sexp* timeSexp = mentionSexp->getNthChild(3);
		idSexp = mentionSexp->getNthChild(4);
		if (!timeRoleSexp->isAtom() || !timeSexp->isAtom()) {
			throw UnexpectedInputException("CorrectRelMention::loadFromSexp()",
				"Didn't find relation mention atoms in correctAnswerSexp");
		}		
		_timeRole = timeRoleSexp->getValue();
		_timeAnnotationID = timeSexp->getValue();
	} else {
		idSexp = mentionSexp->getNthChild(2);
		_timeRole = Symbol(L"NULL");
		_timeAnnotationID = Symbol(L"NULL");
	}

	if (!lhsSexp->isAtom() || !rhsSexp->isAtom() || !idSexp->isAtom()) {
			throw UnexpectedInputException("CorrectRelMention::loadFromSexp()",
									       "Didn't find relation mention atoms in correctAnswerSexp");
	}

	_leftAnnotationID = lhsSexp->getValue();
	_rightAnnotationID = rhsSexp->getValue();
	_annotationID = idSexp->getValue();	
}

/*void CorrectRelMention::loadFromAdept(vector<Argument> relationArgsAdept) 
{

        _timeRole = Symbol(L"NULL");
		_timeAnnotationID = Symbol(L"NULL");
	

	
	_leftAnnotationID = Symbol(string_to_wstring(relationArgsAdept[0].id.id));
	_rightAnnotationID =  Symbol(string_to_wstring(relationArgsAdept[1].id.id));
	_annotationID = Symbol(L"1");	
}*/

Symbol CorrectRelMention::getType() { 
	return _relation->getRelationType(); 
}

bool CorrectRelMention::setMentionArgs(MentionSet *mentionSet, CorrectDocument *correctDocument) {
	_lhs = _rhs = 0;

	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *mention = mentionSet->getMention(i);
		if (mention->getEntityType().isRecognized()) {
			CorrectMention *correctMention 
				= correctDocument->getCorrectMentionFromMentionID(mention->getUID());

			// we used to throw an exception here
			if (correctMention == NULL)
				continue;

			// Check to make sure it's the top mention with this mention id. If not,
			// don't process it. **This shouldn't be an issue with the new CA system,
			// except when we are unifying appositives, since the only mentions we give
			// correct answers to are NAME, DESC, PART, PRON, which by definition I believe
			// do not have parents in the same entity as themselves.**
			if (mention->getNode()->getParent() != 0 &&
				mention->getNode()->getParent()->hasMention())
			{	
				Mention * parentMent 
					= mentionSet->getMentionByNode(mention->getNode()->getParent());
				CorrectMention *parentCM 
					= correctDocument->getCorrectMentionFromMentionID(parentMent->getUID());

				if (parentCM != 0) {
					// if we're unifying appositives, we need to skip this (in favor of 
					// the parent mention that has the same entity)
					if (unifyAppositives()) {
						CorrectEntity *parentCE = correctDocument->getCorrectEntityFromCorrectMention(parentCM);
						CorrectEntity *correctEntity = correctDocument->getCorrectEntityFromCorrectMention(correctMention);
						if (correctEntity == parentCE)
							continue;
					} else {
						// if we're not unifying appositives, we only want to promote to something,
						// other than an appositive, that has the same MENTION id.
						if (parentMent->getMentionType() != Mention::APPO &&
							parentCM->getAnnotationID() == correctMention->getAnnotationID())
							continue;
					}
				}
			}

			if (_leftAnnotationID == correctMention->getAnnotationID()) 
				_lhs = mention;
			if (_rightAnnotationID == correctMention->getAnnotationID())
				_rhs = mention;
		}
	}
	
	if (_lhs && _rhs) {
		_sentence_number = mentionSet->getSentenceNumber();
		return true;
	}

	return false;
}

void CorrectRelMention::setTimeArg(ValueMentionSet *valueMentionSet, 
								   CorrectDocument *correctDocument) 
{
	_timeArg = 0;

	if (_timeAnnotationID == Symbol(L"NULL"))
		return;

	for (int i = 0; i < valueMentionSet->getNValueMentions(); i++) {
		ValueMention *valueMention = valueMentionSet->getValueMention(i);
		if (valueMention->isTimexValue()) {
			CorrectValue *correctValue 
				= correctDocument->getCorrectValueFromValueMentionID(valueMention->getUID());

			// we used to throw an exception here
			if (correctValue== NULL)
				continue;

			if (_timeAnnotationID == correctValue->getAnnotationValueMentionID()) 
				_timeArg = valueMention;
		}
	}

}

bool CorrectRelMention::unifyAppositives() {
	static bool init = false;
	static bool unify_appositives;
	if (!init) {
		init = true;
		Symbol unifyAppositives = ParamReader::getParam(L"unify_appositives");
		if (unifyAppositives == Symbol(L"true")) {
			unify_appositives = true;
		}
		else if (unifyAppositives == Symbol(L"false")) {
			unify_appositives = false;
		}
		else {
			throw UnexpectedInputException(
				"CorrectRelMention::unifyAppositives()",
				"Parameter 'unify_appositives' must be 'true' or 'false'");
		}
	}
	return unify_appositives;
}
