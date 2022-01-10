// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "MaxEntSALRelationTrainer.h"

/** General Notes
 *
 * getXXXParam:  All the getXXXParam functions are from Utilities.h; they simply fetch the named
 * parameter or throw an exception if it doesn't exist.
 *
 * model update:  Weights are written out whenever _decoder->deriveModel() is called because
 * new training has been added.  If training is added in a subfunction, deriveModel() should be
 * called from the caller of the subfunction UNLESS that caller is not in this class.  (See
 * doActiveLearning() vs. trainOnSeed().)
 *
 * iteration:  Only the following functions modify _iteration:
 *		- loadWeights() sets _iteration to the model number it loads
 *		- doActiveLearning() increments _iteration by 1 after every run.
 *
 * filenames:  The iteration number in the different filenames is interpreted as follows:
 *		- model n:  The model resulting from n iterations of Active Learning, with 0 being the model
 *		  that was trained on seed data
 *
 *		- request file n:  These are the relation instances that were selected as most confusing
 *		  when model n was run over the active learning pool.
 *
 *		- used file n:  Contains the id's of the relation instances that are in the active
 *		  learning pool, but which have already been included in the training of model n.  Relation
 *		  instances are NOT marked used just because they're written out to a request file.
 *		  (Note: could include the id's of relation instances in the seed data too, for consistency
 *		  and possible error catching.  Currently not implemented.)  Model 0 will be the model
 *		  trained from seed so it will not have any used instances written out.
 *
 *		- devtest file n:  The test set decoded with model n.
 **/


void MaxEntSALRelationTrainer::doActiveLearning(bool realAL, int modelNum) {
	if (realAL)
		printf("MaxEntSALRelationTrainer::doActiveLearning():  Doing REAL AL\n");
	else
		printf("MaxEntSALRelationTrainer::doActiveLearning():  Doing SIMULATED AL\n");

	printf("MaxEntSALRelationTrainer::doActiveLearning():  Reading seed instances from file...\n");
	addSeedSentencesFromFileList();
	printf("MaxEntSALRelationTrainer::doActiveLearning():  Training on seed instances...\n");
	trainOnSeed(modelNum);  // includes deriveModel()
	printf("MaxEntSALRelationTrainer::doActiveLearning():  Reading active learning sentences from file...\n");
	addActiveLearningSentencesFromFileList();
	printf("MaxEntSALRelationTrainer::doActiveLearning(): %d instances in AL pool.\n", _activeLearningPool.size());

	if (ParamReader::getRequiredTrueFalseParam("maxent_sal_random")) {
		printf("MaxEntSALRelationTrainer::doActiveLearning():  Doing RANDOM active learning for baseline\n");
		srand(42);
	}

	_iteration = modelNum;
	//int numIterations = realAL ? 1 : _params._numALIterations;
	int numIterations = _params._numALIterations;
	if (realAL && numIterations != 1)
		printf("MaxEntSALRelationTrainer::doActiveLearning(): WARNING: REAL AL iterations not set to 1\n");
	for (int i = 1; i <= numIterations; i++) {
		if (realAL && _iteration >= 0) {
			loadAnnotatedRequests(_iteration);
			loadUsed(_iteration);
			printf("MaxEntSALRelationTrainer::doActiveLearning():  Single real AL iteration\n");
		} else {
			printf("MaxEntSALRelationTrainer::doActiveLearning():  AL iteration %d of %d\n", i, _params._numALIterations);
		}

		int reqAdded = addRequestsToTraining();
		if (reqAdded > 0) {
			printf("MaxEntSALRelationTrainer::doActiveLearning():  Added %d requests to training\n", reqAdded);
		}
		_decoder->deriveModel(_params._pruning);
		_iteration++;
		openDebugStream(_iteration);
		int numDone = selectRequests();
		printRequests(_iteration);
		printUsed(_iteration);
		writeWeights(_iteration);
		if (numDone < 0) // indicates we ran out of instances
			break;
		closeDebugStream();
	}
	_params.printParameters(cout);
	_stats.printInstanceStats(cout);
	_historyStream.close();
}

