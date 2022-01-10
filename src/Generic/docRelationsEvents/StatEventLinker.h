// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_EVENT_LINKER_H
#define STAT_EVENT_LINKER_H

class EventMention;
class EntitySet;
class EventLinkObservation;
class MaxEntModel;
class DTTagSet;
class DTFeatureTypeSet;
class DocTheory;
#include "discTagger/DTFeature.h"
#include <vector>


class StatEventLinker {
public:
	StatEventLinker(int mode_);
	~StatEventLinker();

	void train();
	void roundRobin();

	void getLinkScores(EntitySet *entitySet, std::vector<EventMention*> &vMentions, int n_vmentions,
                       std::vector< std::vector<float> > &scores);
	void replaceModel(char *model_file);
	void resetRoundRobinStatistics();
	void printRoundRobinStatistics(UTF8OutputStream &out);

	enum { TRAIN, ROUNDROBIN, DECODE };
private:
	int MODE;

	MaxEntModel *_model;
	DTFeature::FeatureWeightMap *_weights;
	DTTagSet *_tagSet;
	DTFeatureTypeSet *_featureTypes;
	double *_tagScores;
	double _link_threshold;

	void loadTrainingDataFromList(const char *listfile);
	void loadTrainingData(const wchar_t *filename, int& index);
	void addTrainingDataToModel();
	int fillAllVMentionsArray(DocTheory *docTheory);

	EventLinkObservation *_observation;
	class DocTheory ** _docTheories;
	int _num_documents;
	EventMention **_allVMentions;

	void dumpTrainingParameters(UTF8OutputStream &out);

	// ROUND ROBIN DECODING
	int _correct_link;
	int _correct_no_link;
	int _missed;
	int _spurious;
	void printHTMLEventMention(EventMention *vm, UTF8OutputStream &out);


};


#endif
