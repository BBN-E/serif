// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/theories/Token.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/relations/discmodel/P1RelationTrainer.h"
#include "Generic/relations/discmodel/xx_P1RelationFeatureTypes.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/DTRelationSet.h"
#include "Generic/relations/discmodel/DTRelSentenceInfo.h"
#include "Generic/relations/HighYield/HYInstanceSet.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/TrainingLoader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"

#include "Generic/docRelationsEvents/DocEventHandler.h"

#include "Generic/SPropTrees/SForestUtilities.h"
#include "Generic/relations/discmodel/PropTreeLinks.h"
#include "Generic/relations/discmodel/featuretypes/ParsePathBetweenFT.h"

#include <iostream>
#include <stdio.h>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>


using namespace std;
#define PRINT_EVERY_EPOCH 0

P1RelationTrainer::P1RelationTrainer()
	: _featureTypes(0), _tagSet(0),
	  _decoder(0), _weights(0), _num_sentences(0),
	   _use_high_yield_annotation(false),
	  _highYieldPool(0), _highYieldAnnotation(0),
	  _set_secondary_parse(true), _debug_devtest_mistakes(false)
{
	
	FeatureModule::load();
	std::vector<std::string> modules = FeatureModule::loadedFeatureModules();
	if (!modules.empty()) {
		cout << "  Using Modules:";
		BOOST_FOREACH(std::string name, modules) {
			cout << " " << name;
		}
		cout << "\n";
	}

	P1RelationFeatureTypes::ensureFeatureTypesInstantiated();
	WordClusterTable::ensureInitializedFromParamFile();
	//The relation validation string is used to determine which relation type/ argument types are allowed
	//
	std::string validation_str = ParamReader::getRequiredParam("p1_relation_validation_str");
	_observation = _new RelationObservation(validation_str.c_str());

	//Initialize the Morphological Analyzer,
	//Because Arabic needs it to load state files
	_morphAnalysis = MorphologicalAnalyzer::build();


	// TAG SET
	std::string tag_set_file = ParamReader::getRequiredParam("p1_relation_tag_set_file");
	_tagSet = _new DTTagSet(tag_set_file.c_str(), false, false);

	// FEATURES
	std::string features_file = ParamReader::getRequiredParam("p1_relation_features_file");
	_featureTypes = _new DTFeatureTypeSet(features_file.c_str(), P1RelationFeatureType::modeltype);

	_overgen_percentage = 0;
	// TRAINING ONLY FEATURES (not devtest)
	if (!ParamReader::isParamTrue("p1_relation_devtest")) {
		// EPOCHS
		_epochs = ParamReader::getRequiredIntParam("p1_trainer_epochs");

		// SEED FEATURES
		_seed_features = ParamReader::getRequiredTrueFalseParam("p1_trainer_seed_features");
		// HYP FEATURES
		_add_hyp_features = ParamReader::getRequiredTrueFalseParam("p1_trainer_add_hyp_features");

		// REAL AVERAGED
		_real_averaged_mode = ParamReader::isParamTrue("p1_real_averaged_mode");

		if (!_real_averaged_mode) {
			// WEIGHTSUM GRANULARITY
			_weightsum_granularity = ParamReader::getRequiredIntParam("p1_trainer_weightsum_granularity");
		}

	} else {

		// OVERGENERATION PERCENTAGE
		_overgen_percentage = ParamReader::getRequiredFloatParam("p1_relation_overgen_percentage");

	}

	// MODEL FILE
	_model_file = ParamReader::getRequiredParam("p1_relation_model_file");

	// TRAINING DATA LIST
	std::string file_list = ParamReader::getRequiredParam("p1_training_file_list");

	// BEAM WIDTH -- this needs to be the same as in training, or the stateloader can't work
	_beam_width = ParamReader::getOptionalIntParamWithDefaultValue("beam_width", 1);

	//This is required to be set before loading data
	_set_secondary_parse = ParamReader::getOptionalTrueFalseParamWithDefaultVal("p1_set_secondary_parse", true);

	// LOAD DATA
	_trainingLoader = _new TrainingLoader(file_list.c_str(), L"doc-relations-events");
	_sentenceInfo = _new DTRelSentenceInfo *[_trainingLoader->getMaxSentences()];
	_num_sentences = loadTrainingData();

	// CREATE WEIGHTS TABLE
	_weights = _new DTFeature::FeatureWeightMap(50000);

	_correct = 0;
	_missed = 0;
	_spurious = 0;
	_wrong_type = 0;
	//IDENT relations make a relation between all coreferant mentions in a sentence.
	//This proved useful in previous relation models, but was not helpful in Arabic Serif, 
	//so it is turned off for now
	//to train a model with 'ident' relations-
	//1) CASerif must be run with the identity relation producing flag on
	//2) The tag-set must include 'IDENT'
	//3) include-ident-relations must be in the param file as include-ident-relations: true
	_include_ident_relations = ParamReader::isParamTrue("include_ident_relations");

	// ADD HIGH YIELD ANNOTATION
	if (ParamReader::isParamTrue("p1_use_high_yield_annotation"))
		_use_high_yield_annotation = true;

	if (_use_high_yield_annotation) {
		// READ IN STATE FILES FOR HIGH YIELD POOL
		std::string high_yield_pool = ParamReader::getRequiredParam("p1_high_yield_pool_list");
		_highYieldPool = _new HYInstanceSet(_tagSet);
		_highYieldPool->addDataFromSerifFileList(high_yield_pool.c_str(), _beam_width);

		// READ IN ACTUAL HIGH YIELD ANNOTATION
		std::string high_yield_annotation_list = ParamReader::getRequiredParam("p1_high_yield_annotation_list");
		_highYieldAnnotation = _new HYInstanceSet(_tagSet);
		boost::scoped_ptr<UTF8InputStream> tempFileListStream_scoped_ptr(UTF8InputStream::build(high_yield_annotation_list.c_str()));
		UTF8InputStream& tempFileListStream(*tempFileListStream_scoped_ptr);
		UTF8Token token;
		while (!tempFileListStream.eof()) {
			tempFileListStream >> token;
			if (wcscmp(token.chars(), L"") == 0)
				break;
			_highYieldAnnotation->addDataFromAnnotationFile(token.chars(), *_highYieldPool);
		}
		tempFileListStream.close();

		if (!ParamReader::isParamTrue("p1_relation_devtest")) {
			if (!_real_averaged_mode) {
				_hy_weightsum_granularity = ParamReader::getRequiredIntParam("p1_trainer_hy_weightsum_granularity");
			}
		}
	}

	// Output examples that cause mistakes in devtest
	_debug_devtest_mistakes = ParamReader::isParamTrue("p1_trainer_debug_devtest_mistakes");
}

