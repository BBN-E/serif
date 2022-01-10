// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/P1Decoder.h"
#include "Generic/common/ParamReader.h"

using namespace std;


P1Decoder::P1Decoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
				   DTFeature::FeatureWeightMap *weights,
				   bool add_hyp_features)
	: _add_hyp_features(add_hyp_features), _tagSet(tagSet),
	_featureTypes(duplicateFeatureTypes(featureTypes, tagSet->getNTags())), _weights(weights), _overgen_percentage(0),
	_undergen_percentage(0), _real_averaged_mode(false), _hypothesis_life(1), _duplicated_featureTypes(true),
	_dumpStream(0)
{
	std::string buffer = ParamReader::getParam("p1_debug");
	DEBUG = false;
	if (!buffer.empty()) {
		DEBUG = true;
		_debugStream.open(buffer.c_str());
	}
}

P1Decoder::P1Decoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
				   DTFeature::FeatureWeightMap *weights, double overgen_percentage,
				   bool add_hyp_features, bool real_averaged_mode)
	: _add_hyp_features(add_hyp_features), _tagSet(tagSet),
	_featureTypes(duplicateFeatureTypes(featureTypes, tagSet->getNTags())), _weights(weights), _overgen_percentage(overgen_percentage),
	_undergen_percentage(0), _real_averaged_mode(real_averaged_mode), _hypothesis_life(1), _duplicated_featureTypes(true),
	_dumpStream(0)
{
	std::string buffer = ParamReader::getParam("p1_debug");
	DEBUG = false;
	if (!buffer.empty()) {
		DEBUG = true;
		_debugStream.open(buffer.c_str());
	}
}

P1Decoder::P1Decoder(DTTagSet *tagSet, DTFeatureTypeSet **featureTypes,
				   DTFeature::FeatureWeightMap *weights,
				   bool add_hyp_features, bool real_averaged_mode)
	: _add_hyp_features(add_hyp_features), _tagSet(tagSet),
	_featureTypes(featureTypes), _weights(weights), _overgen_percentage(0),
	_undergen_percentage(0), _real_averaged_mode(real_averaged_mode), _hypothesis_life(1), _duplicated_featureTypes(false),
	_dumpStream(0)
{
	std::string buffer = ParamReader::getParam("p1_debug");	
	DEBUG = false;
	if (!buffer.empty()) {
		DEBUG = true;
		_debugStream.open(buffer.c_str());
	}
}

P1Decoder::~P1Decoder(){
	if (_duplicated_featureTypes)
		delete [] _featureTypes;
	if (_dumpStream) {
		_dumpStream->close();
		delete _dumpStream;
	}
}

DTFeatureTypeSet** P1Decoder::duplicateFeatureTypes(DTFeatureTypeSet *featureTypes, int n_tags){
	DTFeatureTypeSet** tagsFeatureTypes	= _new DTFeatureTypeSet*[n_tags];
	for(int i=0; i<n_tags; i++)
		tagsFeatureTypes[i] = featureTypes;
	return tagsFeatureTypes;
}



