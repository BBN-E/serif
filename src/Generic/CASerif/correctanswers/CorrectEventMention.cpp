// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#include "common/leak_detection.h"

#include "Generic/CASerif/correctanswers/CorrectEventMention.h"
#include "Generic/CASerif/correctanswers/CorrectEvent.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "common/UnexpectedInputException.h"
#include "common/InternalInconsistencyException.h"
#include "common/ParamReader.h"
#include "theories/MentionSet.h"
#include "theories/ValueMentionSet.h"

CorrectEventMention::CorrectEventMention() : _n_args(0), 
	_sentence_number(-1), _args(0)
{ }

CorrectEventMention::~CorrectEventMention() { 
	delete [] _args;
}

void CorrectEventMention::loadFromSexp(Sexp* mentionSexp) 
{
	size_t num_children = mentionSexp->getNumChildren();
	if (num_children < 5) 
		throw UnexpectedInputException("CorrectEventMention::loadFromSexp()",
									   "event mention sexp must have at least 5 children");

	Sexp* idSexp = mentionSexp->getNthChild(0);
	Sexp* anchorStartSexp = mentionSexp->getNthChild(1);
	Sexp* anchorEndSexp = mentionSexp->getNthChild(2);
	Sexp* extentStartSexp = mentionSexp->getNthChild(3);
	Sexp* extentEndSexp = mentionSexp->getNthChild(4);

	if (!idSexp->isAtom() || !anchorStartSexp->isAtom() || 
		!anchorEndSexp->isAtom() || !extentStartSexp->isAtom() || 
		!extentEndSexp->isAtom())
			throw UnexpectedInputException("CorrectEventMention::loadFromSexp()",
									       "Didn't find event mention atoms in correctAnswerSexp");

	_extent_start = EDTOffset(_wtoi(extentStartSexp->getValue().to_string()));
	_extent_end = EDTOffset(_wtoi(extentEndSexp->getValue().to_string()));
	_anchor_start = EDTOffset(_wtoi(anchorStartSexp->getValue().to_string()));
	_anchor_end = EDTOffset(_wtoi(anchorEndSexp->getValue().to_string()));
	_annotationID = idSexp->getValue();

	_n_args = static_cast<int>(num_children) - 5;
	int i;
	
	if (_n_args > 0) 
		_args = _new CorrectEventArgument[_n_args];

	for (i = 0; i < _n_args; i++ ) {
		Sexp* argSexp = mentionSexp->getNthChild(i + 5);
		if (argSexp->getNumChildren() != 2)
			throw UnexpectedInputException("CorrectEventMention::loadFromSexp()",
									   "event argument sexp must have exactly 2 children");
		_args[i].annotationID = argSexp->getNthChild(0)->getValue();
		_args[i].role = argSexp->getNthChild(1)->getValue();
		_args[i].mention = 0;
		_args[i].valueMention = 0;
	}

}

/*
void CorrectEventMention::loadFromAdept(vector<Argument> eventArgsAdept) 
{
	
    _n_args=eventArgsAdept.size()-1;

	if (_n_args > 0) 
		_args = _new CorrectEventArgument[_n_args];

    for(int i=0;i<eventArgsAdept.size();i++){
	string type=eventArgsAdept[i].argumentType.type;
	 int arg_counter=0;
	   if(type.compare("deft://event.ere#trigger")==0){   //this is the trigger
			std::map<ChunkUnion, double> pair= eventArgsAdept[i].argumentDistribution;
		 
			for (std::map<ChunkUnion,double>::iterator it=pair.begin(); it!=pair.end(); ++it){
                int begin=it->first.chunk.tokenOffset.beginIndex;
			           int end=it->first.chunk.tokenOffset.endIndex;
						Token tokenBegin = it->first.chunk.tokenStream.tokenList[begin];
						_anchor_start = EDTOffset(tokenBegin.charOffset.beginIndex);
       
						Token tokenEnd = it->first.chunk.tokenStream.tokenList[end];
						_anchor_end = EDTOffset(tokenEnd.charOffset.endIndex);
			}

	_extent_start = EDTOffset(_wtoi(extentStartSexp->getValue().to_string()));  //what are extent?
	_extent_end = EDTOffset(_wtoi(extentEndSexp->getValue().to_string()));
	
	_annotationID = idSexp->getValue(); //what should it be?
	  }
	  else{   //regular arguments
	  
	  _args[arg_counter].annotationID = //what is annotationID for an argument?
		_args[arg_counter].role = Symbol(string_to_wstring(type));
		_args[arg_counter].mention = 0;
		_args[arg_counter].valueMention = 0;
		arg_counter+=1;
	  }
	}

	
}
*/

Symbol CorrectEventMention::getType() { 
	return _event->getEventType(); 
}

void CorrectEventMention::setMentionArgs(MentionSet *mentionSet, CorrectDocument *correctDocument, const std::set<Symbol> &eventTypesAllowUndet) {

	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *mention = mentionSet->getMention(i);
		if (mention->getEntityType().isRecognized() ||
		   (mention->getEntityType()==EntityType::getUndetType() && eventTypesAllowUndet.find(getType())!=eventTypesAllowUndet.end()) ) {
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

			for (int j = 0; j < _n_args; j++) {
				if (_args[j].annotationID == correctMention->getAnnotationID()) {
					_args[j].mention = mention;
					_args[j].valueMention = 0;
				}
			}
		}
	}
	
}

void CorrectEventMention::setValueArgs(ValueMentionSet *valueMentionSet, 
									   CorrectDocument *correctDocument) 
{

	for (int i = 0; i < valueMentionSet->getNValueMentions(); i++) {
		const ValueMention *valueMention = valueMentionSet->getValueMention(i);
		CorrectValue *correctValue 
			= correctDocument->getCorrectValueFromValueMentionID(valueMention->getUID());
		if (correctValue == 0)
			continue;

		for (int j = 0; j < _n_args; j++) {
			if (_args[j].annotationID == correctValue->getAnnotationValueMentionID()) {
				_args[j].valueMention = valueMention;
				_args[j].mention = 0;
			}
		}
	}
}


bool CorrectEventMention::unifyAppositives() {
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
				"CorrectEventMention::unifyAppositives()",
				"Parameter 'unify_appositives' must be 'true' or 'false'");
		}
	}
	return unify_appositives;
}
