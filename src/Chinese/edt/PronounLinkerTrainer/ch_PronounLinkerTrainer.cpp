// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Chinese/edt/PronounLinkerTrainer/ch_PronounLinkerTrainer.h"
#include "Chinese/edt/ch_PronounLinkerUtils.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/HobbsDistance.h"
#include "Chinese/edt/ch_Guesser.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/WordConstants.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/common/ParamReader.h"

ChinesePronounLinkerTrainer::ChinesePronounLinkerTrainer() : _antPriorWriter(2,100), _hobbsDistWriter(2,25),
	_pronHeadWordWriter(4,250), _pronParWordWriter(2,500)
{
	Guesser::initialize();

	// open output files
	char prefix[501];
	if (!ParamReader::getParam("pronlink_model",prefix,									 500))	{
		throw UnexpectedInputException("ChinesePronounLinkerTrainer::ChinesePronounLinkerTrainer()",
									   "Param `pronlink_model' not defined");
	}

	openOutputFiles(prefix);

}

ChinesePronounLinkerTrainer::~ChinesePronounLinkerTrainer() {
	closeInputFile();
	closeOutputFiles();
}

void ChinesePronounLinkerTrainer::openInputFile(char *trainingFile) {
	if(trainingFile != NULL)
		_trainingSet.openFile(trainingFile);
}

void ChinesePronounLinkerTrainer::closeInputFile() {
	_trainingSet.closeFile();
}

void ChinesePronounLinkerTrainer::openOutputFiles(const char* model_prefix)
{
	char file[501];

	sprintf(file,"%s.antprior", model_prefix);
	_antPriorStream.open(file);

	sprintf(file,"%s.distmodel", model_prefix);
	_hobbsDistStream.open(file);

	sprintf(file,"%s.pronheadmodel", model_prefix);
	_pronHeadWordStream.open(file);

	sprintf(file,"%s.parwordmodel", model_prefix);
	_pronParWordStream.open(file);
}

void ChinesePronounLinkerTrainer::closeOutputFiles() {
	_antPriorStream.close();
	_hobbsDistStream.close();
	_pronHeadWordStream.close();
	_pronParWordStream.close();

}

void ChinesePronounLinkerTrainer::trainModels() {
	//read documents, train on each document in turn
//	char buffer[256] = "c:\\data\\reader_debug.txt";
//	_trainingSet.setDebugStream(buffer);
	GrowableArray <CorefDocument *> documents;
	_trainingSet.readAllDocuments(documents);
	while(documents.length()!= 0) {
		CorefDocument *thisDocument = documents.removeLast();
		cout << "\nProcessing document: " << thisDocument->documentName.to_debug_string();
		trainDocument(thisDocument);
		delete thisDocument;
	}
}


