// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/edt/NameLinkerTrainer/NameLinkerTrainer.h"
#include "Generic/edt/NameLinkFunctions.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/state/TrainingLoader.h"
#include "Generic/state/StateLoader.h"
#include <boost/scoped_ptr.hpp>

NameLinkerTrainer::NameLinkerTrainer(int mode_)
	: MODE(mode_), _uniWriter(2, 100), _priorWriter(2,100), _stopWords(100)
{
	_debugOut.init(Symbol(L"name_link_train_debug"));

	// load stop words
	std::string file = ParamReader::getRequiredParam("stopword_file");
	loadStopWords(file.c_str());
}

void NameLinkerTrainer::train() {
	if (MODE != TRAIN)
		return;

	std::string param = ParamReader::getRequiredParam("name_link_train_source");
	if (param == "state_files")
		trainOnStateFiles();
	else if (param == "aug_parses")
		trainOnAugParses();
	else
		throw UnexpectedInputException("NameLinkerTrainer::train()", 
			"Invalid parameter value for 'name_link_train_source'.  Must be 'state_files' or 'aug_parses'.");

	// we are now set to decode
	MODE = DECODE;
}

void NameLinkerTrainer::trainOnStateFiles() {
	// MODEL FILE
	std::string model_prefix = ParamReader::getRequiredParam("name_linker_model");

	// TRAINING FILE(S)
	std::string file = ParamReader::getParam("training_file");
	if (!file.empty()) {
		loadTrainingDataFromStateFile(file.c_str());
	} else {
		file = ParamReader::getParam("training_file_list");
		if (!file.empty())
			loadTrainingDataFromStateFileList(file.c_str());
		else  
			throw UnexpectedInputException("NameLinkerTrainer::trainOnStateFiles()",
									   "Params `training_file' and 'training_file_list' both undefined");
	}

	writeModels(model_prefix.c_str());
}

void NameLinkerTrainer::trainOnAugParses() {
	// MODEL FILE
	std::string model_prefix = ParamReader::getRequiredParam("name_linker_model");

	std::string file = ParamReader::getParam("training_file");
	if (!file.empty()) {
		loadTrainingDataFromStateFile(file.c_str());
	} else {
		file = ParamReader::getParam("training_file_list");
		if (!file.empty())
			loadTrainingDataFromStateFileList(file.c_str());
		else  
			throw UnexpectedInputException("NameLinkerTrainer::trainOnAugParses()",
									   "Params `training_file' and 'training_file_list' both undefined");
	}

	writeModels(model_prefix.c_str());
}

void NameLinkerTrainer::loadTrainingDataFromStateFileList(const char *listfile) {
	boost::scoped_ptr<UTF8InputStream> fileListStream_scoped_ptr(UTF8InputStream::build(listfile));
	UTF8InputStream& fileListStream(*fileListStream_scoped_ptr);	
	UTF8Token token;

	while (!fileListStream.eof()) {
		fileListStream >> token;
		if (wcscmp(token.chars(), L"") == 0)
			break;
		loadTrainingDataFromStateFile(token.chars());
	}

}

void NameLinkerTrainer::loadTrainingDataFromStateFile(const wchar_t *filename) {
	char state_file_name_str[501];
	StringTransliterator::transliterateToEnglish(state_file_name_str, filename, 500);
	loadTrainingDataFromStateFile(state_file_name_str);
}

