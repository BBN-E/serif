// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/edt/PronounLinkerTrainer/PronounLinkerTrainer.h"
#include "Generic/edt/PronounLinkerUtils.h"
#include "Generic/edt/HobbsDistance.h"
#include "Generic/edt/Guesser.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/state/TrainingLoader.h"
#include "Generic/state/StateLoader.h"
#include "Generic/trainers/AnnotatedParseReader.h"
#include <boost/scoped_ptr.hpp>

PronounLinkerTrainer::PronounLinkerTrainer(int mode_) : 
	 MODE(mode_), 
	_antPriorWriter(2,100), _hobbsDistWriter(2,25),
	_pronHeadWordWriter(4,250), _pronParWordWriter(2,500)
{ 
	_debugOut.init(Symbol(L"pronlink_debug"));
	Guesser::initialize();
}

void PronounLinkerTrainer::train() {
	if (MODE != TRAIN)
		return;

	std::string param = ParamReader::getRequiredParam("pronlink_train_source");
	if (param == "state_files")
		trainOnStateFiles();
	else if (param == "aug_parses")
		trainOnAugParses();
	else
		throw UnexpectedInputException("PronounLinkerTrainer::train()", 
			"Invalid parameter value for 'pronlink_train_source'.  Must be 'state_files' or 'aug_parses'.");

	// we are now set to decode
	MODE = DECODE;
}

void PronounLinkerTrainer::trainOnStateFiles() {
	// MODEL FILE
	std::string model_prefix = ParamReader::getRequiredParam("pronlink_model");

	// TRAINING FILE(S)
	std::string file = ParamReader::getParam("training_file");
	if (!file.empty()) {
		loadTrainingDataFromStateFile(file.c_str());
	} else {
		file = ParamReader::getParam("training_file_list");
		if (!file.empty())
			loadTrainingDataFromStateFileList(file.c_str());
		else  
			throw UnexpectedInputException("PronounLinkerTrainer::trainOnStateFiles()",
									   "Params `training_file' and 'training_file_list' both undefined");
	}

	writeModels(model_prefix.c_str());
}

void PronounLinkerTrainer::trainOnAugParses() {
	// MODEL FILE
	std::string model_prefix = ParamReader::getRequiredParam("pronlink_model");

	std::string file = ParamReader::getParam("training_file");
	if (!file.empty()) {
		loadTrainingDataFromStateFile(file.c_str());
	} else {
		file = ParamReader::getParam("training_file_list");
		if (!file.empty())
			loadTrainingDataFromStateFileList(file.c_str());
		else  
			throw UnexpectedInputException("PronounLinkerTrainer::trainOnAugParses()",
									   "Params `training_file' and 'training_file_list' both undefined");
	}

	writeModels(model_prefix.c_str());
}


void PronounLinkerTrainer::loadTrainingDataFromStateFileList(const char *listfile) {
	boost::scoped_ptr<UTF8InputStream> fileListStream_scoped_ptr(UTF8InputStream::build(listfile));
	UTF8InputStream& fileListStream(*fileListStream_scoped_ptr);	
	UTF8Token token;
	
	int index = 0;
	while (!fileListStream.eof()) {
		fileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		loadTrainingDataFromStateFile(token.chars());
	}

}

void PronounLinkerTrainer::loadTrainingDataFromStateFile(const wchar_t *filename) {
	char state_file_name_str[501];
	StringTransliterator::transliterateToEnglish(state_file_name_str, filename, 500);
	loadTrainingDataFromStateFile(state_file_name_str);
}

void PronounLinkerTrainer::loadTrainingDataFromStateFile(const char *filename) {
	SessionLogger::info("SERIF") << "Loading data from " << filename << "\n";
	
	StateLoader *stateLoader = _new StateLoader(filename);
	int num_docs = TrainingLoader::countDocumentsInFile(filename);

	wchar_t state_tree_name[100];
	wcscpy(state_tree_name, L"DocTheory following stage: doc-relations-events");

	for (int i = 0; i < num_docs; i++) {
		DocTheory *docTheory =  _new DocTheory(static_cast<Document*>(0));
		docTheory->loadFakedDocTheory(stateLoader, state_tree_name);
		docTheory->resolvePointers(stateLoader);
		trainDocument(docTheory);
		delete docTheory;
	}
}

