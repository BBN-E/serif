// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/DescriptorLinkerTrainer/DescriptorLinkerTrainer.h"
#include "Generic/trainers/CorefDocument.h"
#include "Generic/edt/StatDescLinker.h"
#include "Generic/edt/DescLinkFunctions.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/EntityType.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolHash.h"
#include <boost/scoped_array.hpp>

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

const float targetLoadingFactor = static_cast<float>(0.7);

DescriptorLinkerTrainer::DescriptorLinkerTrainer(const char* output_file) :
										_primaryModel(0), _secondaryModel(0),
										_searcher(false),
										_partitiveClassifier(), _appositiveClassifier(),
										_listClassifier(), _nestedClassifier(),
										_pronounClassifier(PronounClassifier::build()), _otherClassifier()
{

	// Load param settings for training options
	int mode;
	int stop_criterion;
	int percent_held_out;
	double variance;

	std::string buffer;
	_cutoff = ParamReader::getRequiredIntParam("desclink_train_pruning_cutoff");
	_threshold = ParamReader::getRequiredFloatParam("desclink_train_threshold");

	buffer = ParamReader::getRequiredParam("desclink_train_mode");
	if (buffer == "GIS")
		mode = OldMaxEntModel::GIS;
	else if (buffer == "IIS")
		mode = OldMaxEntModel::IIS;
	else if (buffer == "IIS_GAUSSIAN") {
		mode = OldMaxEntModel::IIS_GAUSSIAN;
		variance = ParamReader::getRequiredFloatParam("desclink_train_gaussian_variance");
	}
	else if (buffer == "IIS_FEATURE_SELECTION")
		mode = OldMaxEntModel::IIS_FEATURE_SELECTION;
	else
		throw UnrecoverableException("DescriptorLinkerTrainer()",
									 "Invalid setting for parameter 'desclink_train_mode'");

	buffer = ParamReader::getRequiredParam("desclink_train_stop_criterion");
	if (buffer == "PROBS_CONVERGE")
		stop_criterion = OldMaxEntModel::PROBS_CONVERGE;
	else if (buffer == "HELD_OUT_LIKELIHOOD")
		stop_criterion = OldMaxEntModel::HELD_OUT_LIKELIHOOD;
	else
		throw UnrecoverableException("DescriptorLinkerTrainer()",
									 "Invalid setting for parameter 'desclink_train_stop_criterion'");

	percent_held_out = ParamReader::getRequiredIntParam("desclink_train_percent_held_out");
	if (percent_held_out < 0 || percent_held_out > 100)
		throw UnrecoverableException("DescriptorLinkerTrainer()",
									 "Invalid setting for parameter 'desclink_train_percent_held_out'");


	// Set up link types
	Symbol symArray[2];
	symArray[0] = DescLinkFunctions::OC_LINK;
	symArray[1] = DescLinkFunctions::OC_NO_LINK;

	_primaryModel = _new OldMaxEntModel(2, symArray, mode, stop_criterion, percent_held_out, variance);
	//_secondaryModel = _new OldMaxEntModel(2, symArray, mode, stop_criterion, percent_held_out, variance);
}

DescriptorLinkerTrainer::~DescriptorLinkerTrainer() {
	closeInputFile();
	delete _primaryModel;
	delete _secondaryModel;

	while (_primaryEvents.length() > 0) {
		OldMaxEntEvent *e = _primaryEvents.removeLast();
		delete e;
	}
	/*while (_secondaryEvents.length() > 0) {
		OldMaxEntEvent *e = _secondaryEvents.removeLast();
		delete e;
	}*/
}

void DescriptorLinkerTrainer::openInputFile(const char *inputFile) {
	if(inputFile != NULL)
		_inputSet.openFile(inputFile);
}

void DescriptorLinkerTrainer::closeInputFile() {
	_inputSet.closeFile();
}


void DescriptorLinkerTrainer::extractDescLinkEvents() {
	//read documents, train on each document in turn
	GrowableArray<CorefDocument*> documents;
	_inputSet.readAllDocuments(documents);
	while(documents.length() != 0) {
		CorefDocument *thisDocument = documents.removeLast();
		SessionLogger::info("SERIF") << "Processing document: \"" << thisDocument->documentName.to_debug_string() << "\"\n";
		processDocument(thisDocument);
		delete thisDocument;
	}
}