P1RelationTrainer::~P1RelationTrainer() {
	delete _featureTypes;
	delete _tagSet;
	if (_highYieldPool != 0) {
		_highYieldPool->deleteObjects();
		delete _highYieldPool;
	}
	if (_highYieldAnnotation != 0) {
		// I think this just points to objects also in 
		// _highYieldPool, so we don't need to delete both.
		//_highYieldAnnotation->deleteObjects(); 
		delete _highYieldAnnotation;
	}
}


int P1RelationTrainer::loadTrainingData() {
	int max_sentences = _trainingLoader->getMaxSentences();
	int i;

	//std::wcerr << "\nload sentences: " << std::setw(8);
	for (i = 0; i < max_sentences; i++) {
		SentenceTheory *theory = _trainingLoader->getNextSentenceTheory();
		const DocTheory* dt=_trainingLoader->getCurrentDocTheory();
		if (theory == 0) break;
		_sentenceInfo[i] = _new DTRelSentenceInfo(1);
		_sentenceInfo[i]->parses[0] = theory->getPrimaryParse();
		_sentenceInfo[i]->parses[0]->gainReference();
		_sentenceInfo[i]->npChunks[0] = theory->getNPChunkTheory();
		if (_sentenceInfo[i]->npChunks[0] != 0)
			_sentenceInfo[i]->npChunks[0]->gainReference();
		//make sure this isnt double counting when fullParse == primaryParse
		if (_set_secondary_parse) {
			_sentenceInfo[i]->secondaryParses[0] = theory->getFullParse();
			_sentenceInfo[i]->secondaryParses[0]->gainReference();
		} else {
			_sentenceInfo[i]->secondaryParses[0] = 0;
		}
		_sentenceInfo[i]->mentionSets[0] = theory->getMentionSet();
		_sentenceInfo[i]->mentionSets[0]->gainReference();
		_sentenceInfo[i]->valueMentionSets[0] = theory->getValueMentionSet();
		_sentenceInfo[i]->valueMentionSets[0]->gainReference();
		_sentenceInfo[i]->propSets[0] = theory->getPropositionSet();
		_sentenceInfo[i]->propSets[0]->gainReference();
		_sentenceInfo[i]->propSets[0]->fillDefinitionsArray();
		_sentenceInfo[i]->relSets[0] =
			_new DTRelationSet(_sentenceInfo[i]->mentionSets[0]->getNMentions(),
			theory->getRelMentionSet(), _tagSet->getNoneTag());
		_sentenceInfo[i]->entitySets[0] = _trainingLoader->getCurrentEntitySet();
		_sentenceInfo[i]->entitySets[0]->gainReference();
		_sentenceInfo[i]->documentTopic = DocEventHandler::assignTopic(dt);
		//these proptrees operate on only one sentence!!
		if ( !(i%1000) ) SForestUtilities::initializeMemoryPools();
		_sentenceInfo[i]->propTreeLinks[0] = _new PropTreeLinks(_trainingLoader->getCurrentSentenceIndex()-1, dt);

		/*bool missedSome=false;
			_observation->resetForNewSentence(_sentenceInfo[ind]);
			for ( int k=0; k < _sentenceInfo[ind]->mentionSets[0]->getNMentions(); k++ ) {
				for ( int l=0; l < _sentenceInfo[ind]->mentionSets[0]->getNMentions(); l++ ) {
					Symbol tag=_sentenceInfo[ind]->relSets[0]->getRelation(k,l);
					if ( k == l || tag == _sentenceInfo[ind]->relSets[0]->getNoneSymbol() ) 
						continue;
					_observation->populate(k,l);
					tried++;
					if ( _observation->getPropLink()->isEmpty() ) {
						const Mention* m1=_sentenceInfo[ind]->mentionSets[0]->getMention(k);
						const Mention* m2=_sentenceInfo[ind]->mentionSets[0]->getMention(l);

						std::wcerr << "\n--- " << tag.to_string() << " for \"" << m1->getNode()->toTextString() 
							<< "\" and \"" << m2->getNode()->toTextString() << "\" in \"" 
							<< dt->getSentenceTheory(j)->getTokenSequence()->toString() << "\"";
						missed++;
						missedSome = true;
					}
				}
			}*/
			//std::wcerr << "\nafter sent=" << ind << ": tried=" << tried << "; missed=" << missed << "\t";*/
		/* AFTER THAT, WE SHOULDN'T COUNT ON DocTheory OR SentenceTheory ANYMORE, BECAUSE THEY ARE GONE
	       HOWEVER MentionSet's AND PropositionSet's (with all the Mention's, Proposittion's AND Entity's) 
	       ARE STILL THERE BECAUSE WE GAINED ONE REFERENCE TO THEM. */
		//std::wcerr << "\b\b\b\b\b\b\b\b" << std::setw(8) << i;
	}
	SForestUtilities::clearMemoryPools();
	//std::wcerr << "\n";

	return i;
}