void PronounLinkerTrainer::loadTrainingDataFromAugParseFileList(char *listfile) {
	boost::scoped_ptr<UTF8InputStream> fileListStream_scoped_ptr(UTF8InputStream::build(listfile));
	UTF8InputStream& fileListStream(*fileListStream_scoped_ptr);	
	UTF8Token token;
	
	while (!fileListStream.eof()) {
		fileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		loadTrainingDataFromAugParseFile(token.chars());
	}
}

void PronounLinkerTrainer::loadTrainingDataFromAugParseFile(const wchar_t *filename) {
	char parse_file_name_str[501];
	StringTransliterator::transliterateToEnglish(parse_file_name_str, filename, 500);
	loadTrainingDataFromAugParseFile(parse_file_name_str);
}

void PronounLinkerTrainer::loadTrainingDataFromAugParseFile(const char *filename) {
	GrowableArray <CorefDocument *> documents;
	AnnotatedParseReader trainingSet;

	SessionLogger::info("SERIF") << "Loading data from " << filename << "\n";
	
	trainingSet.openFile(filename);
	trainingSet.readAllDocuments(documents);
	
	while (documents.length()!= 0) {
		CorefDocument *doc= documents.removeLast();
		cout << "\nProcessing document: " << doc->documentName.to_debug_string();
		trainDocument(doc);
		delete doc;
	}
}