void DescriptorLinkerTrainer::processDocument(CorefDocument *document) {
	EntitySet *entitySet = _new EntitySet(document->parses.length());
	MentionSet *mentionSet[MAX_DOCUMENT_SENTENCES];
	CorefItemList corefItems;

	// initialize coref ID hash
	int numBuckets = static_cast<int>(document->corefItems.length() / targetLoadingFactor);
	numBuckets = (numBuckets >= 2 ? numBuckets : 5);
	_alreadySeenIDs = _new IntegerMap(numBuckets);

	for (int i = 0; i < document->parses.length(); i++) {

		// collect all CorefItems associated with parse before creating MentionSet,
		// because it overwrites each node's mention pointer
		findSentenceCorefItems(corefItems, document->parses[i]->getRoot(), document);
		mentionSet[i] = _new MentionSet(document->parses[i], i);

		// add annotated mention and entity types to the new mention set
		boost::scoped_array<int> coref_ids(new int[mentionSet[i]->getNMentions()]);
		addCorefItemsToMentionSet(mentionSet[i], corefItems, coref_ids.get());

		// classify mention types to rule out all but valid DESC mentions
		processMentions(document->parses[i]->getRoot(), mentionSet[i]);

		// add mentions to entity sets and collect desc link possibilities as we go
		entitySet->loadMentionSet(mentionSet[i]);
		processEntities(mentionSet[i], entitySet, coref_ids.get());

		/*std::cout << std::endl;
		document->parses[i]->dump(std::cout);
		std::cout << std::endl;
		mentionSet[i]->dump(std::cout);
		std::cout << std::endl;
		entitySet->dump(std::cout);
		std::cout << std::endl;*/

		while (corefItems.length() > 0)
			CorefItem *item = corefItems.removeLast();
	}

	// clean up data structures
	for (int j = 0; j < document->parses.length(); j++) {
		delete mentionSet[j];
	}
	delete _alreadySeenIDs;

	delete entitySet;
}

void DescriptorLinkerTrainer::findSentenceCorefItems(CorefItemList &items,
														const SynNode *node,
														CorefDocument *document)
{

	if (node->hasMention()) {
		if  (document->corefItems[node->getMentionIndex()]->mention != 0)
			items.add(document->corefItems[node->getMentionIndex()]);
		// Reset the mention index to default -1, so we don't have problems later
		// with non-NPKind coref items
		document->corefItems[node->getMentionIndex()]->node->setMentionIndex(-1);
	}

	for (int i = 0; i < node->getNChildren(); i++) {
		findSentenceCorefItems(items, node->getChild(i), document);
	}

}

void DescriptorLinkerTrainer::addCorefItemsToMentionSet(MentionSet* mentionSet,
												   CorefItemList &items,
												   int ids[])
{
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		Mention *mention = mentionSet->getMention(i);
		ids[i] = CorefItem::NO_ID;
		const SynNode *node = mention->getNode();
		for (int j = 0; j < items.length(); j++) {
			if (items[j]->node == node) {
				mention->setEntityType(items[j]->mention->getEntityType());
				ids[i] = items[j]->corefID;
				// Only add NAME mention type, bc mention classifiers die on already-marked DESCs
				// JM : also have to add PRON
				if (items[j]->mention->mentionType == Mention::NAME ||
					items[j]->mention->mentionType == Mention::PRON)
					mention->mentionType = items[j]->mention->mentionType;

				break;
			}
		}
	}
}

void DescriptorLinkerTrainer::processMentions(const SynNode* node, MentionSet *mentionSet) {

	// first time into the sentence, so initialize the searcher
	_searcher.resetSearch(mentionSet, 1);
	processNode(node);

}