void P1RelationTrainer::devTest() {
	int numCandidates=0, numCorrect=0;
	std::string param = ParamReader::getRequiredParam("p1_relation_devtest_out");
	_devTestStream.open(param.c_str());

	DTFeature::readWeights(*_weights, _model_file.c_str(), P1RelationFeatureType::modeltype);
	_decoder = _new P1Decoder(_tagSet, _featureTypes, _weights,
		_overgen_percentage, _add_hyp_features);

	for (int i = 0; i < _num_sentences; i++) {
		if (i % 10 == 0)
			std::cerr << i << "\r";
		walkThroughSentence(i, DEVTEST, numCandidates, numCorrect);
	}

	double recall = (double) _correct / (_missed + _wrong_type + _correct);
	double precision = (double) _correct / (_spurious + _wrong_type + _correct);

	_devTestStream << "MENTION_PAIRS: " << numCandidates << "<br>\n";
	_devTestStream << "TOTAL_CORRECT: " << numCorrect << "<br>\n";
	_devTestStream << "CORRECT: " << _correct << "<br>\n";
	_devTestStream << "MISSED: " << _missed << "<br>\n";
	_devTestStream << "SPURIOUS: " << _spurious << "<br>\n";
	_devTestStream << "WRONG TYPE: " << _wrong_type << "<br>\n<br>\n";
	_devTestStream << "RECALL: " << recall << "<br>\n";
	_devTestStream << "PRECISION: " << precision << "<br>\n";

}