void PronounLinkerTrainer::trainDocument(DocTheory *docTheory) {
	GrowableArray <Mention *> frontedPronouns(50);
	std::vector<const Parse *> prevSentences;
	int i, j, k;

	for (i = 0; i < docTheory->getNSentences(); i++) {
		SentenceTheory *sentTheory = docTheory->getSentenceTheory(i);
		EntitySet *entitySet = sentTheory->getEntitySet();
		MentionSet *mentSet = sentTheory->getMentionSet();
		Parse *parse = docTheory->getSentenceTheory(i)->getPrimaryParse();

		//_debugOut << "\nPARSE " << i << "\n";
		//_debugOut << parse->getRoot()->toString(0) << "\n\n";

		for (j = 0; j < mentSet->getNMentions(); j++) {
			Mention *mention = mentSet->getMention(j);
			Entity *entity = entitySet->getEntityByMention(mention->getUID(), mention->getEntityType());
			if (mention != 0 && mention->getMentionType() == Mention::PRON) {

				// CASerif will sometimes (very rarely) put a mention of type X in an entity of type Y.
				// This call is necessary to get the correct entity in these cases.
				if (entity == 0) entity = entitySet->getEntityByMentionWithoutType(mention->getUID());

				if (!WordConstants::isPronoun(mention->getHead()->getHeadWord()))
					continue;
				if (!mention->getEntityType().isRecognized()) {
					registerPronounLink(NULL, NULL, mention, SymbolConstants::nullSymbol);
					_debugOut << "Non-referring pronoun found: " << mention->getHead()->toFlatString() << "\n";
				}
				// we've already seen another non-pronoun mention of this pronoun
				else if (antecedentAlreadySeen(entitySet, entity, mention)) {
					// hobbs search for antecedent
					const int MAX_CANDIDATES = 100;
					HobbsDistance::SearchResult candidates[MAX_CANDIDATES];
					const SynNode *thisCandidate;
					int nCandidates = HobbsDistance::getCandidates(mention->getHead(), prevSentences, candidates, MAX_CANDIDATES);
					Symbol hobbsDistance;

					// look in Hobbs results for a mention in the same entity
					Mention *antecedent = 0;
					for (k = 0; k < nCandidates; k++) {
						thisCandidate = candidates[k].node;
						if (thisCandidate->hasMention()) {
							Mention *candidateMention = docTheory->getSentenceTheory(candidates[k].sentence_number)->getMentionSet()->getMentionByNode(thisCandidate);
							Entity *candidateEntity = entitySet->getEntityByMention(candidateMention->getUID());
							if (candidateEntity != 0 && entity->getID() == candidateEntity->getID()) {
								antecedent = candidateMention;
								hobbsDistance = PronounLinkerUtils::getNormDistSymbol(k+1);
								break;
							}
						}
					}

					// True antecedent is not a hobbs candidate. Just find the most recent mention in
					// the same entity. i.e., search backwards in the entity's mentions list
					if (antecedent == 0) {
						for (k = entity->getNMentions() - 1; k > 0; k--) {
							if (mention->getUID() == entity->getMention(k)) {
								antecedent = entitySet->getMention(entity->getMention(k-1));
								hobbsDistance = SymbolConstants::unreachableSymbol;
								break;
							}
						}
					}
					if (antecedent == 0) {
						throw InternalInconsistencyException("PronounLinkerTrainer::trainDocument()", 
															 "Expected antecedent not found.");
					}

					Mention *antecedentHead = antecedent;
					while (antecedentHead->getChild() != 0) {
						antecedentHead = antecedentHead->getChild();
					}
					_debugOut << "link found: <antecedent> " << antecedentHead->getNode()->toFlatString()
							  << ", <pronoun> " << mention->getHead()->toFlatString() << "\n";
					registerPronounLink(antecedent, antecedentHead->getNode(), mention, hobbsDistance);
				}
				else {
					// we have not yet seen a non-pronoun in this entity
					// add to fronted pronoun list
					frontedPronouns.add(mention);
				}
			}
			else if (entity != 0) {
				// not a pronoun. Check for fronted pronouns in the same entity
				for (k = 0; k < frontedPronouns.length(); k++) {
					Entity *frontedEntity = entitySet->getEntityByMention(frontedPronouns[k]->getUID());
					if (frontedEntity->getID() == entity->getID())
						break;
				}

				if (k != frontedPronouns.length()) {
					// found a matching fronted pronoun. Register pronoun link, then remove pronoun from set
					Mention *mentionHead = mention;
					while (mentionHead->getChild() != 0) {
						mentionHead = mentionHead->getChild();
					}
					
					_debugOut << "link found: <antecedent> " << mentionHead->getNode()->toFlatString()
						      << ", <pronoun> " << frontedPronouns[k]->getHead()->toFlatString() << "\n";
					registerPronounLink(mention, mentionHead->getNode(), frontedPronouns[k], SymbolConstants::unreachableSymbol);
					// now remove this fronted pronoun, keeping the two sets parallel
					Mention *lastPronoun = frontedPronouns.removeLast();
					// either this is the pronoun itself, or this is what we will use to plug in the hole
					// left by the pronoun
					// if the index is no longer valid, we are in the first case (i.e. we're done). otherwise...
					if (k < frontedPronouns.length()) {
						// second case: fill up the hole
						frontedPronouns[k] = lastPronoun;
					}
				}
			}

		}
		prevSentences.push_back(parse);
	}
}

