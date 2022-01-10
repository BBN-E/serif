// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/GrowableArray.h"

#include "Generic/edt/OutsideDescLinker.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Sexp.h"
#include "Generic/edt/LexEntity.h"
#include "Generic/edt/DescLinkFunctions.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/DebugStream.h"

#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include <boost/scoped_ptr.hpp>
DebugStream OutsideDescLinker::_debug;

OutsideDescLinker::OutsideDescLinker() 
{
	_debug.init(Symbol(L"desclink_debug"));
	_outside_coref_directory = ParamReader::getRequiredParam("outside_coref_directory");
}

OutsideDescLinker::~OutsideDescLinker() {}

void OutsideDescLinker::resetForNewSentence() {
	_debug << L"*** NEW SENTENCE ***\n";
}

void OutsideDescLinker::resetForNewDocument(Symbol docName) {
	_debug << L"*** NEW DOCUMENT " << docName.to_string()  <<  " ***\n";

	std::stringstream outside_coref_file;
	outside_coref_file << _outside_coref_directory << "/" << docName.to_debug_string() << ".entities";

	boost::scoped_ptr<UTF8InputStream> outsideCoref_scoped_ptr(UTF8InputStream::build(outside_coref_file.str().c_str()));
	UTF8InputStream& outsideCoref(*outsideCoref_scoped_ptr);
	_outsideSexp = _new Sexp(outsideCoref);
	// check well-formedness
	for (int s = 0; s < _outsideSexp->getNumChildren(); s++) {
		Sexp *entitySexp = _outsideSexp->getNthChild(s);
		if (!entitySexp->isList() || entitySexp->getNumChildren() < 3 ||
			!entitySexp->getFirstChild()->isAtom() ||
			!entitySexp->getSecondChild()->isAtom())
		{
			char message[2000];
			_snprintf(message, 2000, "Ill-formed entity sexp (%s) in file %s",
				entitySexp->to_debug_string().c_str(), outside_coref_file.str().c_str());
			throw UnexpectedInputException("OutsideDescLinker::resetForNewDocument", message);
		}
	}
	outsideCoref.close();
}

int OutsideDescLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID,
								 EntityType linkType, LexEntitySet *results[], int max_results)
{
	Mention *currMention = currSolution->getMention(currMentionUID);
	EntityGuess* guess = _findOutsideMatch(currMention, currSolution, linkType);

	if (guess == 0)
		guess = _guessNewEntity(currMention, linkType);

	_debug << "Linking:\n" << currMention->node->toDebugString(0).c_str() << "\n";
	_debug << " to:\n**********************\n";

	// only one branch here - desc linking is deterministic
	LexEntitySet* newSet = currSolution->fork();
	if (guess->id == EntityGuess::NEW_ENTITY) {
		newSet->addNew(currMention->getUID(), guess->type);
		_debug << "Itself: entity with id " << guess->id << " and of type " << guess->type.getName().to_debug_string() << "\n";
	}
	else {
		// debug: what are we linking?
		if (_debug.isActive()) {
			int i;
			for (i=0; i < currSolution->getNEntities(); i++) {
				Entity* ent = currSolution->getEntity(i);
				if (ent->getID() == guess->id) {
					GrowableArray<MentionUID> &ments = ent->mentions;
					int j;
					for (j=0; j < ments.length(); j++) {
						Mention *thisMention = currSolution->getMention(ments[j]);
						_debug << thisMention->node->toDebugString(0).c_str() << "\n";
					}
					break;
				}
			}
		}
		newSet->add(currMention->getUID(), guess->id);
	}
	_debug << "**********************\n";

	results[0] = newSet;

	// MEMORY: a guess was definitely created above, and is removed now.
	delete guess;
	return 1;

}

EntityGuess* OutsideDescLinker::_findOutsideMatch(Mention* ment, LexEntitySet *ents, EntityType linkType)
{

	Sexp *entitySexpForThisMention = 0;
	for (int s = 0; s < _outsideSexp->getNumChildren(); s++) {
		Sexp *entitySexp = _outsideSexp->getNthChild(s);
		for (int m = 2; m < entitySexp->getNumChildren(); m++) {
			Symbol mentIDSym = entitySexp->getNthChild(m)->getValue();
			MentionUID ment_id(atoi(mentIDSym.to_debug_string()));
			if (ment_id == ment->getUID()) {
				entitySexpForThisMention = entitySexp;
				break;
			}
		}       
		if (entitySexpForThisMention) break;
	}

	int i;
	for (i = ents->getNMentionSets()-1; i >=0; i--) {
		const MentionSet* prevMents = ents->getMentionSet(i);
		int j;

		for (j=prevMents->getNMentions()-1; j >= 0; j--) {
			Mention* prevMent = prevMents->getMention(j);
			if (prevMent->mentionType != Mention::NAME &&
				prevMent->mentionType != Mention::DESC) 
				continue;

			Entity* ent = ents->getEntityByMention(prevMent->getUID(), linkType);
			if (ent == 0) // SRS -- this results from pre-linked mentions
				continue; // JCS -- or from mentions with the wrong entity type

			for (int ment = 2; ment < entitySexpForThisMention->getNumChildren(); ment++) {
				Symbol mentIDSym = entitySexpForThisMention->getNthChild(ment)->getValue();
				MentionUID ment_id(atoi(mentIDSym.to_debug_string()));
				if (ment_id == prevMent->getUID()) {
					EntityGuess* guess = _new EntityGuess();
					guess->id = ent->ID;
					guess->score = 1;
					guess->type = ent->getType();
					return guess;
				}
			}
		}
	}

	return 0;
}


EntityGuess* OutsideDescLinker::_guessNewEntity(Mention *ment, EntityType linkType)
{
	// MEMORY: linkMention, the caller of this method, is guaranteed to delete this
	EntityGuess* guess = _new EntityGuess();
	guess->id = EntityGuess::NEW_ENTITY;
	guess->score = 1;
	guess->type = linkType;
	return guess;
}