int P1Decoder::decodeToInt(DTObservation *observation, double& finalscore) {
	int n_tags = _tagSet->getNTags();

	double best_score = -10000000000000LL; //-10000000;
	int best_tag = -1;
	double second_best_score = -10000000000000LL; //-10000000;
	int second_best_tag = -1;
	for (int i = 0; i < n_tags; i++) {
		if (!observation->isValidTag(_tagSet->getTagSymbol(i)))
			continue;
		DTState state(_tagSet->getTagSymbol(i), Symbol(),
			Symbol(), 0, std::vector<DTObservation*>(1, observation));
		double score = scoreState(state);
		if (score > best_score) {
			second_best_score = best_score;
			second_best_tag = best_tag;
			best_score = score;
			best_tag = i;
		} else if (score > second_best_score) {
			second_best_score = score;
			second_best_tag = i;
		}
	}

	// for now -- default
	if (best_tag == -1)
		best_tag = _tagSet->getNoneTagIndex();
	if (second_best_tag == -1)
		second_best_tag = _tagSet->getNoneTagIndex();

	if (DEBUG) {
		_debugStream << "*********************************\n";
		printDebugInfo(observation, best_tag);
		_debugStream << L"\n";
		if (second_best_tag != best_tag) {
			printDebugInfo(observation, second_best_tag);
			_debugStream << L"\n";
		}
		if (best_tag != _tagSet->getNoneTagIndex() &&
			second_best_tag != _tagSet->getNoneTagIndex())
		{
			printDebugInfo(observation, _tagSet->getNoneTagIndex());
			_debugStream << L"\n";
		}

	}

	// this will never be true if _overgen_percentage == 0
	if (best_tag ==  _tagSet->getNoneTagIndex() &&
		((best_score - second_best_score) / best_score < _overgen_percentage))
	{
		finalscore = second_best_score;
		return second_best_tag;
	}

	// likewise, this will never be true if _undergen_percentage == 0
	if (best_tag != _tagSet->getNoneTagIndex() &&
		second_best_tag == _tagSet->getNoneTagIndex() &&
		((best_score - second_best_score) / best_score < _undergen_percentage))
	{
		finalscore = second_best_score;
		return _tagSet->getNoneTagIndex();
	}

	finalscore = best_score;
	return best_tag;
}

int P1Decoder::decodeToInt(DTObservation *observation, double *tagScores, double& finalscore) {
	int n_tags = _tagSet->getNTags();

	double best_score = -10000000000000LL; //-10000000;
	int best_tag = -1;
	double second_best_score = -10000000000000LL; //-10000000;
	int second_best_tag = -1;
	for (int i = 0; i < n_tags; i++) {
		if (!observation->isValidTag(_tagSet->getTagSymbol(i)))
			continue;
		double score = tagScores[i];
		if (score > best_score) {
			second_best_score = best_score;
			second_best_tag = best_tag;
			best_score = score;
			best_tag = i;
		} else if (score > second_best_score) {
			second_best_score = score;
			second_best_tag = i;
		}
	}

	// for now -- default
	if (best_tag == -1)
		best_tag = _tagSet->getNoneTagIndex();
	if (second_best_tag == -1)
		second_best_tag = _tagSet->getNoneTagIndex();

	if (DEBUG) {
		_debugStream << "*********************************\n";
		printDebugInfo(observation, best_tag);
		_debugStream << L"\n";
		if (second_best_tag != best_tag) {
			printDebugInfo(observation, second_best_tag);
			_debugStream << L"\n";
		}
		if (best_tag != _tagSet->getNoneTagIndex() &&
			second_best_tag != _tagSet->getNoneTagIndex())
		{
			printDebugInfo(observation, _tagSet->getNoneTagIndex());
			_debugStream << L"\n";
		}

	}

	// this will never be true if _overgen_percentage == 0
	if (best_tag ==  _tagSet->getNoneTagIndex() &&
		((best_score - second_best_score) / best_score < _overgen_percentage))
	{
		finalscore = second_best_score;
		return second_best_tag;
	}

	// likewise, this will never be true if _undergen_percentage == 0
	if (best_tag != _tagSet->getNoneTagIndex() &&
		second_best_tag == _tagSet->getNoneTagIndex() &&
		((best_score - second_best_score) / best_score < _undergen_percentage))
	{
		finalscore = second_best_score;
		return _tagSet->getNoneTagIndex();
	}

	finalscore = best_score;
	return best_tag;
}

int P1Decoder::decodeToInt(DTObservation *observation) {
	double finalscore;
	return decodeToInt(observation, finalscore);
}

int P1Decoder::decodeToInt(DTObservation *observation, double *tagScores) {
	double finalscore;
	return decodeToInt(observation, tagScores, finalscore);
}

double P1Decoder::getScore(DTObservation *observation, int tag) {
	if (!observation->isValidTag(_tagSet->getTagSymbol(tag)))
		return -10000;
	DTState state(_tagSet->getTagSymbol(tag), Symbol(),
		Symbol(), 0, std::vector<DTObservation*>(1, observation));
	return scoreState(state);
}

