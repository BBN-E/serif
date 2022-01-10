// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/GrowableArray.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/edt/StatDescLinker.h"
#include "Generic/edt/LexEntity.h"
#include "Generic/edt/DescLinkFunctions.h"
#include "Generic/theories/EntityType.h"
#include "Generic/maxent/OldMaxEntEvent.h"
#include "Generic/theories/SynNode.h"

#include <math.h>
#include <stdio.h>

#include "Generic/CASerif/correctanswers/CorrectMention.h"
#include <boost/scoped_ptr.hpp>

//DebugStream &StatDescLinker::_debugOut = DebugStream::referenceResolverStream;
const int StatDescLinker::MAX_PREDICATES = 2000;
DebugStream StatDescLinker::_debugStream;

StatDescLinker::StatDescLinker() : _primaryModel(0), _secondaryModel(0)
{
	_debugStream.init(Symbol(L"desclink_debug"));

	std::string modelname = ParamReader::getRequiredParam("desc_link_model_file");

	std::string buffer = modelname + ".maxent";
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& stream(*stream_scoped_ptr);
	stream.open(buffer.c_str());
	_primaryModel = _new OldMaxEntModel(stream);
	//_secondaryModel = _new OldMaxEntModel(stream);
	stream.close();

	_linkThreshhold = ParamReader::getRequiredFloatParam("desc_link_model_threshhold");
	// at what score should a link be acceptable? Conventional wisdom would say 50, but
	// the model is weak and probably wants overlinking, so it's a knob.
	if (_linkThreshhold < 0.0 || _linkThreshhold > 100.0)
		throw UnrecoverableException("StatDescLinker::StatDescLinker()",
			"Parameter 'desc_link_model_threshhold' must range from 0 to 100");

}

StatDescLinker::~StatDescLinker() {
	delete _primaryModel;
	delete _secondaryModel;
}

void StatDescLinker::resetForNewSentence() {
	_debugStream << L"*** NEW SENTENCE ***\n";
}

void StatDescLinker::resetForNewDocument(Symbol docName) {
	_debugStream << L"*** NEW DOCUMENT " << docName.to_string()  <<  " ***\n";
}

// set up to link. This method doesn't do any of the actual processing - it just provides a way
// for multiple paths to get through
int StatDescLinker::linkMention (LexEntitySet * currSolution, MentionUID currMentionUID,
								 EntityType linkType, LexEntitySet *results[], int max_results)
{
	LexEntitySet * newSet = NULL;
	EntityGuess *guesses [64];
	Mention *currMention = currSolution->getMention(currMentionUID);

	_debugStream << L"\nPROCESSING Mention #" << currMention->getUID() << L": ";
	_debugStream << currMention->getNode()->toTextString() << L"\n";

	int nGuesses = guessEntity(currSolution, currMention, linkType, guesses, max_results>64 ? 64: max_results);
	for (int i = 0; i < nGuesses; i++) {
		newSet = currSolution->fork();
		EntityGuess *thisGuess = guesses[i];
		if (thisGuess->id == EntityGuess::NEW_ENTITY) {
			newSet->addNew(currMention->getUID(), thisGuess->type);
			_debugStream << L"OUTCOME: CREATED NEW ENTITY #" << newSet->getNEntities() - 1 << "\n";
		}
		else {
			newSet->add(currMention->getUID(), thisGuess->id);
			_debugStream << L"OUTCOME: LINKED TO ENTITY #"  << thisGuess->id << "\n";
		}
		newSet->setScore(newSet->getScore() + static_cast<float>(thisGuess->score));
		results[i] = newSet;
		delete thisGuess;
		thisGuess = guesses[i] = NULL;
		newSet->customDebugPrint(_debugStream);
	}
	_debugStream << L"DONE PROCESSING MENTION #" << currMention->getUID() << "\n";

	return nGuesses;
}