void P1RelationTrainer::train() {
	_decoder = _new P1Decoder(_tagSet, _featureTypes, _weights, 0, _add_hyp_features, _real_averaged_mode);

	if (_seed_features) {
		SessionLogger::info("SERIF") << "Seeding weight table with all features from training set...\n";
		for (int i = 0; i < _num_sentences; i++) {
			//std::wcerr << "\r" << i;
			walkThroughSentence(i, ADD_FEATURES);
		}
		if (_use_high_yield_annotation) {
			walkThroughHighYieldAnnotation(ADD_FEATURES);
		}
		for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
			iter != _weights->end(); ++iter)
		{
			(*iter).second.addToSum();
		}
		writeWeights(0);
	}

	std::cerr << "\n";
	for (int epoch = 0; epoch < _epochs; epoch++) {
		std::cerr << "Epoch " << epoch + 1 << "...\n";
		if (_decoder->DEBUG) {
			_decoder->_debugStream << "Epoch: " << epoch + 1 << "\n";
		}
		trainEpoch();
		writeWeights(epoch+1);
	}

	if (_real_averaged_mode)
		_decoder->computeAverage();
	writeWeights();

	// this isn't really necessary in practice, but helpful for memory leak
	// detection
	for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
		 iter != _weights->end(); ++iter)
	{
		(*iter).first->deallocate();
	}
	delete _weights;
	delete _decoder;
	delete _trainingLoader;
}