Symbol P1Decoder::decodeToSymbol(DTObservation *observation) {
	int tag = decodeToInt(observation);
	return _tagSet->getTagSymbol(tag);
}
Symbol P1Decoder::decodeToSymbol(DTObservation *observation, double& score) {
	int tag = decodeToInt(observation, score);
	return _tagSet->getTagSymbol(tag);
}

void P1Decoder::printTrainDebugInfo(DTObservation *observation, int correct_answer, double weight, bool is_feature_vector_dump) {
	if ((!is_feature_vector_dump && DEBUG) || (is_feature_vector_dump && _dumpStream)) { 
		DTState state(_tagSet->getTagSymbol(correct_answer), Symbol(),
			Symbol(), 0, std::vector<DTObservation*>(1, observation));
		DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];

		
		if (is_feature_vector_dump) {
			(*_dumpStream) << _tagSet->getTagSymbol(correct_answer).to_string() ;
		} else {
			_debugStream << _tagSet->getTagSymbol(correct_answer).to_string() << L"\n";
		}

		for (int i = 0; i < _featureTypes[correct_answer]->getNFeaturesTypes(); i++) {
			int n_features = _featureTypes[correct_answer]->getFeatureType(i)->extractFeatures(state, featureArray);
			for (int j = 0; j < n_features; j++) {
				if (is_feature_vector_dump) {
					(*_dumpStream) << " (" << featureArray[j]->getFeatureType()->getName().to_string();
					featureArray[j]->write(*_dumpStream);
					(*_dumpStream) << ")";
				} else {
					_debugStream << featureArray[j]->getFeatureType()->getName().to_string() << L": ";
					featureArray[j]->write(_debugStream);
					_debugStream << L"\n";
				}

			}
		}

		if (is_feature_vector_dump) {
			(*_dumpStream) << "\n";
		} else {
			_debugStream << L"\n";
		}
	}
}

bool P1Decoder::train(DTObservation *observation, int correct_answer, double weight)
{
	if (DEBUG) {
		printTrainDebugInfo(observation, correct_answer, weight, false);
	}
	if (_dumpStream) {
		printTrainDebugInfo(observation, correct_answer, weight, true);
	}

	int hypothesis = decodeToInt(observation);

	if (hypothesis != correct_answer) {
		if (_real_averaged_mode)
			computeAverage();
		adjustWeights(observation, hypothesis, -weight, _add_hyp_features);
		adjustWeights(observation, correct_answer, weight, true);
	}
	if (_real_averaged_mode) {
		advanceInLife();
	}

	return (hypothesis == correct_answer);
}

void P1Decoder::addFeatures(DTObservation *observation, int correct_answer, int default_value) {

	DTState state(_tagSet->getTagSymbol(correct_answer), Symbol(),
		Symbol(), 0, std::vector<DTObservation*>(1, observation));

	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];

	for (int i = 0; i < _featureTypes[correct_answer]->getNFeaturesTypes(); i++) {
		int n_features = _featureTypes[correct_answer]->getFeatureType(i)->extractFeatures(state, featureArray);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = featureArray[j];
			DTFeature::FeatureWeightMap::iterator iter = _weights->find(feature);
			if (DEBUG) {
				_debugStream << feature->getFeatureType()->getName().to_string() << L": ";
				feature->write(_debugStream);
				_debugStream << L"\n";
			}
			if (iter != _weights->end()) {
				if (default_value > *(*iter).second)
					*(*iter).second = default_value;
				feature->deallocate();
			}
			else {
				// add the feature to the table, and don't delete it
				*(*_weights)[feature] = default_value;
			}
		}
	}

	if (correct_answer != _tagSet->getNoneTagIndex()) {
		addFeatures(observation, _tagSet->getNoneTagIndex(), 0);
	}

}

double P1Decoder::scoreState(const DTState &state) {
	return scoreState(state, _tagSet->getTagIndex(state.getTag()));
}