void ChinesePronounLinkerTrainer::trainDocument(CorefDocument *document) {
	GrowableArray <CorefItem *> frontedPronouns(50); //unresolvedPronouns(50),
	GrowableArray <int> alreadySeenIDs(50);
	int i;

	/*
	for(i=0; i<document->parses.length(); i++) {
		_debugOut << "\nPARSE " << i << "\n";
		_debugOut << document->parses[i]->getRoot()->toString(0);
	}
*/

	for(i=0; i<document->corefItems.length(); i++) {
		CorefItem *item = document->corefItems[i];

		Mention *mention = item->mention;
		//int corefID = document->corefIDs[i];
		if(mention != NULL && mention->mentionType == Mention::PRON) {
			//pronouns can only have one ID
			int corefID = (item->corefID == -1) ? item->prolinkID : item->corefID;
			if(!WordConstants::isLinkingPronoun(mention->getHead()->getHeadWord()))
				continue;
			else if(!item->hasID()) {
				//register this as a non-referring pronoun
				registerPronounLink(NULL, NULL,mention, SymbolConstants::nullSymbol);
				_debugOut << "Non-referring pronoun found: " << mention->getHead()->toFlatString() << "\n";
			}
			else if(alreadySeenIDs.find(corefID)!=-1) {
				//find the antecedent of this pronoun and process the pronoun-antecedent pair
				GrowableArray <const Parse *> prevSentences(document->parses.length());
				//set mentionRoot to be the root of the mention's parse tree
				const SynNode *mentionRoot = mention->node;
				while(mentionRoot->getParent() != NULL)
					mentionRoot = mentionRoot->getParent();
				int j;
				for(j=0; document->parses[j]->getRoot() != mentionRoot; j++) {
					if(j > document->parses.length())
						throw InternalInconsistencyException("ChinesePronounLinkerTrainer::trainDocument()", "Pronoun's parse tree not found in parses array!");
					prevSentences.add(document->parses[j]);
				}

				//hobbs search for antecedent
				const int MAX_CANDIDATES = 100;
				HobbsDistance::SearchResult candidates[MAX_CANDIDATES];
				const SynNode *thisCandidate;
				int nCandidates = HobbsDistance::getCandidates(mention->getHead(), prevSentences, candidates, MAX_CANDIDATES);
				Symbol hobbsDistance;

				CorefItem *antecedent = NULL;
				//look in these results for something with the same corefID
				for(j=0; j < nCandidates; j++) {
					thisCandidate = candidates[j].node;
					if(thisCandidate->getMentionIndex() != -1) {
						if(document->corefItems[thisCandidate->getMentionIndex()]->corefersWith(*item)) {
							antecedent = document->corefItems[thisCandidate->getMentionIndex()];
							hobbsDistance = PronounLinkerUtils::getNormDistSymbol(j+1);
							break;
						}
					}
				}
				if(antecedent == NULL) {
					//true antecedent is not a hobbs candidate. Just find the most recent mention having
					//the same ID. i.e., search backwards from position i in the mentions list
					for(j=i-1; j>=0; j--)
						if(document->corefItems[j]->corefersWith(*item)) {
							antecedent = document->corefItems[j];
							hobbsDistance = SymbolConstants::unreachableSymbol;
							break;
						}
				}
				if(antecedent == NULL)
					throw InternalInconsistencyException("ChinesePronounLinkerTrainer::trainDocument()", "Expected antecedent not found.");
				_debugOut << "link found: <antecedent> " << antecedent->node->toFlatString()
					<< ", <pronoun> " << mention->getHead()->toFlatString() << "\n";
				registerPronounLink(antecedent->mention, antecedent->node, mention, hobbsDistance);
			} // (alreadySeenIDs.find(corefID)!=-1)
			else {
				//we have not yet seen a non-pronoun with this mention's corefID.
				// Add to fronted pronouns list
				frontedPronouns.add(item);
			}
		} // mention->mentionType == Mention::PRON
		else {
			//not a pronoun. Check for fronted pronouns with the same corefID
			int index;
			for(index=0; index<frontedPronouns.length(); index++)
				if(frontedPronouns[index]->corefersWith(*item))
					break;

			if(index != frontedPronouns.length()) {
				//found a matching fronted pronoun. Register pronoun link, then remove pronoun from set
				registerPronounLink(mention, item->node, frontedPronouns[index]->mention, SymbolConstants::unreachableSymbol);
				_debugOut << "link found: <antecedent> " << item->node->toFlatString()
					<< ", <pronoun> " << frontedPronouns[index]->mention->getHead()->toFlatString() << "\n";
				//now remove this fronted pronoun, keeping the two sets parallel
				CorefItem *lastPronoun = frontedPronouns.removeLast();
				//either this is the pronoun itself, or this is what we will use to plug in the hole
				//left by the pronoun
				//if the index is no longer valid, we are in the first case (i.e. we're done). otherwise...
				if(index < frontedPronouns.length()) {
					//second case: fill up the hole
					frontedPronouns[index] = lastPronoun;
				}
				for(index=0; index<frontedPronouns.length(); index++)
					if(frontedPronouns[index]->corefersWith(*item))
						break;
			}
			//add current IDs to list of already seen
			if(item->corefID != CorefItem::NO_ID && alreadySeenIDs.find(item->corefID) == -1)
				alreadySeenIDs.add(item->corefID);
			if(item->prolinkID != CorefItem::NO_ID && alreadySeenIDs.find(item->prolinkID) == -1)
				alreadySeenIDs.add(item->prolinkID);
		}
	}
}

void ChinesePronounLinkerTrainer::registerPronounLink(Mention *antecedent, const SynNode *antecedentNode,
											   Mention *pronoun, Symbol hobbsDistance) {
	const SynNode *pronounNode;
	pronounNode = pronoun->getHead();

	Symbol antTNG[3];
	if(antecedentNode) {
		antTNG[0] = Guesser::guessType(antecedentNode, antecedent);
		antTNG[1] = Guesser::guessNumber(antecedentNode, antecedent);
		antTNG[2] = Guesser::guessGender(antecedentNode, antecedent);
	}
	else {
		antTNG[0] = SymbolConstants::nullSymbol;
		antTNG[1] = SymbolConstants::nullSymbol;
		antTNG[2] = SymbolConstants::nullSymbol;
	}

	Symbol nullToAnt[2] = {SymbolConstants::nullSymbol, PronounLinkerUtils::combineSymbols(antTNG, 3)};
	_antPriorWriter.registerTransition(nullToAnt);

	Symbol nullToDist[2] = {SymbolConstants::nullSymbol, hobbsDistance};
	_hobbsDistWriter.registerTransition(nullToDist);

	Symbol antTNGToPronoun[4] = {antTNG[0], antTNG[1], antTNG[2], pronounNode->getHeadWord()};
	_pronHeadWordWriter.registerTransition(antTNGToPronoun);

	Symbol antTypeToParWord[2] = {PronounLinkerUtils::augmentIfPOS(antTNG[0], pronounNode),
								  PronounLinkerUtils::getAugmentedParentHeadWord(pronounNode)};
	_pronParWordWriter.registerTransition(antTypeToParWord);

	//also, resolve the entity type of the pronoun mention
    if(antecedent != NULL)
		pronoun->entityType = antecedent->entityType;
}

void ChinesePronounLinkerTrainer::writeModels() {
	_antPriorWriter.writeModel(_antPriorStream);
	_hobbsDistWriter.writeModel(_hobbsDistStream);
	_pronHeadWordWriter.writeModel(_pronHeadWordStream);
	_pronParWordWriter.writeModel(_pronParWordStream);
}