void P1RelationTrainer::trainEpoch() {
	int num_candidates = 0;
	int num_correct = 0;
	for (int i = 0; i < _num_sentences; i++) {
		if (i % 10 == 0){
			std::cerr << i << "\r";
		}
		if(i % 1000 == 0){
			if(num_candidates != 0){
				double per_corr = ((double) num_correct/num_candidates);
				SessionLogger::info("SERIF")<<"    "<<i<<": "<<per_corr<<std::endl;
			}

		}
		walkThroughSentence(i, TRAIN, num_candidates, num_correct);

		if (!_real_averaged_mode && (i % _weightsum_granularity == 0)) {
			for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
				iter != _weights->end(); ++iter)
			{

				(*iter).second.addToSum();
			}
		}
	}
	if (_use_high_yield_annotation)
		walkThroughHighYieldAnnotation(TRAIN, num_candidates, num_correct);

	if (num_candidates != 0){
		double per_corr = ((double) num_correct/num_candidates);
		SessionLogger::info("SERIF")<<"** Epoch complete:"<<per_corr<<std::endl;
	}

}
void P1RelationTrainer::walkThroughSentence(int index, int mode){
	int a, b;
	walkThroughSentence(index, mode, a, b);
}
void P1RelationTrainer::walkThroughSentence(int index, int mode, int& num_candidates, int& num_correct) {
	const MentionSet *mset = _sentenceInfo[index]->mentionSets[0];
	_observation->resetForNewSentence(_sentenceInfo[index]);
	int nmentions = mset->getNMentions();
	int n_comparisons = 0;

	if (mode == DEVTEST) {
		_devTestStream << "Sentence " << index << ": ";
		_devTestStream << mset->getParse()->getRoot()->toTextString() << "<br>\n";
	}

	for (int i = 0; i < nmentions; i++) {
		if (!mset->getMention(i)->isOfRecognizedType() ||
			mset->getMention(i)->getMentionType() == Mention::NONE ||
			mset->getMention(i)->getMentionType() == Mention::APPO ||
			mset->getMention(i)->getMentionType() == Mention::LIST)
			continue;
		for (int j = i + 1; j < nmentions; j++) {
			if (!mset->getMention(j)->isOfRecognizedType() ||
				mset->getMention(j)->getMentionType() == Mention::NONE ||
				mset->getMention(j)->getMentionType() == Mention::APPO ||
				mset->getMention(j)->getMentionType() == Mention::LIST)
				continue;
			if (!RelationUtilities::get()->validRelationArgs(mset->getMention(i), mset->getMention(j)))
				continue;
			Symbol correct_symbol = _sentenceInfo[index]->relSets[0]->getRelation(i,j);

			//a hack to skip mention pairs for which we don't know if they relate to each other or not
			if ( correct_symbol == L"IGNORE" ) continue;

			num_candidates++;
			_observation->populate(i, j);

			int correct_answer = _tagSet->getTagIndex(correct_symbol);
			if(!_include_ident_relations){
				if(_sentenceInfo[index]->relSets[0]->getRelation(i,j) == Symbol(L"IDENT")){
					correct_answer = _tagSet->getTagIndex(_tagSet->getNoneTag());
					correct_symbol = _tagSet->getNoneTag();
				}
			}
			
			n_comparisons++;
			if (correct_answer == -1) {
				char error[150];
				sprintf(error, "unknown relation type in training: %s",
					_sentenceInfo[index]->relSets[0]->getRelation(i,j).to_debug_string());
				throw UnexpectedInputException("P1RelationTrainer::trainSentence()", error);
			}
			if (mode != GET_SYNTAX){
				if (_decoder->DEBUG) {	
					_decoder->_debugStream << "Sentence " << index << ":" << n_comparisons << "\n";
					_decoder->_debugStream << "RELATION: " << _tagSet->getTagSymbol(correct_answer).to_string() << "\n";
					_decoder->_debugStream << "MENTIONS:\n";
					_decoder->_debugStream << _observation->getMention1()->getNode()->toTextString() << L"\n";
					_decoder->_debugStream << _observation->getMention2()->getNode()->toTextString() << L"\n";
				}
			}
			if (mode == TRAIN) {
				double dweight=1.0;
				if ( correct_symbol != _tagSet->getNoneTag() ) {
					dweight = ParamReader::getOptionalFloatParamWithDefaultValue("p1_weight_delta_for_true_relations",dweight);
				}
				bool correct = _decoder->train(_observation, correct_answer, dweight);
				if (correct) num_correct++;
			} else if (mode == ADD_FEATURES) {
				_decoder->addFeatures(_observation,
					_tagSet->getTagIndex(_sentenceInfo[index]->relSets[0]->getRelation(i,j)), 0);
			} 
			else if (mode == DEVTEST) {
				Symbol correctAnswer = correct_symbol;
				Symbol hypothesis = _decoder->decodeToSymbol(_observation);
				if(!_include_ident_relations){
					if(hypothesis == Symbol(L"IDENT")){
						hypothesis = _tagSet->getNoneTag();
					}
				}
				/*if (correctAnswer != _tagSet->getNoneTag() &&
					_observation->getPropLink()->isEmpty() &&
					mset->getMention(i)->getMentionType() != Mention::PART &&
					mset->getMention(j)->getMentionType() != Mention::PART)
				{
					_devTestStream << L"<b>Answer without prop link (";
					for (int th = 1; th < 10; th++) {
						if (!_observation->getNthPropLink(th)->isEmpty()) {
							_devTestStream << "X";
						}
					}
					_devTestStream << "):</b> ";
				}*/


				if ( correctAnswer == hypothesis ) num_correct++;
				//!!! std::wcout << "%%%###\t" << correctAnswer.to_string() << "\t" << hypothesis.to_string() << "\n";


				if (correctAnswer != _tagSet->getNoneTag() ||
					hypothesis != _tagSet->getNoneTag())
				{
					_devTestStream << n_comparisons << ": ";
					_devTestStream << mset->getMention(i)->getNode()->getHeadWord().to_string();
					_devTestStream << L" & ";
					_devTestStream << mset->getMention(j)->getNode()->getHeadWord().to_string();
					_devTestStream << L": ";
					_devTestStream << hypothesis.to_string() << L" ";
					if (correctAnswer == hypothesis) {
						_devTestStream << L"<font color=\"red\">CORRECT</font><br>\n";
						_correct++;
					} else if (correctAnswer == _tagSet->getNoneTag()) {
						_devTestStream << L"<font color=\"blue\">SPURIOUS</font><br>\n";
						_spurious++;
					} else if (hypothesis == _tagSet->getNoneTag()) {
						_devTestStream << L"<font color=\"purple\">MISSING (";
						_devTestStream << correctAnswer.to_string();
						_devTestStream << ")</font><br>\n";
						_missed++;
					} else if (correctAnswer != hypothesis) {
						_devTestStream << L"<font color=\"green\">WRONG TYPE (";
						_devTestStream << correctAnswer.to_string();
						_devTestStream << ")</font><br>\n";
						_wrong_type++;
					}

					/*//TMP
					_devTestStream << "<br>" << hypothesis.to_debug_string() << "<br>";
					if (hypothesis == _tagSet->getNoneTag())
						_decoder->printHTMLDebugInfo(_observation, _tagSet->getNoneTagIndex(), _devTestStream);
					else _decoder->printHTMLDebugInfo(_observation, _tagSet->getTagIndex(hypothesis), _devTestStream);
					_devTestStream << "<br>";
					*/

				}
				
				if (_debug_devtest_mistakes && correctAnswer != hypothesis) {
					//Record examples that cause mistakes
					writeHtmlMentions(_observation, _devTestStream);

					_devTestStream << "<br>CORRECT RELATION: " << correctAnswer.to_debug_string() << "<br>";
					_decoder->printHTMLDebugInfo(_observation, _tagSet->getTagIndex(correctAnswer), _devTestStream);
					_devTestStream << "<br>PREDICTED RELATION: " << hypothesis.to_debug_string() << "<br>";
					_decoder->printHTMLDebugInfo(_observation, _tagSet->getTagIndex(hypothesis), _devTestStream);
					_devTestStream << "<br>\n";
				}
			} 
			//use this mode to pull parse tree patterns out of serif produced parses
			else if (mode == GET_SYNTAX){
				Symbol ngram[2];
				Symbol relType =_sentenceInfo[index]->relSets[0]->getRelation(i,j);
				Symbol noneRelType = Symbol(L"NONE");
				Symbol someRelType = Symbol(L"SomeRelation");
				_pair_count++;
				if(relType != noneRelType){
						_non_none_count++;
				}
				ngram[0] = relType;
				

				//get Distance
				int start_ment1;
				int end_ment1;
				int start_ment2;
				int end_ment2;
				if(_observation->getMention1()->mentionType == Mention::NAME){
					start_ment1 = _observation->getMention1()->getNode()->getStartToken();
					end_ment1 = _observation->getMention1()->getNode()->getEndToken();
				}
				else{
					start_ment1 = _observation->getMention1()->getHead()->getStartToken();
					end_ment1 = _observation->getMention1()->getHead()->getEndToken();
				}
				if(_observation->getMention2()->mentionType == Mention::NAME){
					start_ment2 = _observation->getMention2()->getNode()->getStartToken();
					end_ment2 = _observation->getMention2()->getNode()->getEndToken();
				}
				else{
					start_ment2 = _observation->getMention2()->getHead()->getStartToken();
					end_ment2 = _observation->getMention2()->getHead()->getEndToken();
				}
				
				int dist = end_ment1 - start_ment2;
				if(dist  < 0){
					dist = end_ment2 - start_ment1;
				}

				wchar_t num[50];
#if defined(_WIN32)
				_itow(dist, num, 10);
#else
				swprintf (num, 10, L"%d", dist);
#endif
				ngram[1] = Symbol(num);
				_distBtwnMent->add(ngram, 1);

				if(relType != noneRelType){
					ngram[0] = someRelType;
					_distBtwnMent->add(ngram, 1);
					ngram[0] = relType;
				}

				if(dist < 6){
					//get POS
					ngram[1] = _observation->getPOSBetween();
				
					_posBtwnMent->add(ngram, 1);
					if(relType != noneRelType){
						ngram[0] = someRelType;
						_posBtwnMent->add(ngram, 1);
						ngram[0] = relType;
					}

				
					//get ParsePAth
					Symbol big_ngram[3];
					Symbol path1 = Symbol(L"-notset-");
					Symbol path2 = Symbol(L"-notset-");
					big_ngram[0] = relType;
					big_ngram[1] = path1;
					big_ngram[2] = path2;

					ParsePathBetweenFT::getParsePath(_observation->getMention1(), 
						_observation->getMention2(), _observation->getSecondaryParse()->getRoot(), path1, path2);
					
					big_ngram[0] = relType;
					big_ngram[1] = path1;
					big_ngram[2] = path2;

					_parsePathBtwnMent->add(big_ngram, 1);
					if(relType != noneRelType){
						big_ngram[0] = someRelType;
						_parsePathBtwnMent->add(big_ngram, 1);
						big_ngram[0] = relType;
					}
				}


			}
		}
	}

	if (mode == DEVTEST) {
		_devTestStream << "<br>\n";
	}

}