// fills up _annoRequests
// returns its size, or -1 if it couldn't find _numActiveToAdd instances
// model must be trained
int MaxEntSALRelationTrainer::selectRequests() {
	decodeALArray();  // decodes each unused instance in _activeLearningPool and assigns it a score which is its confusion
	modifyDecodingScores();
	sort(_activeLearningPool.begin(), _activeLearningPool.end(), HYInstanceSet::lessthanRelInstancePtr());

	// add the first _numActiveToAdd instances to training
	_annoRequests.clear();
	for (vector<HYRelationInstance*>::iterator iter = _activeLearningPool.begin();
		(static_cast<int>(_annoRequests.size()) < _params._numActiveToAdd) && (iter < _activeLearningPool.end());
		++iter)
	{
		HYRelationInstance* instance = *iter;
		if (_params.DEBUG) 
			printDebugInfo(instance, _debugStream);
		if (_activeLearningPool.isUsed(instance)) {  // ran out of instances
			printf("MaxEntSALRelationTrainer::selectRequests():  Ran out of instances after %d\n",
				_stats.numTrainingTotal());
			return -1;
		}
		_annoRequests.addInstance(instance);
	}
	return static_cast<int>(_annoRequests.size());
}

// assumes _finishedRequests is filled up with instances read from a human-annotated file
int MaxEntSALRelationTrainer::addRequestsToTraining() {
	int added = 0;

	for (vector<HYRelationInstance*>::iterator iter = _finishedRequests.begin();
		iter < _finishedRequests.end();
		++iter)
	{
		HYRelationInstance* instance = *iter;
		if (!instance->isValid()) {
			printf("%s is invalid; marking used but not adding to training.\n", instance->getID().to_debug_string());
			_activeLearningPool.setUsed(instance);

			// make all the other relation instances that contain these mentions used also
			for (vector<HYRelationInstance*>::iterator iter = _activeLearningPool.instancesBegin(instance->getSentenceInfo());
				iter < _activeLearningPool.instancesEnd(instance->getSentenceInfo());
				++iter) {

					HYRelationInstance* inst = *iter;
					if ((inst->getFirstIndex() == instance->getFirstIndex()) ||
						(inst->getSecondIndex() == instance->getFirstIndex()) ||
						(inst->getFirstIndex() == instance->getSecondIndex()) ||
						(inst->getSecondIndex() == instance->getSecondIndex())) {

							inst->invalidate();
							_activeLearningPool.setUsed(inst);
						}
				}
			continue;  // we don't add this instance to training
		}

		DTRelSentenceInfo* sentenceInfo = instance->getSentenceInfo();
		int index1 = instance->getFirstIndex();
		int index2 = instance->getSecondIndex();
		Symbol relation = instance->getRelationSymbol();
		_observation->resetForNewSentence(sentenceInfo);
		_observation->populate(index1, index2);

		_decoder->addToTraining(_observation, _tagSet->getTagIndex(relation));
		added++;
		_activeLearningPool.setUsed(instance);
		if (relation == sentenceInfo->relSets[0]->getNoneSymbol()) {  // for NONE observations, there may be a limit on how many we can add
			_stats.addedNone();
		} else {
			_stats.addedNonNone();
		}
	}
	return added;
}

void MaxEntSALRelationTrainer::decodeALArray() {
	// XXX note also that we can stop if we find _numActiveToAdd with a margin of 0
	int const MAX = 999999;

	// decode and set margins
	for (int i = 0; i < (int) _activeLearningPool.size(); i++) {
		HYRelationInstance* instance = _activeLearningPool[i];
		if (_activeLearningPool.isUsed(instance)) {
			instance->_margin = numeric_limits<double>::max();
		} else {
			if (ParamReader::getRequiredTrueFalseParam("maxent_sal_random")) {
				instance->_margin = rand();
			} else {
				_observation->resetForNewSentence(instance->getSentenceInfo());
				_observation->populate(instance->getFirstIndex(), instance->getSecondIndex());
				_decoder->decodeToDistribution(_observation, _probabilities, _probSize);
				instance->_margin = getMargin(_probabilities, _probSize);
#if 0
				if (_params.DEBUG) 
					printDebugInfo(instance, _debugStream);
				// let's see what kind of scores we're getting
				printf("Decoded probabilities for %s:\n", instance->getID().to_debug_string());
				for (int i = 0; i < _probSize; i++) {
					printf("%f\t", _probabilities[i]);
				}
				printf("\n");
#endif
			}
		}
	}
}

