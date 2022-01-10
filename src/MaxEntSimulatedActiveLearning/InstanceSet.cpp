// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "InstanceSet.h"
#include "state/TrainingLoader.h"
#include <boost/scoped_ptr.hpp>

vector<RelationInstance*>::iterator InstanceSet::instancesBegin(DTRelSentenceInfo* info) {
	if (!contains(info))
		throw UnexpectedInputException("InstanceSet::instancesBegin", "SentenceInfo doesn't belong to this InstanceSet.");
	InfoToInstancesMap::iterator infoIt = _sentenceInfoToRelations.find(info);
	return ((*infoIt).second).begin();
}

vector<RelationInstance*>::iterator InstanceSet::instancesEnd(DTRelSentenceInfo* info) {
	if (!contains(info))
		throw UnexpectedInputException("InstanceSet::instancesBegin", "SentenceInfo doesn't belong to this InstanceSet.");
	InfoToInstancesMap::iterator infoIt = _sentenceInfoToRelations.find(info);
	return ((*infoIt).second).end();
}

RelationInstance* InstanceSet::getInstance(Symbol id) const {
	UidToInstancesMap::iterator it = _uidToInstance.find(id);
	if (it == _uidToInstance.end())
		return 0;
	else
		return (*it).second;
}

/* This is a sanity check function to ensure that there is a non-null _tagSet data member.  In 
   theory one should have been set right after creation but as it is there is no way to enforce 
   this.  Might be worth making the constructor take it as an argument, only that makes it hard
   to have an actual InstanceSet member (as opposed to an InstanceSet*). */
void InstanceSet::ensureTagSet() const {
	if (_tagSet == 0) {
		printf("InstanceSet::ensureTagset():  A DTTagSet object is required but none has been set.\n");
		throw UnexpectedInputException(
			"InstanceSet::ensureTagset",
			"No DTTagSet instance set on this InstanceSet.");
	}
}

RelationInstance* InstanceSet::loadAnnotatedInstance(UTF8InputStream& fileStream, const InstanceSet& sourceSet) {
	ensureTagSet();
	UTF8Token token;
	fileStream >> token;
	RelationInstance* instance = sourceSet.getInstance(token.symValue());
	if (instance == NULL) {
		printf("Relation instance %s not found in sourceSet.\n", token.symValue().to_debug_string());
		throw UnexpectedInputException(
			"InstanceSet::loadRelationInstance",
			"Relation instance not found in sourceSet.");
	}

	std::wstring buffer;
	for (int j = 0; j < 11; j++)
		fileStream.getLine(buffer);  // skip most of the info, which is recoverable from the ID
	fileStream >> token;
	Symbol relType = token.symValue();
	instance->setAnnotationSymbol(relType);
	if (relType == Symbol(L"BAD")) {  // BAD PRONOUN or BAD EDT
		printf("loadAnnotatedInstance(): %s invalid (bad edt).\n", 
			instance->getID().to_debug_string());
		instance->invalidate();
		fileStream.getLine(buffer);  // finish up the BAD X line

	} else if (_tagSet->getTagIndex(relType) == -1) {  // also covers SAME and METONYMY
		printf("loadAnnotatedInstance(): %s invalid (type '%s' not found).\n", 
			instance->getID().to_debug_string(), relType.to_debug_string());
		instance->invalidate();
	}

	fileStream.getLine(buffer);  // reversed 
	bool reversedInAnno = (buffer.substr(0, 8) == L"reversed");
	if ((reversedInAnno && !instance->isReversed()) ||
		(!reversedInAnno && instance->isReversed())) { 
			// actually, since we're only using a maxent model, we don't really care about the order
			/* 
			throw UnexpectedInputException("InstanceSet::addDataFromAnnotationFile",
			"Reversed relations not implemented.  You should fix this.");
			*/
	}
	return instance;
}

int InstanceSet::addDataFromAnnotationFile(const wchar_t * const fileName,
										   const InstanceSet& sourceSet) {
	boost::scoped_ptr<UTF8InputStream> fileStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& fileStream(*fileStream_scoped_ptr);
	fileStream.open(fileName);
	if (fileStream.fail()) {
		throw UnexpectedInputException(
			"InstanceSet::addDataFromAnnotationFile",
			"Unable to open file.");
	}

	int numInstances = -1;
	fileStream >> numInstances;
	if (numInstances < 1) {
		printf("InstanceSet::addDataFromAnnotationFile():  First line of annotation file must be the number of instances.\n");
		throw UnexpectedInputException(
			"InstanceSet::addDataFromAnnotationFile",
			"First line must contain the number of instances in the annotation file.");
	}
	for (int i = 0; i < numInstances; i++) {
		RelationInstance* instance = loadAnnotatedInstance(fileStream, sourceSet);
		addInstance(instance);
	}
	fileStream.close();
	return numInstances;
}

