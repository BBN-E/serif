// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/relations/ch_RelationModel.h"
#include "Chinese/relations/ch_RelationUtilities.h"
#include "Generic/common/Symbol.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Chinese/relations/ch_PotentialRelationCollector.h"
#include "Generic/relations/VectorModel.h"
#include "Chinese/relations/ch_OldMaxEntRelationModel.h"
#include "Chinese/relations/ch_PotentialRelationInstance.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/driver/DocumentDriver.h"
#include <boost/scoped_ptr.hpp>


const Symbol BAD_EDT_SYM = Symbol(L"BAD EDT");
const Symbol SAME_ENTITY_SYM = Symbol(L"SAME ENTITY");
const Symbol NEG_HYPO_SYM = Symbol(L"NEG/HYPO");

ChineseRelationModel::ChineseRelationModel() : _vectorModel(0), _maxEntModel(0)
{
	_vectorModel = _new VectorModel();
	//_maxEntModel = _new OldMaxEntRelationModel();
}

ChineseRelationModel::ChineseRelationModel(const char *file_prefix) : _vectorModel(0), _maxEntModel(0)
{
	//_vectorModel = _new VectorModel(file_prefix);
	_maxEntModel = _new OldMaxEntRelationModel(file_prefix);
}

ChineseRelationModel::~ChineseRelationModel() {
	delete _vectorModel;
	delete _maxEntModel;
}

void ChineseRelationModel::generateVectorsAndTrain(char *packet_file, 
											char* vector_file, 
											char* output_file_prefix)
{
	// read in PotentialTrainingRelations from packet_file annotation
	// MEM: must delete annotation set when finished
	PotentialRelationMap *annotationSet = readPacketAnnotation(packet_file);

	// extract PotentialTrainingRelations from batch  
	PotentialRelationCollector* resultCollector = PotentialRelationCollector::build(ChinesePotentialRelationCollector::TRAIN);
	extractTrainingRelations(*resultCollector);

	NgramScoreTable vectors(CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE, resultCollector->getNRelations());
	for (int i = 0; i < resultCollector->getNRelations(); i++) {
		PotentialTrainingRelation *rel = resultCollector->getPotentialTrainingRelation(i);
		//_debugStream << rel->toString();
		PotentialRelationMap::iterator it = annotationSet->find(*rel);
		if (it != annotationSet->end())  {
			//_debugStream << L"Found it!\n";		
			PotentialRelationInstance *instance = resultCollector->getPotentialRelationInstance(i);
			instance->setRelationType((*it).second);
			instance->setReverse((((*it).first).relationIsReversed() || rel->argsAreReversed()) &&
							    !(((*it).first).relationIsReversed() && rel->argsAreReversed())); 
			if (instance->getRelationType() != BAD_EDT_SYM &&
				instance->getRelationType() != SAME_ENTITY_SYM &&
				instance->getRelationType() != NEG_HYPO_SYM)
			{
				vectors.add(instance->getTrainingNgram());
			}
			//_debugStream << instance->toString() << L"\n";
		}	
		//else 	_debugStream << L"Didn't find it!\n";

	}

	vectors.print(vector_file);

	//train(vector_file, output_file_prefix);

	delete annotationSet;
}

ChineseRelationModel::PotentialRelationMap *ChineseRelationModel::readPacketAnnotation(const char *packet_file) {
	PotentialRelationMap *annotationSet = _new PotentialRelationMap(1024);
	boost::scoped_ptr<UTF8InputStream> packet_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& packet(*packet_scoped_ptr);
	wchar_t line[200];

	packet.open(packet_file);
	if (packet.fail()) {
		char message[300];
		sprintf(message, "Could not open packet file `%s'", packet_file);
		throw UnexpectedInputException("ChineseRelationModel::readPacketAnnotation()", message);
	}
	packet.getLine(line, 200);
	int n_packet_files = _wtoi(line);
	for (int i = 0; i < n_packet_files; i++) {
		PotentialTrainingRelation rel;
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);

		if (packet.eof()) {
			char message[300];
			sprintf(message, "Reached unexpected end of `%s'", packet_file);
			throw UnexpectedInputException("ChineseRelationModel::readPacketAnnotation()",  message);
		}
		packet.getLine(line, 200);	
		in.open(line);
		if (in.fail()) {
			char message[300];
			sprintf(message, "Could not open annotation file `%s'", UnicodeUtil::toUTF8String(line));
			throw UnexpectedInputException("ChineseRelationModel::readPacketAnnotation()", message);
		}
		in.getLine(line, 200);
		int n_relations = _wtoi(line);
		for (int j = 0; j < n_relations; j++) {
			in >> rel;
			(*annotationSet)[rel] = rel.getRelationType();
		}
		in.close();

	}
	packet.close();
	return annotationSet;
}

void ChineseRelationModel::extractTrainingRelations(PotentialRelationCollector &resultCollector) {
	SessionProgram sessionProgram;
	sessionProgram.setStageActiveness(Stage("sent-break"), true);
	sessionProgram.setStageActiveness(Stage("tokens"), true);
	sessionProgram.setStageActiveness(Stage("parse"), true);
	sessionProgram.setStageActiveness(Stage("mentions"), true);
	sessionProgram.setStageActiveness(Stage("props"), true);
	sessionProgram.setStageActiveness(Stage("metonymy"), true);
	sessionProgram.setStageActiveness(Stage("entities"), true);
	sessionProgram.setStageActiveness(Stage("events"), false);
	sessionProgram.setStageActiveness(Stage("relations"), false);
	sessionProgram.setStageActiveness(Stage("doc-entities"), false);
	sessionProgram.setStageActiveness(Stage("doc-relations-events"), false);
	sessionProgram.setStageActiveness(Stage("generics"), false);
	sessionProgram.setStageActiveness(Stage("output"), true);
	sessionProgram.setStageActiveness(Stage("score"), false);
	sessionProgram.setStageActiveness(Stage("END"), false);
	
	DocumentDriver documentDriver(&sessionProgram,
			  					  &resultCollector);
	documentDriver.run();

}


void ChineseRelationModel::train(char *training_file, char* output_file_prefix) {
	std::cerr << "training on " << training_file << std::endl;
	//_vectorModel->train(training_file, output_file_prefix);
	_maxEntModel->train(training_file, output_file_prefix);
}

void ChineseRelationModel::trainFromStateFileList(char *training_file, char* output_file_prefix)
{
	std::cerr << "training on " << training_file << std::endl;
	_vectorModel->trainFromStateFileList(training_file, output_file_prefix);
}

void ChineseRelationModel::test(char *test_vector_file) {
	_maxEntModel->testModel(test_vector_file); 
}

int ChineseRelationModel::findBestRelationType(PotentialRelationInstance *instance) {
	//return _vectorModel->findBestRelationType(instance);
	return _maxEntModel->findBestRelationType((ChinesePotentialRelationInstance *)instance);
}