void DescriptorLinkerTrainer::processNode(const SynNode* node) {

	// do children first
	for (int i = 0; i < node->getNChildren(); i++)
		processNode(node->getChild(i));

	// process this node only if it has a mention
	if (!node->hasMention())
		return;

	// Test 1: See if it's a partitive
	_searcher.performSearch(node, _partitiveClassifier);

	// Test 2: See if it's an appositive
	// won't do anything if it's already partitive
	_searcher.performSearch(node, _appositiveClassifier);

	// Test 3: See if it's a list
	// won't do anything if it's part or appos
	_searcher.performSearch(node, _listClassifier);

	// Test 4: See if it's a nested mention
	// won't do anything if it's part, appos, or list
	// note that being nested doesn't preclude later classification
	_searcher.performSearch(node, _nestedClassifier);

	// test 5: See if it's a pronoun (and mark it but don't classify till later)
	// won't do anything if it's part, appos, list, or nested
	_searcher.performSearch(node, *_pronounClassifier);

	// label everything else as DESC here
	_searcher.performSearch(node, _otherClassifier);
}


void DescriptorLinkerTrainer::processEntities(MentionSet *mentionSet, EntitySet *entitySet, int ids[]) {

	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		Mention *mention = mentionSet->getMention(i);
		if (mention->isOfRecognizedType()) {
			if (mention->mentionType == Mention::NAME ||
				mention->mentionType == Mention::PRON ||
				mention->mentionType == Mention::DESC)
			{
				int coref_id = ids[i];
				if (coref_id == CorefItem::NO_ID) {
					Mention *child = mention->getChild();
					while (child != NULL) {
						if (child->getMentionType() == Mention::NONE)
							coref_id = ids[child->getIndex()];
						child = child->getChild();
					}
				}
//				std::cout << "Ading new mention of type: " << mention->mentionType
//						  << " with UID " << mention->getUID() << " and COREFID " << coref_id << "\n";
				//if (coref_id != CorefItem::NO_ID) {
					if (mention->mentionType == Mention::DESC && !mention->hasApposRelationship())
						collectLinkEvents(mention, coref_id, entitySet);
					if (_alreadySeenIDs->get(coref_id) == 0) {
						entitySet->addNew(mention->getUID(), mention->getEntityType());
						if (coref_id != CorefItem::NO_ID)
							(*_alreadySeenIDs)[coref_id] = entitySet->getEntityByMention(mention->getUID())->getID();
					}
					else
						entitySet->add(mention->getUID(), (*_alreadySeenIDs)[coref_id]);
				//}
			}
		}
	}
}

MentionUID DescriptorLinkerTrainer::_getLatestMention(Entity* ent) {
	MentionUID lastID;
	for (int j = 0; j < ent->getNMentions(); j++) {
		if (ent->getMention(j) > lastID)
			lastID = ent->getMention(j);
	}
	return lastID;
}

void DescriptorLinkerTrainer::collectLinkEvents(Mention *ment, int id, EntitySet *entitySet) {
	const GrowableArray <Entity *> &entities = entitySet->getEntitiesByType(ment->getEntityType());
	// EXPERIMENT: sort entities by the last mention id
	GrowableArray <Entity *> sortedEnts;
	// brute force method of sorting
	for (int i = 0; i < entities.length(); i++) {
		Entity* ent = entities[i];
		MentionUID lastID = _getLatestMention(ent);
		// find where the entity should be added
		bool inserted = false;
		for (int j = 0; j < sortedEnts.length(); j++) {
			if (lastID < _getLatestMention(sortedEnts[j])) {
				// move the last one down one
				sortedEnts.add(sortedEnts[sortedEnts.length()-1]);
				for (int k = sortedEnts.length()-2; k >= j; k--)
					sortedEnts[k+1] = sortedEnts[k];
				sortedEnts[j] = ent;
				inserted = true;
				break;
			}
		}
		if (!inserted)
			sortedEnts.add(ent);
	}

/*	if (sortedEnts.length() > 1) {
		std::cout << "Mention id is " << ment->getUID() << "\n";
		for (int i = 0; i < sortedEnts.length(); i++) {
				Entity* ent = sortedEnts[i];
				int lastID = 0;
				for (int j = 0; j < ent->getNMentions(); j++) {
					if (ent->getMention(j) > lastID)
						lastID = ent->getMention(j);
				}
				std::cout << "Entity " << ent->getID() << " has " << lastID << " as its latest mention\n";
		}
	}
	*/

	bool found_link = false;

	// change sortedEnts to entities to stop sorting!
	while (sortedEnts.length() > 0) {
		OldMaxEntEvent *e = _new OldMaxEntEvent(StatDescLinker::MAX_PREDICATES);
		Entity* prevEntity = sortedEnts.removeLast();


		// now we stop adding after we find a link
		bool stopLinking = false;
		if (_alreadySeenIDs->get(id) != NULL && prevEntity->getID() == *(_alreadySeenIDs->get(id))) {
			e->setOutcome(DescLinkFunctions::OC_LINK);
			DescLinkFunctions::getEventContext(entitySet, ment, prevEntity, e, StatDescLinker::MAX_PREDICATES);
			//if (e->find(DescLinkFunctions::HEAD_WORD_MATCH))
			//{
				_primaryEvents.add(e);
				_primaryModel->addEvent(e, 1);
			//}
			/*else {
				_secondaryEvents.add(e);
				_secondaryModel->addEvent(e, 1);
			}*/
			stopLinking = true;
		}
		else {
			e->setOutcome(DescLinkFunctions::OC_NO_LINK);
			DescLinkFunctions::getEventContext(entitySet, ment, prevEntity, e, StatDescLinker::MAX_PREDICATES);
			//if (e->find(DescLinkFunctions::HEAD_WORD_MATCH))
			//{
				_primaryEvents.add(e);
				_primaryModel->addEvent(e, 1);
			//}
			/*else {
				_secondaryEvents.add(e);
				_secondaryModel->addEvent(e, 1);
			}*/
		}


		//if (stopLinking) {
		//	found_link = true;
		//	break;
		//}
	}

}

