// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "RelationVectorExtractor/CAVectorResultCollector.h"
#include "CASerif_generic/correctanswers/CorrectDocument.h"
#include "CASerif_generic/correctanswers/CorrectMention.h"
#include "theories/DocTheory.h"
#include "theories/SentenceTheory.h"
#include "theories/PropositionSet.h"
#include "theories/RelMentionSet.h"
#include "theories/RelMention.h"
#include "theories/RelationConstants.h"
#include "theories/Parse.h"
#include "common/UTF8OutputStream.h"
#include "common/ParamReader.h"
#include "relations/PotentialRelationCollector.h"
#include "Chinese/relations/ch_PotentialRelationInstance.h"
#include "common/NgramScoreTable.h"

CAVectorResultCollector::CAVectorResultCollector() :
	_relationCollector(PotentialRelationCollector::TRAIN),
	_vectors(CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE, MAX_DOCUMENT_RELATIONS)
{ }

void CAVectorResultCollector::loadDocTheory(DocTheory *docTheory, CorrectDocument *cd) {
	_docTheory = docTheory;
	_correctDocument = cd;
	_relationCollector.loadDocTheory(docTheory);
}

void CAVectorResultCollector::produceOutput(const char *output_dir, const char *document_filename) {
	char output_file[210];
	strncpy(output_file, output_dir, 100);
	strcat(output_file, "/");
	strncat(output_file, document_filename, 100);
	strcat(output_file, ".vectors");

	alignAndCollectVectors();
}

void CAVectorResultCollector::alignAndCollectVectors() {
	int i, j, k;
	for (i = 0; i < _docTheory->getNSentences(); i++) {
		_relationCollector.resetForNewSentence();
		PropositionSet *props = _docTheory->getSentenceTheory(i)->getPropositionSet();
		MentionSet *mentionSet = _docTheory->getSentenceTheory(i)->getMentionSet();
		Parse *parse = _docTheory->getSentenceTheory(i)->getParse();
		TokenSequence *tokenSequence = _docTheory->getSentenceTheory(i)->getTokenSequence();
		_relationCollector.collectPotentialSentenceRelations(parse, mentionSet, props);

		RelMentionSet *correctRelations = _docTheory->getSentenceTheory(i)->getRelMentionSet();

		for (j = 0; j < _relationCollector.getNRelations(); j++) {
			PotentialRelationInstance *instance = _relationCollector.getPotentialRelationInstance(j);
			int left = instance->getLeftMention();
			int right = instance->getRightMention();

			bool found = false;
			for (k = 0; k < correctRelations->getNRelMentions(); k++) {
				RelMention *ment = correctRelations->getRelMention(k);

				if (left == ment->getLeftMention()->getUID() &&
					right == ment->getRightMention()->getUID())
				{
					instance->setRelationType(ment->getType());
					found = true;
					if (exactMentionMatch(ment, left, right, mentionSet))
						break;
				}
				if (right == ment->getLeftMention()->getUID() &&
					left == ment->getRightMention()->getUID())
				{
					instance->setRelationType(ment->getType());
					instance->setReverse(true);
					found = true;
					if (exactMentionMatch(ment, right, left, mentionSet))
						break;
				}
			}
			if (!found)
				instance->setRelationType(RelationConstants::NONE);

			adjustHeadWords(left, right, instance, tokenSequence);
			_vectors.add(instance->getTrainingNgram());
		}
	}
}

bool CAVectorResultCollector::exactMentionMatch(RelMention *relMent,
												int one, int two,
												MentionSet *mentionSet)
{
	Mention *first = mentionSet->getMention(one);
	Mention *second = mentionSet->getMention(two);

	if (first->getNode()->getStartToken() == relMent->getLeftMention()->getNode()->getStartToken() &&
		first->getNode()->getEndToken() == relMent->getLeftMention()->getNode()->getEndToken() &&
		second->getNode()->getStartToken() == relMent->getRightMention()->getNode()->getStartToken() &&
		second->getNode()->getEndToken() == relMent->getRightMention()->getNode()->getEndToken())
		return true;

	return false;
}