// get probability for linking to each entity. If no entity has > 50% chance, put link to
// new entity at top of heap.
int StatDescLinker::guessEntity(LexEntitySet * currSolution, Mention * currMention,
								EntityType linkType, EntityGuess *results[], int max_results)
{
	_debugStream << L"BEGIN_GUESSES\n";
	// get a feature array for the proposed linking of the mention with each appropriate
	// entity. The feature array itself is language-specific. Use the array in the probability
	// model to get a score. use the score to order the results properly.
	const GrowableArray <Entity *> &filteredEnts = currSolution->getEntitiesByType(linkType);
	int nResults = 0;
	OldMaxEntEvent* evt = _new OldMaxEntEvent(MAX_PREDICATES);
	for (int i = 0; i < filteredEnts.length(); i++) {

		DescLinkFunctions::getEventContext(currSolution, currMention, filteredEnts[i], evt, MAX_PREDICATES);

		if (_debugStream.isActive()) {
			_debugStream << L"-------------------------------\n";
			_debugStream << L"Mention: " << currMention->getNode()->toTextString() << L"\n";
			_debugStream << L"CONSIDERING LINK TO Entity #" << filteredEnts[i]->getID() << L":\n";
			for (int m = 0; m < filteredEnts[i]->getNMentions(); m++)
				_debugStream << currSolution->getMention(filteredEnts[i]->getMention(m))->getNode()->toTextString() << L"\n";
			_debugStream << L"\n";
		}

		double linkprob, nolinkprob;
		//if (evt->find(DescLinkFunctions::HEAD_WORD_MATCH)) {
		//	_debugStream << "PRIMARY\n";
			// get score for link
			evt->setOutcome(DescLinkFunctions::OC_LINK);
			linkprob = _primaryModel->getScoreAndDebug(evt, _debugStream);
			// get score for no link
			evt->setOutcome(DescLinkFunctions::OC_NO_LINK);
			nolinkprob = _primaryModel->getScoreAndDebug(evt, _debugStream);
		/*}
		else {
			_debugStream << "SECONDARY\n";
			// get score for link
			evt->setOutcome(DescLinkFunctions::OC_LINK);
			linkprob = _secondaryModel->getScoreAndDebug(evt, _debugStream);
			// get score for no link
			evt->setOutcome(DescLinkFunctions::OC_NO_LINK);
			nolinkprob = _secondaryModel->getScoreAndDebug(evt, _debugStream);
		}*/

		// compute prob for linking
		double sum = exp(linkprob) + exp(nolinkprob);
		double prob = exp(linkprob) / sum;


		if (_debugStream.isActive()) {
			_debugStream << L"LINK PROB " << prob << L" := ";
			for (int n = 0; n < evt->getNContextPredicates(); n++)
				_debugStream << evt->getContextPredicate(n).to_string() << L" ";
			_debugStream << L"\n";
		}


		if (prob > _linkThreshhold) {
			EntityGuess* newGuess = _new EntityGuess();
			newGuess->id = filteredEnts[i]->getID();
			newGuess->type = linkType;
			newGuess->score = prob;
			// add results in sorted order
			bool insertedSolution = false;
			for (int p = 0; p < nResults; p++) {
				if(results[p]->score < newGuess->score) {
					if (nResults == max_results)
						delete results[max_results-1];
					else if (nResults < max_results)
						nResults++;
					for (int k = nResults-1; k > p; k--)
						results[k] = results[k-1];
					results[p] = newGuess;
					newGuess = NULL;
					insertedSolution = true;
					break;
				}
			}
			// solution doesn't make the cut. if there's room, insert it at the end
			// otherwise, ditch it
			if (!insertedSolution) {
				if (nResults < max_results)
					results[nResults++] = newGuess;
				else
					delete newGuess;
				newGuess = NULL;
			}
		}
		evt->reset();  // reset event for next link candidate
	}

	// add the new entity case
	if (nResults < max_results) {
		EntityGuess* newGuess = _new EntityGuess();
		newGuess->id = EntityGuess::NEW_ENTITY;
		newGuess->type = linkType;
		// not sure what the proper score here is
		// we'll simply make it slightly less than the next best score
		if (nResults > 0)
			newGuess->score = (results[nResults-1]->score - 0.01);
		else
			newGuess->score = 100;
		results[nResults++] = newGuess;

	}
	delete evt;
	return nResults;
}

void StatDescLinker::correctAnswersLinkMention(EntitySet *currSolution,
											   MentionUID currMentionUID,
											   EntityType linkType)
{
	EntityGuess *guesses [64];
	Mention *currMention = currSolution->getMention(currMentionUID);

	_debugStream << L"\nPROCESSING Mention #" << currMention->getUID() << L": ";
	_debugStream << currMention->getNode()->toTextString() << L"\n";

	int nGuesses = guessCorrectAnswerEntity(currSolution, currMention, linkType, guesses, 64);

	for (int i = 0; i < nGuesses; i++) {
		EntityGuess *thisGuess = guesses[i];
		if (i == 0) {
			if (thisGuess->id == EntityGuess::NEW_ENTITY)
				_debugStream << L"OUTCOME: CREATED NEW ENTITY #" << currSolution->getNEntities() << "\n";
			else
				_debugStream << L"OUTCOME: LINKED TO ENTITY #" << thisGuess->id << "\n";
		}
		delete thisGuess;
	}
	_debugStream << L"DONE PROCESSING MENTION #" << currMention->getUID() << "\n";

}

