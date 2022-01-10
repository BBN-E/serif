// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MAXENT_SAL_RELATION_TRAINER_H
#define MAXENT_SAL_RELATION_TRAINER_H

// Most of this class is based on MaxEntRelationTrainer

// NB.  For Arabic to load state files:
// _morphAnalysis = MorphologicalAnalyzer::build();

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <stdio.h>
#include <vector>

#include "MaxEntSimulatedActiveLearning/Definitions.h"

#include "Generic/common/limits.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/discTagger/DTFeature.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/relations/discmodel/DTRelationSet.h"
#include "Generic/relations/discmodel/DTRelSentenceInfo.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/relations/discmodel/P1RelationFeatureTypes.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/RelationUtilities.h"
#include "Generic/state/StateLoader.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Token.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/wordClustering/WordClusterTable.h"
#include "Generic/relations/HighYield/HYRelationInstance.h"
#include "Generic/relations/HighYield/HYInstanceSet.h"

#include "MaxEntSimulatedActiveLearning/ParameterStruct.h"
#include "MaxEntSimulatedActiveLearning/StatTracker.h"
#include "MaxEntSimulatedActiveLearning/Utilities.h"

using namespace std;

class MaxEntSALRelationTrainer {
public:
	typedef enum { TRAIN, DEVTEST, EXTRACT } Mode;

	MaxEntSALRelationTrainer();
	~MaxEntSALRelationTrainer();

	void doActiveLearning(bool realAL = true, int modelNum = -1);
	void devTest(int modelNum = -1);

protected:
	void addDevtestSentencesFromFileList();
	void addSeedSentencesFromFileList();
	void addActiveLearningSentencesFromFileList();
	void loadWeights(int modelNum);
	void loadAnnotatedRequests(int modelNum);
	void trainOnSeed(int modelNum);
	void walkThroughSentence(DTRelSentenceInfo *, HYInstanceSet&, int index, int mode);
	void decodeALArray();
	int selectRequests();
	void printDebugInfo(HYRelationInstance *instance, UTF8OutputStream& stream); 
	int addRequestsToTraining();
	void printRequests(int modelNum) const;
	void printUsed(int modelNum) const;
	void loadUsed(int modelNum);
	void modifyDecodingScores();
	double getMargin(double* array, int size);
	bool validMention(Mention* m);
	int maxInstances();

	void writeWeights(int modelNum);

	void openDebugStream(int modelNum);
	void closeDebugStream();

	// FEATURES, WEIGHTS, TAGS, DECODER
	DTFeatureTypeSet *_featureTypes;
	DTTagSet *_tagSet;
	DTFeature::FeatureWeightMap *_weights;
	MaxEntModel *_decoder;

	// FOR DECODING
	int	_probSize;
	double* _probabilities;

	// OUTPUT STREAMS
	UTF8OutputStream _devTestStream;
	UTF8OutputStream _historyStream;
	UTF8OutputStream _debugStream;
	
	// TRAINING AND ACTIVE LEARNING SETS
	HYInstanceSet _activeLearningPool;
	HYInstanceSet _initialPool;
	HYInstanceSet _devTestSet;
	HYInstanceSet _annoRequests;  // these are the instances that are chosen to be annotated
	HYInstanceSet _finishedRequests;  // these are annotated instances that have been read in

	// MODEL FILE SUFFIX
	int _iteration;

	// PARAMETERS AND STATS
	ParameterStruct _params;  // these are all read from the .par file
	StatTracker _stats;

	// REUSABLE OBSERVATION
	RelationObservation* _observation;
};

#endif
