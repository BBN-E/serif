// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h"

#include "Generic/events/stat/ETProbModelSet.h"
#include "Generic/common/ParamReader.h"
#include <boost/scoped_ptr.hpp>


ETProbModelSet::ETProbModelSet(DTTagSet *tagSet, double *tagScores,
							   double recall_threshold) 
	: _tagSet(tagSet), _tagScores(tagScores), _recall_threshold(recall_threshold),
	_wordModel(0), _wordPriorModel(0), _objModel(0), _subModel(0),
	_wordNetModel(0), _cluster16Model(0), _cluster1216Model(0),
	_cluster81216Model(0)
{
	_use_model[WORD] = ParamReader::getRequiredTrueFalseParam("use_word_model");
	_use_model[OBJ] = ParamReader::getRequiredTrueFalseParam("use_obj_model");
	_use_model[SUB] = ParamReader::getRequiredTrueFalseParam("use_sub_model");
	_use_model[WNET] = ParamReader::getRequiredTrueFalseParam("use_wordnet_model");
	_use_model[CLUSTER16] = ParamReader::getRequiredTrueFalseParam("use_cluster16_model");
	_use_model[CLUSTER1216] = ParamReader::getRequiredTrueFalseParam("use_cluster1216_model");
	_use_model[CLUSTER81216] = ParamReader::getRequiredTrueFalseParam("use_cluster81216_model");
	_use_model[WORD_PRIOR] = ParamReader::getRequiredTrueFalseParam("use_word_prior_model");
	//_use_model[XXX] = ParamReader::getRequiredTrueFalseParam("use_xxx_model");
	//_use_model[XXX] = ParamReader::getRequiredTrueFalseParam("use_xxx_model");
	//_use_model[XXX] = ParamReader::getRequiredTrueFalseParam("use_xxx_model");
	//_use_model[XXX] = ParamReader::getRequiredTrueFalseParam("use_xxx_model");
	_n_models = 8;
}

int ETProbModelSet::decodeToDistribution(EventTriggerObservation *observation, int mt) {
	double best_score = -20000;
	int best_tag = 0;
	double second_best_score = -20000;
	int second_best_tag;
	for (int i = 0; i < _tagSet->getNTags(); i++) {
		_tagScores[i] = getProbability(observation, _tagSet->getTagSymbol(i), mt);
		if (_tagScores[i] > best_score) {
			second_best_score = best_score;
			second_best_tag = best_tag;
			best_score = _tagScores[i];
			best_tag = i;
		} else if (_tagScores[i] > second_best_score) {
			second_best_score = _tagScores[i];
			second_best_tag = i;
		}
	}
	if (best_tag == _tagSet->getNoneTagIndex() && second_best_score > log(_recall_threshold)){
		std::wcout<<"Choosing second best tag!"<<std::endl;
		return second_best_tag;
	}
	else return best_tag;
}

void ETProbModelSet::printDebugScores(UTF8OutputStream& out, EventTriggerObservation *observation, int mt) {
	int selected_tag = decodeToDistribution(observation, mt);
	int best = 0;
	int second_best = 0;
	int third_best = 0;
	double best_score = -100000;
	double second_best_score = -100000;
	double third_best_score = -100000;
	for (int i = 0; i < _tagSet->getNTags(); i++) {
		if (_tagScores[i] > best_score) {
			third_best = second_best;
			third_best_score = second_best_score;
			second_best = best;
			second_best_score = best_score;
			best = i;
			best_score = _tagScores[i];
		} else if (_tagScores[i] > second_best_score) {
			third_best = second_best;
			third_best_score = second_best_score;
			second_best = i;			
			second_best_score = _tagScores[i];
		} else if (_tagScores[i] > third_best_score) {
			third_best = i;
			third_best_score = _tagScores[i];
		} 
	}
	
	if(mt  == ETProbModelSet::SUB) out <<L"ETPM-SUB: <br>\n";
	else if(mt  == ETProbModelSet::OBJ) out <<L"ETPM-OBJ: <br>\n";
	else out <<L"ETPM-"<<mt<<L" <br>\n";
	out << _tagSet->getTagSymbol(best).to_string() << L": " << _tagScores[best];
	if(selected_tag == best)
		out <<" ---Selected";
	out	<< L"<br>\n";
	out << _tagSet->getTagSymbol(second_best).to_string() << L": " << _tagScores[second_best] ;
	if(selected_tag == second_best)
		out <<" ---Selected";
	out	<< L"<br>\n";
	out << _tagSet->getTagSymbol(third_best).to_string() << L": " << _tagScores[third_best] << L"<br>\n";
	out << "<br>\n";
}

