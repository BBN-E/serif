// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_OLD_MAXENT_RELATION_MODEL_H
#define CH_OLD_MAXENT_RELATION_MODEL_H

class OldMaxEntEvent;
class OldMaxEntModel;
class ChinesePotentialRelationInstance;


class OldMaxEntRelationModel  {
private:
	OldMaxEntModel* _typeModel;
	OldMaxEntModel* _existanceModel;

	/** true if we first decide whether a relation exists,
	  * and then classify it -- false if we make the decision
	  *	in one step */
	bool SPLIT_LEVEL_DECISION;

	struct HashKey {
        size_t operator()(const Symbol s) const {
			return s.hash_code();
        }
    };

    struct EqualKey {
        bool operator()(const Symbol s1, const Symbol s2) const {
			return s1 == s2;
		}
    };

	typedef serif::hash_map<Symbol, Symbol*, HashKey, EqualKey> WordClusterTable;
	WordClusterTable *_clusterTable;

	
public:
	OldMaxEntRelationModel();
	OldMaxEntRelationModel(const char *file_prefix);
	~OldMaxEntRelationModel();
	
	int findBestRelationType(ChinesePotentialRelationInstance *instance);
	int findNBestRelationTypes(ChinesePotentialRelationInstance *instance, int *results, double *scores, int max_results); 
	
	void train(char *training_file, char* output_file_prefix);
	void printModel(const char *file_prefix);
	void testModel(char *vector_file);
	
	static const int MAX_PREDICATES;
	static const Symbol IS_RELATION;
	static const Symbol NO_RELATION;
	static const Symbol NULL_SYM;

private:
	void initializeWordClusterTable();

	void fillMaxEntTypeEvent(OldMaxEntEvent *event, ChinesePotentialRelationInstance *inst);
	void fillMaxEntExistanceEvent(OldMaxEntEvent *event, ChinesePotentialRelationInstance *inst);

	int readTrainingVector(UTF8InputStream &stream, Symbol *ngram);
	
};

#endif