void PronounLinkerTrainer::trainDocument(CorefDocument *document) {
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
			if(!WordConstants::isPronoun(mention->getHead()->getHeadWord()))
				continue;
			else if(!item->hasID()) {
				//register this as a non-referring pronoun
				registerPronounLink(NULL, NULL,mention, SymbolConstants::nullSymbol);
				_debugOut << "Non-referring pronoun found: " << mention->getHead()->toFlatString() << "\n";
			}
			else if(alreadySeenIDs.find(corefID)!=-1) {
				//find the antecedent of this pronoun and process the pronoun-antecedent pair
				std::vector<const Parse *> prevSentences;
				//set mentionRoot to be the root of the mention's parse tree
				const SynNode *mentionRoot = mention->node;
				while(mentionRoot->getParent() != NULL)
					mentionRoot = mentionRoot->getParent();
				int j;
				for(j=0; document->parses[j]->getRoot() != mentionRoot; j++) {
					if(j > document->parses.length())
						throw InternalInconsistencyException("PronounLinkerTrainer::trainDocument()", "Pronoun's parse tree not found in parses array!");
					prevSentences.push_back(document->parses[j]);
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
					throw InternalInconsistencyException("PronounLinkerTrainer::trainDocument()", "Expected antecedent not found.");
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

void PronounLinkerTrainer::registerPronounLink(Mention *antecedent, 
													  const SynNode *antecedentNode,
											          Mention *pronoun, 
													  Symbol hobbsDistance) 
{
	const SynNode *pronounNode = pronoun->getHead();

	Symbol antTNG[3];
	if (antecedentNode) {
		antTNG[0] = Guesser::guessType(antecedentNode, antecedent);
		antTNG[1] = Guesser::guessNumber(antecedentNode, antecedent);
		antTNG[2] = Guesser::guessGender(antecedentNode, antecedent);
	}
	else {
		antTNG[0] = SymbolConstants::nullSymbol;
		antTNG[1] = SymbolConstants::nullSymbol;
		antTNG[2] = SymbolConstants::nullSymbol;
	}

	_debugOut << "(" << hobbsDistance.to_string() << " ";
	_debugOut << antTNG[0].to_string() << " ";
	_debugOut << antTNG[1].to_string() << " ";
	_debugOut << antTNG[2].to_string() << ")\n";

	Symbol nullToAnt[2] = {SymbolConstants::nullSymbol, PronounLinkerUtils::combineSymbols(antTNG, 3)};
	_antPriorWriter.registerTransition(nullToAnt);

	Symbol nullToDist[2] = {SymbolConstants::nullSymbol, hobbsDistance};
	_hobbsDistWriter.registerTransition(nullToDist);

	Symbol antTNGToPronoun[4] = {antTNG[0], antTNG[1], antTNG[2], pronounNode->getHeadWord()};
	_pronHeadWordWriter.registerTransition(antTNGToPronoun);

	Symbol antTypeToParWord[2] = {PronounLinkerUtils::augmentIfPOS(antTNG[0], pronounNode),
								  PronounLinkerUtils::getAugmentedParentHeadWord(pronounNode)};
	_pronParWordWriter.registerTransition(antTypeToParWord);
}

void PronounLinkerTrainer::writeModels(const char* model_prefix) {
	char file[501];

	sprintf(file,"%s.antprior", model_prefix);
	UTF8OutputStream antPriorStream(file);
	if (antPriorStream.fail()) {
		throw UnexpectedInputException("PronounLinkerTrainer::writeModels()",
									   "Could not open model file(s) for writing");
	}
	_antPriorWriter.writeModel(antPriorStream);
	antPriorStream.close();

	sprintf(file,"%s.distmodel", model_prefix);
	UTF8OutputStream hobbsDistStream(file);
	if (hobbsDistStream.fail()) {
		throw UnexpectedInputException("PronounLinkerTrainer::writeModels()",
									   "Could not open model file(s) for writing");
	}
	_hobbsDistWriter.writeModel(hobbsDistStream);
	hobbsDistStream.close();

	sprintf(file,"%s.pronheadmodel", model_prefix);
	UTF8OutputStream pronHeadWordStream(file);
	if (pronHeadWordStream.fail()) {
		throw UnexpectedInputException("PronounLinkerTrainer::writeModels()",
									   "Could not open model file(s) for writing");
	}
	_pronHeadWordWriter.writeModel(pronHeadWordStream);
	pronHeadWordStream.close();


	sprintf(file,"%s.parwordmodel", model_prefix);
	UTF8OutputStream pronParWordStream(file);
	if (pronParWordStream.fail()) {
		throw UnexpectedInputException("PronounLinkerTrainer::writeModels()",
									   "Could not open model file(s) for writing");
	}
	_pronParWordWriter.writeModel(pronParWordStream);
	pronParWordStream.close();
}

bool PronounLinkerTrainer::antecedentAlreadySeen(EntitySet *entitySet,
														Entity *entity, 
														Mention *ment) 
{
	for (int i = 0; i < entity->getNMentions(); i++) {
		if (entity->getMention(i) == ment->getUID())
			return false;
		if (entitySet->getMention(entity->getMention(i))->getMentionType() != Mention::PRON)
			return true;
	}
	return false;
}