double P1Decoder::scoreState(const DTState &state, int tag) {
	double result = 0;
	//std::cerr<<"in scoreState state:"<<tag<<std::endl;
	//std::cerr<<"feature types:"<<_featureTypes<<std::endl;
	//std::cerr<<"feature type["<<tag<<"]:"<<_featureTypes[tag]->getFeatureType(0)->getName()<<std::endl;
	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	

	for (int i = 0; i < _featureTypes[tag]->getNFeaturesTypes(); i++) {
          int n_features = _featureTypes[tag]->getFeatureType(i)->extractFeatures(state, featureArray);
		  if (n_features > (DTFeatureType::MAX_FEATURES_PER_EXTRACTION - 5)) {
				//std::cerr<<"featuretype had size "<<n_features<<" near limit of "<<DTFeatureType::MAX_FEATURES_PER_EXTRACTION<<std::endl;
				//std::cerr<<"in scoreState state:"<<i<<std::endl;
				//std::cerr<<"feature type["<<tag<<"]:"<<_featureTypes[tag]->getFeatureType(i)->getName()<<std::endl;
				//std::cerr.flush();
		  }
          for (int j = 0; j < n_features; j++) {
            PWeight* res = _weights->get(featureArray[j]);
            if (res) 
              result += **res;
            featureArray[j]->deallocate();
          }
	}

	return result;
}


std::set<int> P1Decoder::featureInTable(DTObservation *observation, Symbol feature_name){
	std::set<int> known_tags;
	for(int tno = 0; tno <_tagSet->getNTags(); tno++){
		DTState state(_tagSet->getTagSymbol(tno), Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, observation));
		DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
		for (int fno = 0; fno < _featureTypes[tno]->getNFeaturesTypes(); fno++) {
			if (_featureTypes[tno]->getFeatureType(fno)->getName() == feature_name) {
				int n_features = _featureTypes[tno]->getFeatureType(fno)->extractFeatures(state, featureArray);
				for (int i = 0; i < n_features; i++) {
					DTFeature::FeatureWeightMap::iterator iter = _weights->find(featureArray[i]);
					if (iter != _weights->end()) {
						known_tags.insert(tno);
						break;
					}
				}
				for (int i = 0; i < n_features; i++)
					featureArray[i]->deallocate();
			}
		}
	}
	
	return known_tags;
}

void P1Decoder::adjustErrorWeights(DTObservation *hypothesis_observation, int hypothsys_tag
								   , DTObservation *correct_observation, int correct_tag
								   , double delta, bool add_unseen_features)
{
	adjustWeights(correct_observation, correct_tag, delta, true);
	adjustWeights(hypothesis_observation, hypothsys_tag, -delta, add_unseen_features);
}


void P1Decoder::adjustWeights(DTObservation *observation, int answer, double delta,
					   bool add_unseen_features)
{
	if (answer < 0)
		throw InternalInconsistencyException("P1Decoder::adjustWeights", "Negative answer index while training");

	DTState state(_tagSet->getTagSymbol(answer), Symbol(),
		Symbol(), 0, std::vector<DTObservation*>(1, observation));

	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];

	for (int i = 0; i < _featureTypes[answer]->getNFeaturesTypes(); i++) {
		int n_features = _featureTypes[answer]->getFeatureType(i)->extractFeatures(state, featureArray);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = featureArray[j];
			if (DEBUG) {
				_debugStream << "ADJUSTING ";
				_debugStream << featureArray[j]->getFeatureType()->getName().to_string() << L" ";
				featureArray[j]->write(_debugStream);
				_debugStream << " BY " << delta << "\n";
			}
			DTFeature::FeatureWeightMap::iterator iter = _weights->find(feature);
			if (iter != _weights->end()) {
				*(*iter).second += delta;
				// the feature is already in the hash table, so delete this
				// copy of it
				feature->deallocate();
			}
			else if (add_unseen_features) {
				// add the feature to the table, and don't delete it
				*(*_weights)[feature] = delta;
			}
		}
	}
}

void P1Decoder::computeAverage() {
	if (_hypothesis_life == 0)
		return;
	for (DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
		iter != _weights->end(); ++iter)
	{
		(*iter).second.addToSum(_hypothesis_life);
	}
	_hypothesis_life = 0;
}