void InstanceSet::writeToAnnotationFile(const wchar_t * const fileName) const {
	UTF8OutputStream outputStream;
	outputStream.open(fileName);
	outputStream << static_cast<int>(_instances.size()) << L"\n";
	for (vector<RelationInstance*>::const_iterator instIt = _instances.begin();
		instIt != _instances.end(); 
		++instIt)
	{
		outputStream << makeHYAString(*instIt);
	}
	outputStream.close();
}

/*
	The first line of each annotation file should contain the number of instances in the file.
	The format for each individual relation instance should be:

	docno
	full sentence text
	EDT start offset of first mention (*)
	EDT end offset of first mention (*)
	start offset of first mention relative to the beginning of the sentence
	length (in chars) of first mention
	mention type of first mention
	EDT start offset of second mention (*)
	EDT end offset of second mention (*)
	start offset of second mention relative to the beginning of the sentence
	length (in chars) of second mention
	mention type of second mention
	relation type (set to "---------" if the type is unknown)
	"reversed" if the relation is reversed, blank otherwise

	(*) these values are not actually used by the HYA tool as far as I can tell.
	*/
std::wstring InstanceSet::makeHYAString(RelationInstance const * const instance) {
	const TokenSequence* tokens = instance->getSentenceInfo()->tokenSequences[0];

/*	const SynNode* firstNode = instance->getFirstMention()->getNode();
	const SynNode* secondNode = instance->getSecondMention()->getNode();

	// these are all inclusive; they should be the same numbers that appear in the CA file
	int edt_start1 = tokens->getToken(firstNode->getStartToken())->getStartOffset();
	int edt_end1 = tokens->getToken(firstNode->getEndToken())->getEndOffset();
	int edt_start2 = tokens->getToken(secondNode->getStartToken())->getStartOffset();
	int edt_end2 = tokens->getToken(secondNode->getEndToken())->getEndOffset();

	// here start and end are NOT inclusive as they will be used only to print out to the HYA format
	int start1 = -1;
	int end1 = -1;
	int start2 = -1;
	int end2 = -1;  

	int index = 0;
	for (int t = 0; t < tokens->getNTokens(); t++) {

		const Token* token = tokens->getToken(t);
		const wchar_t* str = token->getSymbol().to_string();

		if (t == firstNode->getStartToken())
			start1 = index;
		if (t == firstNode->getEndToken())
			end1 = index + static_cast<int>(wcslen(str));
		if (t == secondNode->getStartToken())
			start2 = index;
		if (t == secondNode->getEndToken())
			end2 = index + static_cast<int>(wcslen(str));

		index += static_cast<int>(wcslen(str));
	}
*/

	const SynNode* head1 = PotentialTrainingRelation::getEDTHead(instance->getFirstMention(), 
														  		 instance->getMentionSet());
	const SynNode* head2 = PotentialTrainingRelation::getEDTHead(instance->getSecondMention(), 
														  		 instance->getMentionSet());

	int edt_start1 = tokens->getToken(head1->getStartToken())->getStartOffset();
	int edt_start2 = tokens->getToken(head2->getStartToken())->getStartOffset();
	int edt_end1 = tokens->getToken(head1->getEndToken())->getEndOffset();
	int edt_end2 = tokens->getToken(head2->getEndToken())->getEndOffset();

	int start1, end1, start2, end2;  // start and end are inclusive
	int index = 0;
	for (int t = 0; t < tokens->getNTokens(); t++) {

		const Token* token = tokens->getToken(t);
		const wchar_t* str = token->getSymbol().to_string();

		if (t == head1->getStartToken())
			start1 = index;
		if (t == head1->getEndToken())
			end1 = index + static_cast<int>(wcslen(str));
		if (t == head2->getStartToken())
			start2 = index;
		if (t == head2->getEndToken())
			end2 = index + static_cast<int>(wcslen(str));

		index += static_cast<int>(wcslen(str));
	}

	std::wstring str = L"";
	wchar_t wc[PARAM_LENGTH];
	str += instance->getID().to_string();
	str += L"\n";
	str += tokens->toStringNoSpaces();
	str += L"\n";
	swprintf(wc, L"%d", edt_start1);
	str += wc;
	str += L"\n";
	swprintf(wc, L"%d", edt_end1);
	str += wc;
	str += L"\n";
	swprintf(wc, L"%d", start1);
	str += wc;
	str += L"\n";
	swprintf(wc, L"%d", end1 - start1);
	str += wc;
	str += L"\n";
	str += instance->getFirstMention()->getEntityType().getName().to_string();
	str += L"\n";
	swprintf(wc, L"%d", edt_start2);
	str += wc;
	str += L"\n";
	swprintf(wc, L"%d", edt_end2);
	str += wc;
	str += L"\n";
	swprintf(wc, L"%d", start2);
	str += wc;
	str += L"\n";
	swprintf(wc, L"%d", end2 - start2);
	str += wc;
	str += L"\n";
	str += instance->getSecondMention()->getEntityType().getName().to_string();
	str += L"\n";
	
	str += instance->getRelationSymbol().to_string();
	str += L"\n";
	if (instance->isReversed()) {
		str += L"reversed\n";
	} else {
		str += L"\n";
	}
	return str;
}