void ETProbModelSet::getModelName(int mt, const char *model_prefix, char *buffer) {
	if (mt == WORD)
		sprintf(buffer, "%s.word", model_prefix);
	else if (mt == OBJ)
		sprintf(buffer, "%s.obj", model_prefix);
	else if (mt == SUB)
		sprintf(buffer, "%s.sub", model_prefix);
	else if (mt == WNET)
		sprintf(buffer, "%s.wordnet", model_prefix);
	else if (mt == CLUSTER16)
		sprintf(buffer, "%s.cluster16", model_prefix);
	else if (mt == CLUSTER1216)
		sprintf(buffer, "%s.cluster1216", model_prefix);
	else if (mt == CLUSTER81216)
		sprintf(buffer, "%s.cluster81216", model_prefix);
	else if (mt == WORD_PRIOR)
		sprintf(buffer, "%s.wordprior", model_prefix);
	//else if (mt == XXX)
	//	sprintf(buffer, "%s.xxx", model_prefix);
	//else if (mt == XXX)
	//	sprintf(buffer, "%s.xxx", model_prefix);
	//else if (mt == XXX)
	//	sprintf(buffer, "%s.xxx", model_prefix);
	//else if (mt == XXX)
	//	sprintf(buffer, "%s.xxx", model_prefix);
	else sprintf(buffer, "%s", model_prefix);
}

void ETProbModelSet::loadModel(int mt, const char *model_prefix) {
	char buffer[550];
	getModelName(mt, model_prefix, buffer);		
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(buffer));
	UTF8InputStream& stream(*stream_scoped_ptr);
	if (mt == WORD)
		_wordModel = _new type_et_word_model_t(stream);
	else if (mt == OBJ)
		_objModel = _new type_et_obj_model_t(stream);
	else if (mt == SUB)
		_subModel = _new type_et_sub_model_t(stream);
	else if (mt == WNET)
		_wordNetModel = _new type_et_wordnet_model_t(stream);
	else if (mt == CLUSTER16)
		_cluster16Model = _new type_et_cluster16_model_t(stream);
	else if (mt == CLUSTER1216)
		_cluster1216Model = _new type_et_cluster1216_model_t(stream);
	else if (mt == CLUSTER81216)
		_cluster81216Model = _new type_et_cluster81216_model_t(stream);
	else if (mt == WORD_PRIOR)
		_wordPriorModel = _new type_et_word_prior_model_t(stream);
	//else if (mt == XXX)
	//	_xxxModel = _new type_et_xxx_model_t(stream);
	//else if (mt == XXX)
	//	_xxxModel = _new type_et_xxx_model_t(stream);
	//else if (mt == XXX)
	//	_xxxModel = _new type_et_xxx_model_t(stream);
	//else if (mt == XXX)
	//	_xxxModel = _new type_et_xxx_model_t(stream);
	stream.close();
}	

void ETProbModelSet::printModel(int mt, const char *model_prefix) {
	char buffer[550];
	getModelName(mt, model_prefix, buffer);
	UTF8OutputStream stream(buffer);
	if (mt == WORD)
		_wordModel->print(stream);
	else if (mt == OBJ)
		_objModel->print(stream);
	else if (mt == SUB)
		_subModel->print(stream);
	else if (mt == WNET)
		_wordNetModel->print(stream);
	else if (mt == CLUSTER16)
		_cluster16Model->print(stream);
	else if (mt == CLUSTER1216)
		_cluster1216Model->print(stream);
	else if (mt == CLUSTER81216)
		_cluster81216Model->print(stream);
	else if (mt == WORD_PRIOR)
		_wordPriorModel->print(stream);
	//else if (mt == XXX)
	//	_xxxModel->print(stream);
	//else if (mt == XXX)
	//	_xxxModel->print(stream);
	//else if (mt == XXX)
	//	_xxxModel->print(stream);
	//else if (mt == XXX)
	//	_xxxModel->print(stream);
	stream.close();
}