void MaxEntSALRelationTrainer::printDebugInfo(HYRelationInstance *instance,
											  UTF8OutputStream& stream) 
{
	_observation->resetForNewSentence(instance->getSentenceInfo());
	_observation->populate(instance->getFirstIndex(), instance->getSecondIndex());

	int best_tag;
	_decoder->decodeToDistribution(_observation, _probabilities, _probSize, &best_tag);
	
	int best = 0;
	int second_best = 0;
	int third_best = 0;
	double best_score = -100000;
	double second_best_score = -100000;
	double third_best_score = -100000;
	for (int i = 0; i < _probSize; i++) {
		if (_probabilities[i] > best_score) {
			third_best = second_best;
			third_best_score = second_best_score;
			second_best = best;
			second_best_score = best_score;
			best = i;
			best_score = _probabilities[i];
		} else if (_probabilities[i] > second_best_score) {
			third_best = second_best;
			third_best_score = second_best_score;
			second_best = i;			
			second_best_score = _probabilities[i];
		} else if (_probabilities[i] > third_best_score) {
			third_best = i;
			third_best_score = _probabilities[i];
		} 
	}
	stream << L"Decoded probabilities for " << instance->getID().to_debug_string() << L"<br>\n";
	stream << _tagSet->getTagSymbol(best).to_string() << L": " << _probabilities[best] << L"<br>\n";
	stream << "<font size=1>\n";
	_decoder->printHTMLDebugInfo(_observation, best, stream);
	stream << "</font>\n";
	stream << _tagSet->getTagSymbol(second_best).to_string() << L": " << _probabilities[second_best] << L"<br>\n";
	stream << "<font size=1>\n";
	_decoder->printHTMLDebugInfo(_observation, second_best, stream);
	stream << "</font>\n";
	//stream << _tagSet->getTagSymbol(third_best).to_string() << L": " << _tagScores[third_best] << L"<br>\n";
	//stream << "<font size=1>\n";
	//_model->printHTMLDebugInfo(_observation, third_best, stream);
	//stream << "</font>\n";
	stream << L"<br>\n";
}

void MaxEntSALRelationTrainer::modifyDecodingScores() {
	// XXX blank for now
}

void MaxEntSALRelationTrainer::printRequests(int modelNum) const {
	char buffer[PARAM_LENGTH];
	sprintf(buffer, "%s.%d.relsent", _params._annotation_ready_file, modelNum);
	printf("Writing annotation requests to %s\n", buffer);
	_annoRequests.writeToAnnotationFile(buffer);
}

void MaxEntSALRelationTrainer::printUsed(int modelNum) const {
	char buffer[PARAM_LENGTH];
	sprintf(buffer, "%s.%d.used", _params._model_file, modelNum);
	printf("Writing set of used relation instances to %s\n", buffer);
	_activeLearningPool.writeUsedToFile(buffer);
}

void MaxEntSALRelationTrainer::loadUsed(int modelNum)  {
	char buffer[PARAM_LENGTH];
	sprintf(buffer, "%s.%d.used", _params._model_file, modelNum);
	printf("Loading set of used relation instances from %s\n", buffer);
	_activeLearningPool.readUsedFromFile(buffer);
}

void MaxEntSALRelationTrainer::loadAnnotatedRequests(int modelNum) {
	char buffer[PARAM_LENGTH];
	_finishedRequests.clear();

	// we have to load all previous annotation files because only they have the correct relation type
	for (int model = 0; model <= modelNum; model++) {
		sprintf(buffer, "%s.%d.relsent.done", _params._annotation_ready_file, model);
		printf("Reading finished annotation from %s\n", buffer);
		_finishedRequests.addDataFromAnnotationFile(buffer, _activeLearningPool);
	}
}

double MaxEntSALRelationTrainer::getMargin(double* array, int size) {
	if (size <= 1)
		throw UnexpectedInputException("MaxEntSALRelationTrainer::getMargin", "Array has fewer than two elements!");
	sort(array, array + size);
	return array[size-1] - array[size-2];
}

bool MaxEntSALRelationTrainer::validMention(Mention* m) {
	if (!m->isOfRecognizedType() ||
		m->getMentionType() == Mention::NONE ||
		m->getMentionType() == Mention::APPO ||
		m->getMentionType() == Mention::LIST)

		return false;

	return true;
}