void CAVectorResultCollector::alignAndPrintVectors(UTF8OutputStream &vectorStream) {
	NgramScoreTable vectors(CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE, MAX_DOCUMENT_RELATIONS);

	int i, j, k;
	for (i = 0; i < _docTheory->getNSentences(); i++) {
		_relationCollector.resetForNewSentence();
		PropositionSet *props = _docTheory->getSentenceTheory(i)->getPropositionSet();
		MentionSet *mentionSet = _docTheory->getSentenceTheory(i)->getMentionSet();
		Parse *parse = _docTheory->getSentenceTheory(i)->getParse();
		TokenSequence *tokenSequence = _docTheory->getSentenceTheory(i)->getTokenSequence();
		_relationCollector.collectPotentialSentenceRelations(parse, mentionSet, props);

		RelMentionSet *correctRelations = _docTheory->getSentenceTheory(i)->getRelMentionSet();

		for (j = 0; j < _relationCollector.getNRelations(); j++) {
			PotentialRelationInstance *instance = _relationCollector.getPotentialRelationInstance(j);
			int left = instance->getLeftMention();
			int right = instance->getRightMention();

			bool found = false;
			for (k = 0; k < correctRelations->getNRelMentions(); k++) {
				RelMention *ment = correctRelations->getRelMention(k);

				if (left == ment->getLeftMention()->getUID() &&
					right == ment->getRightMention()->getUID())
				{
					instance->setRelationType(ment->getType());
					found = true;
					if (exactMentionMatch(ment, left, right, mentionSet))
						break;
				}
				if (right == ment->getLeftMention()->getUID() &&
					left == ment->getRightMention()->getUID())
				{
					instance->setRelationType(ment->getType());
					instance->setReverse(true);
					found = true;
					if (exactMentionMatch(ment, right, left, mentionSet))
						break;
				}
			}
			if (!found)
				instance->setRelationType(RelationConstants::NONE);

			adjustHeadWords(left, right, instance, tokenSequence);
			vectors.add(instance->getTrainingNgram());
		}
	}
	vectors.print_to_open_stream(vectorStream);
}

void CAVectorResultCollector::adjustHeadWords(int left, int right,
											  PotentialRelationInstance *inst,
											  TokenSequence *tokenSequence)
{
	int startTok, endTok;

	// first mention head
	CorrectMention *cm_one = _correctDocument->getCorrectMentionFromMentionID(left);
	startTok = static_cast<int>(cm_one->head_start_token);
	endTok = static_cast<int>(cm_one->head_end_token);
	/*if (startTok != endTok)
		throw InternalInconsistencyException("CAVectorResultCollector::adjustHeadWords()",
												"Head word spans more than one token.");*/
	Symbol firstSym = tokenSequence->getToken(endTok)->getSymbol();

	// second mention head
	CorrectMention *cm_two = _correctDocument->getCorrectMentionFromMentionID(right);
	startTok = static_cast<int>(cm_two->head_start_token);
	endTok = static_cast<int>(cm_two->head_end_token);
	/*if (startTok != endTok)
		throw InternalInconsistencyException("CAVectorResultCollector::adjustHeadWords()",
												"Head word spans more than one token.");*/
	Symbol secondSym = tokenSequence->getToken(endTok)->getSymbol();

	inst->setLeftHeadword(firstSym);
	inst->setRightHeadword(secondSym);
}

void CAVectorResultCollector::printVectors() {
	char output_file[501];
	if (!ParamReader::getParam("vector_file_prefix",output_file,									 500))		throw UnexpectedInputException("CAVectorResultCollector::printVectors()",									   "Param `vector_file_prefix' not defined");
	strcat(output_file, ".vectors");

	UTF8OutputStream vectorStream(output_file);
	_vectors.print_to_open_stream(vectorStream);
	vectorStream.close();

}