void DescriptorLinkerTrainer::train(char* output_file_prefix) {
	_primaryModel->deriveModel(_cutoff, _threshold);
	//_secondaryModel->deriveModel(_cutoff, _threshold);
	printModel(output_file_prefix);
}

void DescriptorLinkerTrainer::writeEvents(const char *file_prefix) {
	char buffer[500];

	SessionLogger::info("SERIF") << "Writing Events file...\n";

	sprintf(buffer, "%s.rawevents", file_prefix);
	UTF8OutputStream eventStream;
	eventStream.open(buffer);

	eventStream << "PRIMARY EVENTS\n";
	while (_primaryEvents.length() > 0) {
		OldMaxEntEvent *e = _primaryEvents.removeLast();
		if (e->getOutcome() == DescLinkFunctions::OC_LINK)
			eventStream << L"link ( ";
		else if (e->getOutcome() == DescLinkFunctions::OC_NO_LINK)
			eventStream << L"no-link ( ";
		//eventStream << e->getOutcome().to_string() << " ( ";
		for (int i = 0; i < e->getNContextPredicates(); i++) {
			/*if (e->getContextPredicate(i).getNSymbols() == 1) {
				Symbol s = e->getContextPredicate(i).getSymbol(0);
				eventStream << s.to_string() << " ";
			}*/
			eventStream << e->getContextPredicate(i).to_string() << " ";
		}
		eventStream << ")\n";
		delete e;
	}
	/*eventStream << "SECONDARY EVENTS\n";
	while (_secondaryEvents.length() > 0) {
		OldMaxEntEvent *e = _secondaryEvents.removeLast();
		if (e->getOutcome() == DescLinkFunctions::OC_LINK)
			eventStream << L"link ( ";
		else if (e->getOutcome() == DescLinkFunctions::OC_NO_LINK)
			eventStream << L"no-link ( ";
		//eventStream << e->getOutcome().to_string() << " ( ";
		for (int i = 0; i < e->getNContextPredicates(); i++) {
			if (e->getContextPredicate(i).getNSymbols() == 1) {
				Symbol s = e->getContextPredicate(i).getSymbol(0);
				eventStream << s.to_string() << " ";
			}
			eventStream << e->getContextPredicate(i).to_string() << " ";
		}
		eventStream << ")\n";
		delete e;
	}*/
}

void DescriptorLinkerTrainer::printModel(const char *file_prefix) {
	char buffer[500];

	SessionLogger::info("SERIF") << "Writing Model file...\n";

	sprintf(buffer, "%s.maxent", file_prefix);
	UTF8OutputStream stream;
	stream.open(buffer);
	_primaryModel->print_to_open_stream(stream);
	//_secondaryModel->print_to_open_stream(stream);
	stream.close();
}