// sets up weights and new decoder
void MaxEntSALRelationTrainer::loadWeights(int modelNum) {
	char buffer[PARAM_LENGTH];
	sprintf(buffer, "%s.%d", _params._model_file, modelNum);
	printf("Reading weights from %s\n", buffer);
	if (_weights)
		delete _weights;
	_weights = _new DTFeature::FeatureWeightMap(50000);  // XXX is this necessary?
	DTFeature::readWeights(*_weights, buffer, P1RelationFeatureType::modeltype);
	if (_decoder)
		delete _decoder;
	_decoder = _new MaxEntModel(_tagSet, _featureTypes, _weights);
	_decoder->deriveModel(_params._pruning);
	_iteration = modelNum;
}

void MaxEntSALRelationTrainer::devTest(int modelNum) {
	if (_devTestSet.size() == 0)
		addDevtestSentencesFromFileList();

	// -1 for modelNum means use current weights
	if (modelNum >= 0) {
		loadWeights(modelNum);  // this also sets _iteration to modelNum
	}

	char buffer[PARAM_LENGTH];
	sprintf(buffer, "%s.%d.html", _params._output_file, modelNum);
	_devTestStream.open(buffer);

	int i = 0;
	for (vector<DTRelSentenceInfo*>::iterator iter = _devTestSet.sentencesBegin();
		iter < _devTestSet.sentencesEnd();
		++iter)
	{

		DTRelSentenceInfo* info = *iter;
		if (i % 10 == 0)
			cerr << i << "\r";
		walkThroughSentence(info, _devTestSet, i, DEVTEST);
		i++;
	}

	_stats.printPerformanceStats(_devTestStream);
}

void MaxEntSALRelationTrainer::trainOnSeed(int modelNum) {
	//char train_vectors[PARAM_LENGTH];
	//sprintf(train_vectors, "%s.%d.vectors", _params._train_vector_file, modelNum+1);

	_decoder = _new MaxEntModel(_tagSet, _featureTypes, _weights,
		_params._mode, _params._percent_held_out, _params._max_iterations, _params._variance,
		_params._likelihood_delta, _params._stop_check_freq,
		_params._train_vector_file, _params._test_vector_file);
		//train_vectors, _params._test_vector_file);

	if (_params._numTrainingToAdd < 0) {
		_params._numTrainingToAdd = static_cast<int> (_initialPool.size());
	}

	int i = 0;
	_stats.setInitialMode(true);
	for (vector<DTRelSentenceInfo*>::iterator iter = _initialPool.sentencesBegin();
		iter < _initialPool.sentencesEnd() && (_stats.numTotal() < maxInstances());
		++iter)
	{
		DTRelSentenceInfo* info = *iter;
		walkThroughSentence(info, _initialPool, i++, TRAIN);
	}
	_stats.setInitialMode(false);
	printf("MaxEntSALRelationTrainer::train(): %d seed instances added to training.\n", _stats.numInitialTotal());
	_decoder->deriveModel(_params._pruning);
}