bool InstanceSet::isUsed(RelationInstance* instance) const {
	if (!contains(instance))
		throw UnexpectedInputException("InstanceSet::isUsed", "Instance doesn't belong to this InstanceSet.");
	return (_usedInstances->find(instance->getID())) != _usedInstances->end();
} 

void InstanceSet::setUsed(RelationInstance* instance) {
	if (!contains(instance))
		throw UnexpectedInputException("InstanceSet::isUsed", "Instance doesn't belong to this InstanceSet.");
	_usedInstances->insert(instance->getID());
	_numUsed++;
}

void InstanceSet::writeUsedToFile(const wchar_t * const fileName) const {
	UTF8OutputStream outputStream;
	outputStream.open(fileName);
	if (outputStream.fail()) {
		throw UnexpectedInputException(
			"InstanceSet::writeUsedToFile",
			"Unable to open file.");
	}
	outputStream << _numUsed << L"\n";
	for (Symbol::HashSet::iterator usedIt = _usedInstances->begin();
		usedIt != _usedInstances->end(); 
		++usedIt)
	{
		outputStream << (*usedIt) << L"\n";
	}
	outputStream.close();
}

void InstanceSet::writeUsedToFile(const char * const fileName) const {
	wchar_t wideFileName[MAXLINE];
	mbstowcs(wideFileName, fileName, MAXLINE);
	return writeUsedToFile(wideFileName);
}

void InstanceSet::readUsedFromFile(const wchar_t * const fileName) {
	boost::scoped_ptr<UTF8InputStream> fileStream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& fileStream(*fileStream_scoped_ptr);
	fileStream.open(fileName);
	if (fileStream.fail()) {
		throw UnexpectedInputException(
			"InstanceSet::addDataFromAnnotationFile",
			"Unable to open file.");
	}

	int numInstances = 0;
	fileStream >> numInstances;
	for (int i = 0; i < numInstances; i++) {
		UTF8Token token;
		fileStream >> token;
		RelationInstance* instance = getInstance(token.symValue());
		if (instance == NULL) {
			printf("Relation instance %s not found in this sourceSet; skipping it.\n", token.symValue().to_debug_string());
		} else {
			setUsed(instance);			
		}
	}	
	fileStream.close();
}

void InstanceSet::readUsedFromFile(const char * const fileName) {
	wchar_t wideFileName[MAXLINE];
	mbstowcs(wideFileName, fileName, MAXLINE);
	readUsedFromFile(wideFileName);
}

bool InstanceSet::contains(RelationInstance* instance) const {
	if (instance == 0)
		return false;
	return (_uidToInstance.find(instance->getID()) != _uidToInstance.end());
}

bool InstanceSet::contains(DTRelSentenceInfo* info) const {
	if (info == 0)
		return false;
	return (_sentenceInfoToRelations.find(info) != _sentenceInfoToRelations.end());
}

void InstanceSet::deleteObjects() {
	for (InfoToInstancesMap::iterator infoIt = _sentenceInfoToRelations.begin();
		infoIt != _sentenceInfoToRelations.end(); 
		++infoIt) 
	{
		DTRelSentenceInfo* info = (*infoIt).first;
		delete info;
	}
	for (vector<RelationInstance*>::iterator instIt = _instances.begin();
		instIt != _instances.end(); 
		++instIt)
	{

		RelationInstance* inst = (*instIt);
		delete inst;
	}
	_sentences.empty();
	_instances.empty();
	_uidToInstance.remove_all();
	_sentenceInfoToRelations.remove_all();
	if (_usedInstances)
		delete _usedInstances;
	_usedInstances = _new Symbol::HashSet(1000);
	_numUsed = 0;
}