std::wstring P1Decoder::getDebugInfo(DTObservation *observation, int answer) const {
	std::wstringstream result;
	double score = 0;
	DTState state(_tagSet->getTagSymbol(answer), Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, observation));
	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	
	result << _tagSet->getTagSymbol(answer).to_string() << L":\n";

	for (int i = 0; i < _featureTypes[answer]->getNFeaturesTypes(); i++) {
		int n_features = _featureTypes[answer]->getFeatureType(i)->extractFeatures(state, featureArray);
		for (int j = 0; j < n_features; j++) {
			wstring wstr;
			featureArray[j]->toString(wstr);
			result << featureArray[j]->getFeatureType()->getName().to_string() << L" ";
			result << wstr;
			result << L": ";
			DTFeature::FeatureWeightMap::iterator iter = _weights->find(featureArray[j]);
			if (iter != _weights->end()) {
				result << *(*iter).second;
				score += *(*iter).second;
			} else result << "NOT IN TABLE";
			result << L"\n";
			featureArray[j]->deallocate();
		}
	}
	result << L"SCORE: " << score << L"\n";

	return result.str();
}

void P1Decoder::printDebugInfo(DTObservation *observation, int answer) {
	if (answer < 0)
		return;
	if (DEBUG) {
		_debugStream << getDebugInfo(observation, answer);
	}
}
void P1Decoder::printDebugInfo(DTObservation *observation, int answer, UTF8OutputStream& debug){
	debug << getDebugInfo(observation, answer);
}

void P1Decoder::printDebugInfo(DTObservation *observation, int answer, DebugStream& debug){
	debug << getDebugInfo(observation, answer);
}

void P1Decoder::printHTMLDebugInfo(DTObservation *observation, int answer, UTF8OutputStream& debug){
	double score = 0;
	DTState state(_tagSet->getTagSymbol(answer), Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, observation));
	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];

	for (int i = 0; i < _featureTypes[answer]->getNFeaturesTypes(); i++) {
		int n_features = _featureTypes[answer]->getFeatureType(i)->extractFeatures(state, featureArray);
		for (int j = 0; j < n_features; j++) {
			debug << featureArray[j]->getFeatureType()->getName().to_string() << L" ";
			std::wstring str;
			featureArray[j]->toString(str);
			for (unsigned ch = 0; ch < str.length(); ch++) {
				if (str.at(ch) == L'<')
					str.replace(ch, 1, L"[");
				if (str.at(ch) == L'>')
					str.replace(ch, 1, L"]");
			}
			debug << str;
			debug << L": ";
			DTFeature::FeatureWeightMap::iterator iter = _weights->find(featureArray[j]);
			if (iter != _weights->end()) {
				debug << *(*iter).second;
				score += *(*iter).second;
			} else debug << "NOT IN TABLE";
			debug << L"<br>\n";
			featureArray[j]->deallocate();
		}
	}
}


void P1Decoder::printScoreInfo(DTObservation* observation,
							   int correct_answer, int incorrect_answer,
							   UTF8OutputStream& uos){
		DTState cstate(_tagSet->getTagSymbol(correct_answer), Symbol(),
			Symbol(), 0, std::vector<DTObservation*>(1, observation));
		double correct_score = scoreState(cstate);
		DTState istate(_tagSet->getTagSymbol(incorrect_answer), Symbol(),
			Symbol(), 0, std::vector<DTObservation*>(1, observation));
		double incorrect_score = scoreState(istate);

		uos <<_tagSet->getTagSymbol(correct_answer).to_string()<<" \t"
			<<correct_score<<" \t"<<incorrect_score<<"\n";
		printDebugInfo(observation, correct_answer, uos);

}




void P1Decoder::printScoreInfo(DTObservation* observation,
							   int answer, UTF8OutputStream& uos){
		DTState state(_tagSet->getTagSymbol(answer), Symbol(),
			Symbol(), 0, std::vector<DTObservation*>(1, observation));
		double correct_score = scoreState(state);
		uos <<_tagSet->getTagSymbol(answer).to_string()<<" \t"
			<<correct_score<<"\n";

}

void P1Decoder::setLogFile(const string& logfile) {
	_dumpStream = _new UTF8OutputStream();
	_dumpStream->open(logfile.c_str());
}