void MaxEntSALRelationTrainer::walkThroughSentence(DTRelSentenceInfo *sentenceInfo, HYInstanceSet& set, int index, int mode) {
	_observation->resetForNewSentence(sentenceInfo);
	const MentionSet *mset = sentenceInfo->mentionSets[0];
	int nmentions = mset->getNMentions();
	int n_comparisons = 0;

	if (mode == DEVTEST) {
		_devTestStream << "Sentence " << index << ": ";
		_devTestStream << mset->getParse()->toTextString() << "<br>\n";
	}

	for (vector<HYRelationInstance*>::iterator iter = set.instancesBegin(sentenceInfo);
		iter < set.instancesEnd(sentenceInfo);
		++iter) {

			HYRelationInstance* instance = *iter;
			int i = instance->getFirstIndex();
			int j = instance->getSecondIndex();
			_observation->populate(i, j);
			Symbol correct_answer_symbol = instance->getRelationSymbol();
			int correct_answer = _tagSet->getTagIndex(correct_answer_symbol);
			n_comparisons++;
			if (correct_answer == -1) {
				char error[PARAM_LENGTH];
				sprintf(error, "unknown relation type in training: %s",
					correct_answer_symbol.to_debug_string());
				throw UnexpectedInputException("MaxEntSALTrainer::trainSentence()", error);
			}

			if (mode == TRAIN) {
				instance->setAnnotationSymbol(correct_answer_symbol);
				// for seed data, the relation symbol IS the annotated symbol

				if (_stats.numTotal() >= maxInstances())
					return;

				if (!instance->isValid()) {
					char error[PARAM_LENGTH];
					sprintf(error, "%s is invalid; probably this shouldn't happen in seed data.",
						instance->getID().to_debug_string());
					throw UnexpectedInputException("MaxEntSALTrainer::trainSentence()", error);
				}
				if (correct_answer_symbol == sentenceInfo->relSets[0]->getNoneSymbol()) {  // for NONE observations, there may be a limit on how many we can add
					if (_stats.canAddNone(_params._max_none_percent)) {
						_decoder->addToTraining(_observation, correct_answer);
						_stats.addedNone();
					} else {
						_stats.skippedNone();
					}
				} else {
					_decoder->addToTraining(_observation, correct_answer);
					_stats.addedNonNone();
				}

			} else if (mode == DEVTEST) {
				if (_decoder->DEBUG) {
					_decoder->_debugStream << "Sentence " << index << ":" << n_comparisons << "\n";
					_decoder->_debugStream << "RELATION: " << correct_answer_symbol.to_string() << "\n";
					_decoder->_debugStream << "MENTIONS:\n";
					_decoder->_debugStream << _observation->getMention1()->getNode()->toTextString() << L"\n";
					_decoder->_debugStream << _observation->getMention2()->getNode()->toTextString() << L"\n";

					if (DEBUG_HEADS) {
						_decoder->_debugStream << correct_answer_symbol.to_string() << L" ";
						if (sentenceInfo->relSets[0]->hasReversedRelation(i,j)) {
							_decoder->_debugStream << mset->getMention(j)->getNode()->getHeadWord().to_string() << L" ";
							_decoder->_debugStream << mset->getMention(i)->getNode()->getHeadWord().to_string() << L" ";
						}
						else {
							_decoder->_debugStream << mset->getMention(i)->getNode()->getHeadWord().to_string() << L" ";
							_decoder->_debugStream << mset->getMention(j)->getNode()->getHeadWord().to_string() << L" ";
						}
						RelationPropLink *link = _observation->getPropLink();
						if (link !=0 && !link->isEmpty() && !link->isNested()) {
							Proposition *prop = link->getTopProposition();
							if (prop != 0 && prop->getPredSymbol() != Symbol()) {
								_decoder->_debugStream << prop->getPredSymbol().to_string() << L" ";
								_decoder->_debugStream << prop->getPredTypeString(prop->getPredType());
							}
						}
						_decoder->_debugStream << "\n";
					} // end of head debugging
				}  // end of general debugging

				Symbol hypothesis = _decoder->decodeToSymbol(_observation);
				if (correct_answer_symbol != _tagSet->getNoneTag()
					||
					hypothesis != _tagSet->getNoneTag()) {

						_devTestStream << n_comparisons << ": ";
						_devTestStream << mset->getMention(i)->getNode()->getHeadWord().to_string();
						_devTestStream << L" & ";
						_devTestStream << mset->getMention(j)->getNode()->getHeadWord().to_string();
						_devTestStream << L": ";
						_devTestStream << hypothesis.to_string() << L" ";
						if (correct_answer_symbol == hypothesis) {
							_devTestStream << L"<font color=\"red\">CORRECT</font><br>\n";
							_stats._correct++;
						} else if (correct_answer_symbol == _tagSet->getNoneTag()) {
							_devTestStream << L"<font color=\"blue\">SPURIOUS</font><br>\n";
							_stats._spurious++;
						} else if (hypothesis == _tagSet->getNoneTag()) {
							_devTestStream << L"<font color=\"purple\">MISSING (";
							_devTestStream << correct_answer_symbol.to_string();
							_devTestStream << ")</font><br>\n";
							_stats._missed++;
						} else if (correct_answer_symbol != hypothesis) {
							_devTestStream << L"<font color=\"green\">WRONG TYPE (";
							_devTestStream << correct_answer_symbol.to_string();
							_devTestStream << ")</font><br>\n";
							_stats._wrong_type++;
						}
					}
					_devTestStream << "<br>\n";
			} else {
				throw UnexpectedInputException("MaxEntSALRelationTrainer::walkThroughSentence", "mode not understood");
			}
		}
}

