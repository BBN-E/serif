// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P1_DECODER_H
#define P1_DECODER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/DebugStream.h"
#include "Generic/discTagger/DTFeature.h"

class DTTagSet;
class DTObservation;
class DTFeatureTypeSet;
class DTState;
//class hash_map;

class P1Decoder {

private:
	DTTagSet *_tagSet;
//	DTFeatureTypeSet *_featureTypes;
	DTFeatureTypeSet **_featureTypes;
	bool _duplicated_featureTypes; // True if we own _featureTypes -- i.e., if we should delete it.
	DTFeature::FeatureWeightMap *_weights;
	bool _add_hyp_features;
	double _overgen_percentage;
	double _undergen_percentage;

	/** This is used for taking the actual average of the weights.
	*	The idea is to keep record of how many examples the current hypothesis has seen.
	*   Before it is updated, the _weights*_hypothesis_life is added to the sum.
	*   Note that this also requires the user to notify the object when the training is done,
	*   so that the last hypothesis can be will be added to the summation.
	*/
	bool _real_averaged_mode;
	long _hypothesis_life;

	UTF8OutputStream* _dumpStream;

public:
	/** If add_hyp_features is true, then all wrong-hypothesis features,
	  * even those not seen in training data, will be added to the
	  * weight table. This is different from Scott's QuickTagger and takes longer.
	  * For decoding, add_hyp_features has no effect.
	  */
	P1Decoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
			 DTFeature::FeatureWeightMap *weights, 
			 bool add_hyp_features = true);

	P1Decoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
			 DTFeature::FeatureWeightMap *weights, double overgen_percentage,
			 bool add_hyp_features = true, bool real_averaged_mode = false);
	/** a constructor that allows to define different featureTypes for each tag
	* featureTypes is an array of DTFeatureTypeSet indexed by tag index
	* weights is an array of FeatureWeightMap indexed by tag index
	*/
	P1Decoder(DTTagSet *tagSet, DTFeatureTypeSet **featureTypes,
			 DTFeature::FeatureWeightMap *weights, 
			 bool add_hyp_features = true, bool real_averaged_mode = false);
	~P1Decoder();

	static DTFeatureTypeSet** duplicateFeatureTypes(DTFeatureTypeSet *featureTypes, int n_tags);
    	
	void setOvergenPercentage(double overgen) { _overgen_percentage = overgen; }

	// overgeneration trumps undergeneration -- you can't do both at the same time, duh!
	void setUndergenPercentage(double undergen) { 
		if (_overgen_percentage == 0)
			_undergen_percentage = undergen; 
	}
    
	int decodeToInt(DTObservation *observation);
	int decodeToInt(DTObservation *observation, double *tagScores);
	Symbol decodeToSymbol(DTObservation *observation);
	double getScore(DTObservation *observation, int tag);
	/** For use with descriptor linking, keep track of scores of link
	*	options so that ment-entity links can be sorted	
	*/
	int decodeToInt(DTObservation *observation, double& score);
	/** decode to the integer tag using the scores listed in the tagScores array without reextracting
	 *  and recomputing the scores from observation.
	 *  This is currently used in the event model to eliminate the redundancy in computing the scores twice,
	 *  one for the distribution of the scores, and the other for the best tag.
	 */
	int decodeToInt(DTObservation *observation, double *tagScores, double& score);
	Symbol decodeToSymbol(DTObservation *observation, double& score);
	bool train(DTObservation *observation, int correct_answer, double=1.0);

	UTF8OutputStream _debugStream;
	bool DEBUG;

	/** Add features that occur in given training data to weight table
	  * (with weight of 1), then add features that with future of NONE 
	  * as well (with weight of 0)
	  */
	void addFeatures(DTObservation *observation, int correct_answer, int default_value = 0);
	void adjustErrorWeights(DTObservation *hypothesis_observation, int hypothsys_tag
								   , DTObservation *correct_observation, int correct_tag
								   , double delta, bool add_unseen_features);
	void adjustWeights(DTObservation *observation,
					   int answer, double delta,
					   bool add_unseen_features);
	void advanceInLife() { _hypothesis_life++; }
	/** Add the current weights*_hypothesis_life to the summation, and reset the _hypothesis_life to 0
	*/
	void computeAverage();

	/*print score values for setting a threshhold for descriptor linking
	*/
	void printScoreInfo(DTObservation *observation, int correct_answer, 
		int incorrect_answer, UTF8OutputStream& uos);
	void printScoreInfo(DTObservation *observation, int correct_answer, 
		UTF8OutputStream& uos);
	std::wstring getDebugInfo(DTObservation *observation, int answer) const; 
	void printDebugInfo(DTObservation *observation, int answer, UTF8OutputStream& debug);
	void printDebugInfo(DTObservation *observation, int answer, DebugStream& debug);
	void printHTMLDebugInfo(DTObservation *observation, int answer, UTF8OutputStream& debug);

	void setLogFile(const std::string& logfile);
	//Largely for debugging, Return the tagSet types for which a feature with feature_name exists in the model
	std::set<int> featureInTable(DTObservation *observation, Symbol feature_name);

private:
	double scoreState(const DTState &state, int tag);
	double scoreState(const DTState &state);
	void printDebugInfo(DTObservation *observation, int answer);
	void printTrainDebugInfo(DTObservation *observation, int correct_answer, double weight, bool is_feature_vector_dump);
};

#endif