vector<DTRelSentenceInfo*>::iterator InstanceSet::sentencesBegin() {
	return _sentences.begin();	
}

vector<DTRelSentenceInfo*>::iterator InstanceSet::sentencesEnd() {
	return _sentences.end();	
}

vector<RelationInstance*>::iterator InstanceSet::begin() {
	return _instances.begin();	
}

vector<RelationInstance*>::iterator InstanceSet::end() {
	return _instances.end();
}

RelationInstance*& InstanceSet::operator[](unsigned n) {
	return _instances[n];
}

const RelationInstance* InstanceSet::operator[](unsigned n) const {
	return const_cast<const RelationInstance*>(_instances[n]);
}

size_t InstanceSet::size() const {
	return _instances.size();
}

void InstanceSet::addData(RelationInstance* instance, bool includeWholeSentence) {
	if (includeWholeSentence) {
		addAllInstances(instance->getSentenceInfo(), instance->getDocID().to_string());
	} else {
		addInstance(instance);
	}
}

// in charge of the _sentenceInfos vector as well as _sentenceInfoToRelations hashmap
void InstanceSet::addSentenceInfo(DTRelSentenceInfo* info) {
	if (contains(info))
		return;
	_sentences.push_back(info);
	if (_sentenceInfoToRelations.find(info) == _sentenceInfoToRelations.end())  // should be the case!
		_sentenceInfoToRelations[info] = vector<RelationInstance*>();
}

// enforces the SentenceInfo being in this InstanceSet, and adds the instance
void InstanceSet::addInstance(RelationInstance* instance) {
	if (contains(instance))
		return;

	DTRelSentenceInfo* info = instance->getSentenceInfo();
	if (!contains(info))
		addSentenceInfo(info);

	_instances.push_back(instance);
	_uidToInstance[instance->getID()] = instance;
	_sentenceInfoToRelations[info].push_back(instance);
}

/* this function sets up all four vectors and hashmaps appropriately */
void InstanceSet::addAllInstances(DTRelSentenceInfo *info, Symbol docID) {
	addSentenceInfo(info);

	RelationObservation* observation = _new RelationObservation();
	observation->resetForNewSentence(info);
	const MentionSet *mset = info->mentionSets[0];
	int nmentions = mset->getNMentions();

	for (int i = 0; i < nmentions; i++) {
		if (!validMention(mset->getMention(i)))
			continue;
		for (int j = i + 1; j < nmentions; j++) {
			if (!validMention(mset->getMention(j))
				||
				!RelationUtilities::validRelationArgs(mset->getMention(i), mset->getMention(j)))
				continue;

			observation->populate(i, j);

			if (_tagSet) {  // if no _tagSet, we just don't do error checking
				int correct_answer = _tagSet->getTagIndex(info->relSets[0]->getRelation(i,j));
				if (correct_answer == -1) {
					char error[PARAM_LENGTH];
					sprintf(error, "unknown relation type in training: %s", 
						info->relSets[0]->getRelation(i,j).to_debug_string());
					throw UnexpectedInputException("MaxEntSALTrainer::trainSentence()", error);
				}
			}
			addInstance(_new RelationInstance(i, j, info, docID));	
		}
	}
	delete observation;
}

int InstanceSet::addDataFromSerifFileList(const wchar_t * const filelist, int beamWidth) {
	ensureTagSet();
	int previousSize = static_cast<int>(size());
	TrainingLoader *trainingLoader = _new TrainingLoader(filelist);

	for (int i = 0; i < trainingLoader->getNSentences(); i++) {
		DTRelSentenceInfo *newInfo = _new DTRelSentenceInfo(1);
		SentenceTheory *theory = trainingLoader->getNextSentenceTheory();
		newInfo->tokenSequences[0] = theory->getTokenSequence();
		newInfo->tokenSequences[0]->gainReference();
		newInfo->parses[0] = theory->getPrimaryParse();
		newInfo->parses[0]->gainReference();
		//have to add a reference to the np chunk theory, because it may be 
		//the source of the parse, if the reference isn't gained, the parse 
		//will be deleted when result is deleted
		newInfo->npChunks[0] = theory->getNPChunkTheory();
		if(newInfo->npChunks[0] != 0) {
			newInfo->npChunks[0]->gainReference();
		}
		newInfo->mentionSets[0] = theory->getMentionSet();
		newInfo->mentionSets[0]->gainReference();
		newInfo->propSets[0] = theory->getPropositionSet();
		newInfo->propSets[0]->gainReference();
		newInfo->propSets[0]->fillDefinitionsArray();
		newInfo->relSets[0] = 
			_new DTRelationSet(newInfo->mentionSets[0]->getNMentions(), 
			theory->getRelMentionSet(), _tagSet->getNoneTag());  // XXX what if no tagset??
		addAllInstances(newInfo, theory->getDocID());  // takes care of adding instances to _instances
	}
	delete trainingLoader;
	return static_cast<int>(size()) - previousSize;
}