void MaxEntSALRelationTrainer::addSeedSentencesFromFileList() {
	_initialPool.deleteObjects();  // XXX this is only correct if no pointers are shared between two HYInstanceSets
	_initialPool.addDataFromSerifFileList(_params._training_filelist, _params._beam_width);
}

void MaxEntSALRelationTrainer::addActiveLearningSentencesFromFileList() {
//	_activeLearningPool.deleteObjects();
	_activeLearningPool.addDataFromSerifFileList(_params._active_pool_filelist, _params._beam_width);
	printf("MaxEntSALRelationTrainer::addActiveLearningSentencesFromFileList():  Active Learning Pool has %d instances\n", _activeLearningPool.size());
}

void MaxEntSALRelationTrainer::addDevtestSentencesFromFileList() {
	_devTestSet.deleteObjects();
	_devTestSet.addDataFromSerifFileList(_params._devtest_filelist, _params._beam_width);
}

void MaxEntSALRelationTrainer::writeWeights(int modelNum) {
	char buffer[PARAM_LENGTH];
	sprintf(buffer, "%s.%d", _params._model_file, modelNum);
	printf("Writing weights to %s\n", buffer);

	UTF8OutputStream out;
	out.open(buffer);

	if (out.fail()) {
		throw UnexpectedInputException("MaxEntSALTrainer::writeWeights()",
			"Could not open model file for writing");
	}

	for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
		iter != _weights->end(); ++iter)
	{
		DTFeature *feature = (*iter).first;

		out << L"((" << feature->getFeatureType()->getName().to_string()
			<< L" ";
		feature->write(out);
		out << L") " << (*(*iter).second) << L")\n";
	}

}

// returns the number of instances the model should have at the end of this iteration
int MaxEntSALRelationTrainer::maxInstances() {
	return _params._numTrainingToAdd + _iteration * _params._numActiveToAdd;
}

void MaxEntSALRelationTrainer::openDebugStream(int modelNum) {
	if (_params.DEBUG) {
		char buffer[PARAM_LENGTH];
		sprintf(buffer, "%s.%d.html", _params._debug_stream_file, modelNum);
		_debugStream.open(buffer);
		if(_debugStream.fail()){
			throw UnexpectedInputException("MaxEntSALTrainer::openDebugSteam()",
				"Can't open debug stream");
		}
	}
}

void MaxEntSALRelationTrainer::closeDebugStream() {
	if (_params.DEBUG) {
		_debugStream.close();
	}
}

MaxEntSALRelationTrainer::~MaxEntSALRelationTrainer() {
	delete _featureTypes;
	delete _tagSet;
	delete _weights;
	delete _decoder;
	delete _observation;
	delete[] _probabilities;
}

MaxEntSALRelationTrainer::MaxEntSALRelationTrainer()
: _featureTypes(0), _tagSet(0), _weights(0), _decoder(0), _probSize(0), 
  _probabilities(0), _iteration(0)
{

	P1RelationFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();
	_observation = _new RelationObservation();

	_params.initialize();

	// TAGSET
	char tag_set_file[PARAM_LENGTH];
	getStringParam("maxent_relation_tag_set_file", tag_set_file, PARAM_LENGTH, "MaxEntSALTrainer::MaxEntSALTrainer()");
	_tagSet = _new DTTagSet(tag_set_file, false, false);
	_activeLearningPool.setTagset(_tagSet);
	_initialPool.setTagset(_tagSet);
	_devTestSet.setTagset(_tagSet);
	_annoRequests.setTagset(_tagSet);
	_finishedRequests.setTagset(_tagSet);

	_probSize = _tagSet->getNTags();
	_probabilities = _new double[_probSize];

	// FEATURES
	char features_file[PARAM_LENGTH];
	getStringParam("maxent_relation_features_file", features_file, PARAM_LENGTH, "MaxEntSALTrainer::MaxEntSALTrainer()");
	_featureTypes = _new DTFeatureTypeSet(features_file, P1RelationFeatureType::modeltype);

	// WEIGHTS TABLE
	_weights = _new DTFeature::FeatureWeightMap(50000);

	// HISTORY FILE
	char history_buffer[PARAM_LENGTH];
	strcpy(history_buffer, _params._model_file);
	strcat(history_buffer, ".hist.txt");
	_historyStream.open(history_buffer);
	if(_historyStream.fail()){
		throw UnexpectedInputException("MaxEntSALTrainer::MaxEntSALTrainer()",
			"Can't open history stream");
	}
}