void ETProbModelSet::createEmptyModel(int mt) {
	if (mt == WORD)
		_wordModel = _new type_et_word_model_t();
	else if (mt == OBJ)
		_objModel = _new type_et_obj_model_t();
	else if (mt == SUB)
		_subModel = _new type_et_sub_model_t();
	else if (mt == WNET)
		_wordNetModel = _new type_et_wordnet_model_t();
	else if (mt == CLUSTER16)
		_cluster16Model = _new type_et_cluster16_model_t();
	else if (mt == CLUSTER1216)
		_cluster1216Model = _new type_et_cluster1216_model_t();
	else if (mt == CLUSTER81216)
		_cluster81216Model = _new type_et_cluster81216_model_t();
	else if (mt == WORD_PRIOR)
		_wordPriorModel = _new type_et_word_prior_model_t();
	//else if (mt == XXX)
	//	_xxxModel = _new type_et_xxx_model_t();
	//else if (mt == XXX)
	//	_xxxModel = _new type_et_xxx_model_t();
	//else if (mt == XXX)
	//	_xxxModel = _new type_et_xxx_model_t();
	//else if (mt == XXX)
	//	_xxxModel = _new type_et_xxx_model_t();
}

void ETProbModelSet::clearModel(int mt) {
	if (mt == WORD) {
		_wordModel = 0;
	} else if (mt == OBJ) {
		_objModel = 0;
	} else if (mt == SUB) {
		_subModel = 0;
	} else if (mt == WNET) {
		_wordNetModel = 0;
	} else if (mt == CLUSTER16) {
		_cluster16Model = 0;
	} else if (mt == CLUSTER1216) {
		_cluster1216Model = 0;
	} else if (mt == CLUSTER81216) {
		_cluster81216Model = 0;		
	} else if (mt == WORD_PRIOR) {
		_wordPriorModel = 0;
	//} else if (mt == XXX) {
	//	_xxxModel = 0;
	//} else if (mt == XXX) {
	//	_xxxModel = 0;
	//} else if (mt == XXX) {
	//	_xxxModel = 0;
	//} else if (mt == XXX) {
	//	_xxxModel = 0;
	} 
}

void ETProbModelSet::deleteModel(int mt) {
	if (mt == WORD) {
		delete _wordModel;
	} else if (mt == OBJ) {
		delete _objModel;
	} else if (mt == SUB) {
		delete _subModel;
	} else if (mt == WNET) {
		delete _wordNetModel;
	} else if (mt == CLUSTER16) {
		delete _cluster16Model;
	} else if (mt == CLUSTER1216) {
		delete _cluster1216Model;
	} else if (mt == CLUSTER81216) {
		delete _cluster81216Model;
	} else if (mt == WORD_PRIOR) {
		delete _wordPriorModel;
	//} else if (mt == XXX) {
	//	delete _xxxModel;
	//} else if (mt == XXX) {
	//	delete _xxxModel;
	//} else if (mt == XXX) {
	//	delete _xxxModel;
	//} else if (mt == XXX) {
	//	delete _xxxModel;
	} 
	clearModel(mt);
}

double ETProbModelSet::getProbability(EventTriggerObservation *observation, Symbol et, int mt) {
	if (mt == WORD)
		return _wordModel->getProbability(observation, et);
	else if (mt == OBJ)
		return _objModel->getProbability(observation, et);
	else if (mt == SUB)
		return _subModel->getProbability(observation, et);
	else if (mt == WNET)
		return _wordNetModel->getProbability(observation, et);
	else if (mt == CLUSTER16)
		return _cluster16Model->getProbability(observation, et);
	else if (mt == CLUSTER1216)
		return _cluster1216Model->getProbability(observation, et);
	else if (mt == CLUSTER81216)
		return _cluster81216Model->getProbability(observation, et);
	else if (mt == WORD_PRIOR)
		return _wordPriorModel->getProbability(observation, et);	
	//else if (mt == XXX)
	//	return _xxxModel->getProbability(observation, et);	
	//else if (mt == XXX)
	//	return _xxxModel->getProbability(observation, et);	
	//else if (mt == XXX)
	//	return _xxxModel->getProbability(observation, et);	
	//else if (mt == XXX)
	//	return _xxxModel->getProbability(observation, et);	
	else return 0;
}