int StatDescLinker::guessCorrectAnswerEntity(EntitySet *currSolution, Mention *currMention,
											 EntityType linkType, EntityGuess *results[], int max_results)
{
	_debugStream << L"BEGIN_GUESSES\n";
	// get a feature array for the proposed linking of the mention with each appropriate
	// entity. The feature array itself is language-specific. Use the array in the probability
	// model to get a score. use the score to order the results properly.
	const GrowableArray <Entity *> &filteredEnts = currSolution->getEntitiesByType(linkType);
	int nResults = 0;
	OldMaxEntEvent* evt = _new OldMaxEntEvent(MAX_PREDICATES);
	for (int i = 0; i < filteredEnts.length(); i++) {

		DescLinkFunctions::getEventContext(currSolution, currMention, filteredEnts[i], evt, MAX_PREDICATES);

		if (_debugStream.isActive()) {
			_debugStream << L"-------------------------------\n";
			_debugStream << L"Mention: " << currMention->getNode()->toTextString() << L"\n";
			_debugStream << L"CONSIDERING LINK TO Entity #" << filteredEnts[i]->getID() << L":\n";
			for (int m = 0; m < filteredEnts[i]->getNMentions(); m++)
				_debugStream << currSolution->getMention(filteredEnts[i]->getMention(m))->getNode()->toTextString() << L"\n";
			_debugStream << L"\n";
		}

		double linkprob, nolinkprob;
		//if (evt->find(DescLinkFunctions::HEAD_WORD_MATCH)) {
		//	_debugStream << "PRIMARY\n";
			// get score for link
			evt->setOutcome(DescLinkFunctions::OC_LINK);
			linkprob = _primaryModel->getScoreAndDebug(evt, _debugStream);
			// get score for no link
			evt->setOutcome(DescLinkFunctions::OC_NO_LINK);
			nolinkprob = _primaryModel->getScoreAndDebug(evt, _debugStream);
		/*}
		else {
			_debugStream << "SECONDARY\n";
			// get score for link
			evt->setOutcome(DescLinkFunctions::OC_LINK);
			linkprob = _secondaryModel->getScoreAndDebug(evt, _debugStream);
			// get score for no link
			evt->setOutcome(DescLinkFunctions::OC_NO_LINK);
			nolinkprob = _secondaryModel->getScoreAndDebug(evt, _debugStream);
		}*/

		// compute prob for linking
		double sum = exp(linkprob) + exp(nolinkprob);
		double prob = exp(linkprob) / sum;

		if (_debugStream.isActive()) {
			_debugStream << L"LINK PROB " << prob << L" := ";
			for (int n = 0; n < evt->getNContextPredicates(); n++)
				_debugStream << evt->getContextPredicate(n).to_string() << L" ";
			_debugStream << L"\n";
		}

		if (prob > _linkThreshhold) {
			EntityGuess* newGuess = _new EntityGuess();
			newGuess->id = filteredEnts[i]->getID();
			newGuess->type = linkType;
			newGuess->score = prob;
			//if (evt->find(DescLinkFunctions::HEAD_WORD_MATCH))
			//	newGuess->score += 100;
			// add results in sorted order
			bool insertedSolution = false;
			for (int p = 0; p < nResults; p++) {
				if(results[p]->score < newGuess->score) {
					if (nResults == max_results)
						delete results[max_results-1];
					else if (nResults < max_results)
						nResults++;
					for (int k = nResults-1; k > p; k--)
						results[k] = results[k-1];
					results[p] = newGuess;
					newGuess = NULL;
					insertedSolution = true;
					break;
				}
			}
			// solution doesn't make the cut. if there's room, insert it at the end
			// otherwise, ditch it
			if (!insertedSolution) {
				if (nResults < max_results)
					results[nResults++] = newGuess;
				else
					delete newGuess;
				newGuess = NULL;
			}
		}
		evt->reset();  // reset event for next link candidate
	}

	// add the new entity case
	if (nResults < max_results) {
		EntityGuess* newGuess = _new EntityGuess();
		newGuess->id = EntityGuess::NEW_ENTITY;
		newGuess->type = linkType;
		// not sure what the proper score here is
		// we'll simply make it slightly less than the next best score
		if (nResults > 0)
			newGuess->score = (results[nResults-1]->score - 0.01);
		else
			newGuess->score = 100;
		results[nResults++] = newGuess;

	}
	delete evt;
	return nResults;
}

void StatDescLinker::printCorrectAnswer(int id) {
	if (id == -1)
		_debugStream << "CORRECT: CREATED NEW ENTITY\n\n";
	else
		_debugStream << "CORRECT: LINKED TO ENTITY #" << id << "\n\n";
}