void P1RelationTrainer::walkThroughHighYieldAnnotation(int mode) {
	int a, b;
	walkThroughHighYieldAnnotation(mode, a, b);
}

void P1RelationTrainer::walkThroughHighYieldAnnotation(int mode, int& num_candidates, int& num_correct) {
	int n_comparisons = 0;


	for (vector<HYRelationInstance*>::iterator iter = _highYieldAnnotation->begin();
		iter < _highYieldAnnotation->end();
		++iter)
	{
		HYRelationInstance* instance = *iter;
		
		if (!instance->isValid()) 
			continue;

		DTRelSentenceInfo* sentenceInfo = instance->getSentenceInfo();
		const MentionSet *mset = sentenceInfo->mentionSets[0];
		int i = instance->getFirstIndex();
		int j = instance->getSecondIndex();
		Symbol relation = instance->getRelationSymbol();
		_observation->resetForNewSentence(sentenceInfo);

		if (mode == DEVTEST) {
			_devTestStream << "Sentence ?? " << ": ";
			_devTestStream << mset->getParse()->getRoot()->toTextString() << "<br>\n";
		}


		if (!RelationUtilities::get()->validRelationArgs(mset->getMention(i), mset->getMention(j)))
				continue;
			
		num_candidates++;
		_observation->populate(i, j);

		int correct_answer = _tagSet->getTagIndex(instance->getRelationSymbol());
		Symbol correct_symbol = instance->getRelationSymbol();

		if(!_include_ident_relations) {
			if(relation == Symbol(L"IDENT")){
				correct_answer = _tagSet->getTagIndex(_tagSet->getNoneTag());
				correct_symbol = _tagSet->getNoneTag();
			}
		}
		
		n_comparisons++;
		if (correct_answer == -1) {
			char error[150];
			sprintf(error, "unknown relation type in training: %s",
				instance->getRelationSymbol().to_debug_string());
			throw UnexpectedInputException("P1RelationTrainer::walkThroughHighYieldAnnotation()", error);
		}
		if (mode != GET_SYNTAX){
			if (_decoder->DEBUG) {
				_decoder->_debugStream << "Sentence ?? " << ":" << n_comparisons << "\n";
				_decoder->_debugStream << "RELATION: " << _tagSet->getTagSymbol(correct_answer).to_string() << "\n";
				_decoder->_debugStream << "MENTIONS:\n";
				_decoder->_debugStream << _observation->getMention1()->getNode()->toTextString() << L"\n";
				_decoder->_debugStream << _observation->getMention2()->getNode()->toTextString() << L"\n";
			}
		}
		if (mode == TRAIN) {
			bool correct = _decoder->train(_observation, correct_answer);
			if (correct)
				num_correct++;
			if (!_real_averaged_mode && (n_comparisons % _hy_weightsum_granularity == 0)) {
				for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
					iter != _weights->end(); ++iter)
				{
					(*iter).second.addToSum();
				}
			}
		} else if (mode == ADD_FEATURES) {
			_decoder->addFeatures(_observation,_tagSet->getTagIndex(sentenceInfo->relSets[0]->getRelation(i,j)), 0);
		} else if (mode == DEVTEST) {
			Symbol correctAnswer = correct_symbol;
			Symbol hypothesis = _decoder->decodeToSymbol(_observation);
			if(!_include_ident_relations) {
				if(hypothesis == Symbol(L"IDENT"))
					hypothesis = _tagSet->getNoneTag();
			}		
			/*if (correctAnswer != _tagSet->getNoneTag() &&
				_observation->getPropLink()->isEmpty() &&
				mset->getMention(i)->getMentionType() != Mention::PART &&
				mset->getMention(j)->getMentionType() != Mention::PART)
			{
				_devTestStream << L"<b>Answer without prop link (";
				for (int th = 1; th < 10; th++) {
					if (!_observation->getNthPropLink(th)->isEmpty()) {
						_devTestStream << "X";
					}
				}
				_devTestStream << "):</b> ";
			}*/
			if (correctAnswer != _tagSet->getNoneTag() ||
				hypothesis != _tagSet->getNoneTag())
			{
				_devTestStream << n_comparisons << ": ";
				_devTestStream << mset->getMention(i)->getNode()->getHeadWord().to_string();
				_devTestStream << L" & ";
				_devTestStream << mset->getMention(j)->getNode()->getHeadWord().to_string();
				_devTestStream << L": ";
				_devTestStream << hypothesis.to_string() << L" ";
				if (correctAnswer == hypothesis) {
					_devTestStream << L"<font color=\"red\">CORRECT</font><br>\n";
					_correct++;
				} else if (correctAnswer == _tagSet->getNoneTag()) {
					_devTestStream << L"<font color=\"blue\">SPURIOUS</font><br>\n";
					_spurious++;
				} else if (hypothesis == _tagSet->getNoneTag()) {
					_devTestStream << L"<font color=\"purple\">MISSING (";
					_devTestStream << correctAnswer.to_string();
					_devTestStream << ")</font><br>\n";
					_missed++;
				} else if (correctAnswer != hypothesis) {
					_devTestStream << L"<font color=\"green\">WRONG TYPE (";
					_devTestStream << correctAnswer.to_string();
					_devTestStream << ")</font><br>\n";
					_wrong_type++;
				}
			}
		}
		// We're not using this right now, so I'm not going to implement it. -JSM 10/26/05
		//use this mode to pull parse tree patterns out of serif produced parses
		//else if (mode == GET_SYNTAX) {}
	}

	if (mode == DEVTEST) {
		_devTestStream << "<br>\n";
	}

}