double ETProbModelSet::getLambdaForFullHistory(EventTriggerObservation *observation, 
											   Symbol et, int mt) const
{
	if (mt == WORD)
		return _wordModel->getLambdaForFullHistory(observation, et);
	else if (mt == OBJ)
		return _objModel->getLambdaForFullHistory(observation, et);
	else if (mt == SUB)
		return _subModel->getLambdaForFullHistory(observation, et);
	else if (mt == WNET)
		return _wordNetModel->getLambdaForFullHistory(observation, et);
	else if (mt == CLUSTER16)
		return _cluster16Model->getLambdaForFullHistory(observation, et);
	else if (mt == CLUSTER1216)
		return _cluster1216Model->getLambdaForFullHistory(observation, et);
	else if (mt == CLUSTER81216)
		return _cluster81216Model->getLambdaForFullHistory(observation, et);
	else if (mt == WORD_PRIOR)
		return _wordPriorModel->getLambdaForFullHistory(observation, et);	
	//else if (mt == XXX)
	//	return _xxxModel->getLambdaForFullHistory(observation, et);	
	//else if (mt == XXX)
	//	return _xxxModel->getLambdaForFullHistory(observation, et);	
	//else if (mt == XXX)
	//	return _xxxModel->getLambdaForFullHistory(observation, et);	
	//else if (mt == XXX)
	//	return _xxxModel->getLambdaForFullHistory(observation, et);	
	else return 0;
}

void ETProbModelSet::addEvent(EventTriggerObservation *observation, Symbol et, int mt) {
	if (mt == WORD)
		return _wordModel->addEvent(observation, et);
	else if (mt == OBJ)
		return _objModel->addEvent(observation, et);
	else if (mt == SUB)
		return _subModel->addEvent(observation, et);
	else if (mt == WNET)
		return _wordNetModel->addEvent(observation, et);
	else if (mt == CLUSTER16)
		return _cluster16Model->addEvent(observation, et);
	else if (mt == CLUSTER1216)
		return _cluster1216Model->addEvent(observation, et);
	else if (mt == CLUSTER81216)
		return _cluster81216Model->addEvent(observation, et);
	else if (mt == WORD_PRIOR)
		return _wordPriorModel->addEvent(observation, et);
	//else if (mt == XXX)
	//	return _xxxModel->addEvent(observation, et);
	//else if (mt == XXX)
	//	return _xxxModel->addEvent(observation, et);
	//else if (mt == XXX)
	//	return _xxxModel->addEvent(observation, et);
	//else if (mt == XXX)
	//	return _xxxModel->addEvent(observation, et);
	else return;
}

void ETProbModelSet::deriveModel(int mt) {
	if (mt == WORD)
		return _wordModel->deriveModel();
	else if (mt == OBJ)
		return _objModel->deriveModel();
	else if (mt == SUB)
		return _subModel->deriveModel();
	else if (mt == WNET)
		return _wordNetModel->deriveModel();
	else if (mt == CLUSTER16)
		return _cluster16Model->deriveModel();
	else if (mt == CLUSTER1216)
		return _cluster1216Model->deriveModel();
	else if (mt == CLUSTER81216)
		return _cluster81216Model->deriveModel();
	else if (mt == WORD_PRIOR)
		return _wordPriorModel->deriveModel();
	//else if (mt == XXX)
	//	return _xxxModel->deriveModel();
	//else if (mt == XXX)
	//	return _xxxModel->deriveModel();
	//else if (mt == XXX)
	//	return _xxxModel->deriveModel();
	//else if (mt == XXX)
	//	return _xxxModel->deriveModel();
	else return;
}

bool ETProbModelSet::preferModel(EventTriggerObservation *observation, int mt) const{
	if(!_use_model[mt]) return false;
	double lambda = getLambdaForFullHistory(observation, _tagSet->getNoneTag(), mt);
	if(mt == ETProbModelSet::SUB && (observation->getSubjectOfTrigger().is_null() || lambda <= 0.8))
		return false;
	if(mt == ETProbModelSet::OBJ && (observation->getObjectOfTrigger().is_null() || lambda <= 0.8))
		return false;
	return true;
}