int InstanceSet::addDataFromSerifFileList(const char * const filelist, int beamWidth) {
	wchar_t wideFilelist[MAXLINE];
	mbstowcs(wideFilelist, filelist, MAXLINE);
	return addDataFromSerifFileList(wideFilelist, beamWidth);
}

void InstanceSet::writeToAnnotationFile(const char * const fileName) const { 
	wchar_t wideFilename[MAXLINE];
	mbstowcs(wideFilename, fileName, MAXLINE);
	writeToAnnotationFile(wideFilename);
}

int InstanceSet::addDataFromAnnotationFile(const char * const fileName, const InstanceSet& sourceSet) {
	wchar_t wideFilename[MAXLINE];
	mbstowcs(wideFilename, fileName, MAXLINE);
	return addDataFromAnnotationFile(wideFilename, sourceSet);
}

void InstanceSet::setTagset(DTTagSet* tagset) {
	if (_tagSet)
		throw UnexpectedInputException("InstanceSet::setTagset", "This InstanceSet already has a DTTagSet.");
	if (tagset == 0)
		throw UnexpectedInputException("InstanceSet::setTagset", "Provided DTTagSet was null.");
	_tagSet = tagset;
}

int InstanceSet::getNumUsed() const {
	return _numUsed;
}

InstanceSet::InstanceSet(DTTagSet *tagSet)
: _tagSet(tagSet), _usedInstances(_new Symbol::HashSet(1000)), _numUsed(0) {  }

InstanceSet::~InstanceSet() {
	delete _usedInstances;
}

void InstanceSet::saveUsedInstances(const wchar_t * const fileName) const {
	char char_str[PARAM_LENGTH + 1];
	StringTransliterator::transliterateToEnglish(char_str, fileName, PARAM_LENGTH);
	wprintf(L"Saving used instances to %s\n", fileName);
	StateSaver* stateSaver = _new StateSaver(char_str); 

	wchar_t state_tree_name[PARAM_LENGTH];
	wcscpy(state_tree_name, L"InstanceSet");
	stateSaver->beginStateTree(state_tree_name);
	stateSaver->beginList(L"InstanceSet::UsedInstances");
	stateSaver->saveInteger(_numUsed);
	for (Symbol::HashSet::iterator iter = _usedInstances->begin();
		iter != _usedInstances->end(); 
		++iter) 
	{
		stateSaver->saveSymbol(*iter);
	}
	stateSaver->endList();
	delete stateSaver;
}

/** replaces the used set */
void InstanceSet::loadUsedInstances(const wchar_t * const fileName) {
	if (_usedInstances) 
		delete _usedInstances;
	_usedInstances = _new Symbol::HashSet(1000);

	char char_str[PARAM_LENGTH + 1];
	StringTransliterator::transliterateToEnglish(char_str, fileName, PARAM_LENGTH);
	StateLoader* stateLoader = _new StateLoader(char_str); 

	wchar_t state_tree_name[PARAM_LENGTH];
	wcscpy(state_tree_name, L"InstanceSet");
	stateLoader->beginStateTree(state_tree_name);
	stateLoader->beginList(L"InstanceSet::UsedInstances");
	int numUsed = stateLoader->loadInteger();
	for (int i = 0; i < numUsed; i++) {
		Symbol symbol = stateLoader->loadSymbol();
		_usedInstances->insert(symbol);
	}
	stateLoader->endList();
	delete stateLoader;
}

void InstanceSet::clear() {
	// NOTE that this function leaves the DTTagSet unchanged

	_numUsed = 0;
	_sentenceInfoToRelations.remove_all();
	_uidToInstance.remove_all();
	_instances.clear();
	_sentences.clear();
	if (_usedInstances)
		delete _usedInstances;
	_usedInstances = _new Symbol::HashSet(1000);
}