void P1RelationTrainer::writeWeights(int epoch) {
	UTF8OutputStream out;
	if (epoch != -1) {
		if(PRINT_EVERY_EPOCH == 0)
			return;
		std::stringstream str;
		str << _model_file << "-epoch-" << epoch;
		out.open(str.str().c_str());
	} else out.open(_model_file.c_str());

	if (out.fail()) {
		throw UnexpectedInputException("P1RelationTrainer::writeWeights()",
			"Could not open model file for writing");
	}

	dumpTrainingParameters(out);
	DTFeature::writeSumWeights(*_weights, out);
	out.close();

}

void P1RelationTrainer::dumpTrainingParameters(UTF8OutputStream &out) {

	DTFeature::recordDate(out);

	out << L"Parameters:\n";
	DTFeature::recordFileListForReference(Symbol(L"p1_training_file_list"), out);

	DTFeature::recordParamForConsistency(Symbol(L"p1_relation_tag_set_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"p1_relation_features_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"word_cluster_bits_file"), out);
	DTFeature::recordParamForConsistency(Symbol(L"lc_word_cluster_bits_file"), out);

	DTFeature::recordParamForReference(Symbol(L"word_net_dictionary_path"), out);
	DTFeature::recordParamForReference(Symbol(L"wordnet_level_start"), out);
	DTFeature::recordParamForReference(Symbol(L"wordnet_level_interval"), out);

	DTFeature::recordParamForReference(Symbol(L"p1_trainer_epochs"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_trainer_seed_features"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_trainer_add_hyp_features"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_trainer_weightsum_granularity"), out);
	DTFeature::recordParamForReference(Symbol(L"relation_finder_use_alt_models"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_relation_validation_str"), out);
	DTFeature::recordParamForConsistency(Symbol(L"relation_mention_dist_cutoff"), out);
	DTFeature::recordParamForReference(Symbol(L"allow_relations_within_distance"), out);

	if (_use_high_yield_annotation) {
		DTFeature::recordParamForReference(Symbol(L"p1_high_yield_pool_list"), out);
		DTFeature::recordParamForReference(Symbol(L"p1_high_yield_annotation_list"), out);
	}

	DTFeature::recordParamForReference(Symbol(L"p1_real_averaged_mode"), out);
	DTFeature::recordParamForReference(Symbol(L"p1_set_secondary_parse"), out);

	out << L"\n";

}

void P1RelationTrainer::writeHtmlMentions(RelationObservation* observation, UTF8OutputStream &debugStream) {
	debugStream << L"MENTIONS:<br>\n";
	debugStream << _observation->getMention1()->getNode()->toTextString() << L"<br>\n";
	debugStream << _observation->getMention2()->getNode()->toTextString() << L"<br>\n";
	debugStream << _observation->getMention1()->getNode()->getHeadWord().to_string();
	debugStream << L" & ";
	debugStream << _observation->getMention2()->getNode()->getHeadWord().to_string() << L"<br>\n";
}

//use this mode to pull parse tree patterns out of serif produced parses

void P1RelationTrainer::getSyntacticFeatures(){
	_distBtwnMent = _new NgramScoreTable(2, 1000);
	_posBtwnMent = _new NgramScoreTable(2, 5000);
	_parsePathBtwnMent =  _new NgramScoreTable(3, 5000);
	//_mentBtwnRelMent = _new NgramScoreTable(2, 5000);
	_pair_count = 0;
	_non_none_count = 0;
	for (int i = 0; i < _num_sentences; i++) 
	{
		walkThroughSentence(i, GET_SYNTAX);
	}
	SessionLogger::info("SERIF")<<"# possible relations: "<<_pair_count<<std::endl;
	SessionLogger::info("SERIF")<<"# non-none relations: "<<_non_none_count<<std::endl;
	std::string out_buff = _model_file + ".dist.txt";
	_distBtwnMent->print(out_buff.c_str());
	out_buff = _model_file + ".pos.txt";
	_posBtwnMent->print(out_buff.c_str());
	out_buff = _model_file + ".parsepath.txt";
	_parsePathBtwnMent->print(out_buff.c_str());
	delete _distBtwnMent;
	delete _posBtwnMent;
	delete _parsePathBtwnMent;
	//delete _mentBtwnRelMent;

}