void NameLinkerTrainer::loadTrainingDataFromStateFile(const char *filename) {
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


void NameLinkerTrainer::loadTrainingDataFromAugParseFileList(char *listfile) {
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

void NameLinkerTrainer::loadTrainingDataFromAugParseFile(const wchar_t *filename) {
	char parse_file_name_str[501];
	StringTransliterator::transliterateToEnglish(parse_file_name_str, filename, 500);
	loadTrainingDataFromAugParseFile(parse_file_name_str);
}

void NameLinkerTrainer::loadTrainingDataFromAugParseFile(const char *filename) {
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

void NameLinkerTrainer::trainDocument(DocTheory *docTheory) {
	for (int i = 0; i < docTheory->getNSentences(); i++) {
		MentionSet *mentSet = docTheory->getSentenceTheory(i)->getMentionSet();
		for (int j = 0; j < mentSet->getNMentions(); j++) {
			Mention *mention = mentSet->getMention(j);
			if (mention != 0 && mention->getMentionType() == Mention::NAME) {
				if (!mention->getEntityType().isRecognized())
					continue;
				Mention *mentionHead = mention;
				while (mentionHead->getChild() != 0) {
					mentionHead = mentionHead->getChild();
				}
				
				_debugOut << "\tAdding node from item list as " << mentionHead->getEntityType().getName().to_string() << ": " << mentionHead->getNode()->toTextString() << "\n";
				addMention(mentionHead->getNode(),
						   mentionHead->getEntityType().getName());
			}
		}
	}
}

void NameLinkerTrainer::trainDocument(CorefDocument *document) {
	for (int i = 0; i < document->corefItems.length(); i++) {
		_debugOut << "Item " << i << " of " << document->corefItems.length() << "\n";
		if (document->corefItems[i]->mention != 0 && document->corefItems[i]->mention->mentionType == Mention::NAME) {
			_debugOut << "\tAdding node from item list as " << document->corefItems[i]->mention->getEntityType().getName().to_string() << ": " << document->corefItems[i]->node->toTextString() << "\n";
			addMention(document->corefItems[i]->node,
					   document->corefItems[i]->mention->getEntityType().getName());
		}
	}
}

void NameLinkerTrainer::addMention(const SynNode *node, Symbol type) {
	Symbol priorArr[2];
	Symbol uniArr[2];
	// Gather EDT type, words, left word, and functional parent
	priorArr[0] = SymbolConstants::nullSymbol;
	priorArr[1] = uniArr[0] = type;
	_priorWriter.registerTransition(priorArr);
	// the words
	Symbol terms[100], lexicalItems[100];
	int termSize = node->getTerminalSymbols(terms, 100);
	int nLexicalItems = NameLinkFunctions::getLexicalItems(terms, termSize, lexicalItems, 100);

	for (int j = 0; j < nLexicalItems; j++) {
		if (_stopWords.lookup(lexicalItems[j]))
			continue;
		uniArr[1] = lexicalItems[j];
		// TODO: get word *classes*?
		_uniWriter.registerTransition(uniArr);
	}

}


void NameLinkerTrainer::writeModels(const char* model_prefix) {
	char file[501];

	sprintf(file,"%s.uni", model_prefix);
	UTF8OutputStream uniStream(file);
	if (uniStream.fail()) {
		throw UnexpectedInputException("NameLinkerTrainer::writeModels()",
									   "Could not open model file(s) for writing");
	}
	_uniWriter.writeModel(uniStream);
	uniStream.close();

	sprintf(file,"%s.prior", model_prefix);
	UTF8OutputStream priorStream(file);
	if (priorStream.fail()) {
		throw UnexpectedInputException("NameLinkerTrainer::writeModels()",
									   "Could not open model file(s) for writing");
	}
	_priorWriter.writeModel(priorStream);
	priorStream.close();

	// inverse lambdas
	sprintf(file,"%s.lambdas", model_prefix);
	UTF8OutputStream genLambdaStream(file);
	if (genLambdaStream.fail()) {
		throw UnexpectedInputException("NameLinkerTrainer::writeModels()",
									   "Could not open model file(s) for writing");
	}
	_uniWriter.writeLambdas(genLambdaStream, true);
	genLambdaStream.close();
}


void NameLinkerTrainer::loadStopWords(const char *file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);
	int size = -1;
	stream >> size;
	if (size < 0)
		throw UnexpectedInputException("NameLinkerTrainer::loadStopWords()",
									   "couldn't read size of stopWord array");

	UTF8Token token;
	for (int i = 0; i < size; i++) {
		stream >> token;
		_stopWords.add(token.symValue());
	}
}





