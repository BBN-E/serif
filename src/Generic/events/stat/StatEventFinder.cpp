// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/events/EventUtilities.h"
#include "Generic/events/stat/StatEventFinder.h"
#include "Generic/events/stat/PotentialEventMention.h"
#include "Generic/events/stat/EventTriggerFinder.h"
#include "Generic/events/stat/EventArgumentFinder.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/DocTheory.h"

// added for Modality Classification
#include "Generic/events/stat/EventModalityClassifier.h"

#define MAX_POTENTIALS 100

StatEventFinder::StatEventFinder() : _potentials(0), _triggerFinder(0), _argumentFinder(0)
{
	_potentials = _new EventMention *[MAX_POTENTIALS];

	_triggerFinder = _new EventTriggerFinder(EventTriggerFinder::DECODE);
	_argumentFinder = _new EventArgumentFinder(EventArgumentFinder::DECODE);

#ifdef AssignMODALITY
	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("use_two_layer_modality_model", false)){
		_modalityClassifier = _new EventModalityClassifier(EventModalityClassifier::DECODE_TWO_LAYER);
	}else{
		_modalityClassifier = _new EventModalityClassifier(EventModalityClassifier::DECODE);
	}
#endif

}

StatEventFinder::~StatEventFinder() {
	delete [] _potentials;
	delete _triggerFinder;
	delete _argumentFinder;
}

void StatEventFinder::resetForNewSentence(DocTheory *docTheory, int sentence_num)
{
	_sentence_num = sentence_num;

}

void StatEventFinder::setDocumentTopic(Symbol topic) { 
	_triggerFinder->setDocumentTopic(topic); 
}

const DTTagSet* StatEventFinder::getEventTypeTagSet() {
	return _triggerFinder->getTagSet();
}

const DTTagSet* StatEventFinder::getArgumentTypeTagSet() {
	return _argumentFinder->getTagSet();
}

EventMentionSet *StatEventFinder::getEventTheory(const TokenSequence *tokens,
												 ValueMentionSet *valueMentionSet,
		Parse *parse,
		MentionSet *mentionSet,
		PropositionSet *propSet)
{
	// identify triggers
	int n_potentials = _triggerFinder->processSentence(tokens, parse, mentionSet, propSet, 
		_potentials, MAX_POTENTIALS);

	// attach arguments
	for (int i = 0; i < n_potentials; i++) {
		_argumentFinder->attachArguments(_potentials[i], tokens, valueMentionSet, 
			parse, mentionSet, propSet);

#ifdef AssignMODALITY
		// experiment I -- using a statistical MODALITY classifier
		_modalityClassifier->setModality(tokens, parse, valueMentionSet, mentionSet, 
								propSet, _potentials[i]);
#endif

	}

	


	// assign attributes -- but for now this lowers scores, so let's not
	// uncommented this part for ICEWS-related experiments 
	/*
	std::vector<bool> isNonAssertedProp = EventUtilities::identifyNonAssertedProps(propSet, mentionSet);
	for (int i = 0; i < n_potentials; i++) {
		const Proposition *prop = _potentials[i]->getAnchorProp();
		if (prop != 0 && isNonAssertedProp[prop->getIndex()]) {
			const SynNode *root = _potentials[i]->getAnchorNode();
			while (root->getParent() != 0)
				root = root->getParent();
			std::cerr << root->toDebugTextString() << "\n";
			std::cerr << "Modality == other: " << _potentials[i]->toDebugString();
			for (int j = 0; j < propSet->getNPropositions(); j++) {
				const Proposition *tempProp = propSet->getProposition(j);
				std::cerr << tempProp->toDebugString() << " ";
				if (isNonAssertedProp[prop->getIndex()])
					std::cerr << "NON-ASSERTED";
				std::cerr << "\n";
			}						
			_potentials[i]->setModality(Event::MOD_OTHER);
		}
	}*/

	// transfer ownership to EventMentionSet
	EventMentionSet *result = _new EventMentionSet(parse);
	for (int j = 0; j < n_potentials; j++) {
		result->takeEventMention(_potentials[j]);
		_potentials[j] = 0;
	}

	return result;

}
