// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/relations/ar_RelationModel.h"
#include "Generic/relations/VectorModel.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/NgramScoreTable.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Arabic/relations/ar_PotentialRelationCollector.h"
#include "Generic/preprocessors/EntityTypes.h"
#include "Generic/preprocessors/RelationTypeMap.h"
#include "Arabic/relations/ar_PotentialRelationExtractor.h"
#include "Arabic/relations/ar_PotentialRelationInstance.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/driver/DocumentDriver.h"
#include "Arabic/common/ar_StringTransliterator.h"
#include <boost/scoped_ptr.hpp>

const Symbol BAD_EDT_SYM = Symbol(L"BAD EDT");
const Symbol SAME_ENTITY_SYM = Symbol(L"SAME ENTITY");
const Symbol NEG_HYPO_SYM = Symbol(L"NEG/HYPO");

ArabicRelationModel::ArabicRelationModel() :  _maxEntModel(0)
{
	//_maxEntModel = _new MaxEntRelationModel();

	_debugStream.init(Symbol(L"relations_debug"));
	_vectorModel = _new VectorModel();
}

ArabicRelationModel::ArabicRelationModel(const char *file_prefix) : _maxEntModel(0)
{
	//_maxEntModel = _new MaxEntRelationModel(file_prefix);

	std::string debug_file = ParamReader::getParam("relations_debug");
	if (!debug_file.empty()) {
		_debugStream.openFile(debug_file.c_str());
	}
}

ArabicRelationModel::~ArabicRelationModel() {
	//delete _maxEntModel;
}

void ArabicRelationModel::trainFromStateFileList(char *training_file, char* output_file_prefix) {
	_vectorModel->trainFromStateFileList(training_file, output_file_prefix);

}


void ArabicRelationModel::generateVectorsAndTrain(char *packet_file,
											char* vector_file,
											char* output_file_prefix)
{
	// read in PotentialTrainingRelations from packet_file annotation
	// MEM: must delete annotation set when finished
	PotentialRelationMap *annotationSet = readPacketAnnotation(packet_file);

	// extract PotentialTrainingRelations from batch
	PotentialRelationCollector* resultCollector = PotentialRelationCollector::build(ArabicPotentialRelationCollector::TRAIN);
	extractTrainingRelations(*resultCollector);

	_debugStream << "Found " << resultCollector->getNRelations() << " potential relations.\n";
	NgramScoreTable vectors(AR_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE, resultCollector->getNRelations());
	for (int i = 0; i < resultCollector->getNRelations(); i++) {
		PotentialTrainingRelation *rel = resultCollector->getPotentialTrainingRelation(i);
		_debugStream << rel->toString();
		PotentialRelationMap::iterator it = annotationSet->find(*rel);
		if (it != annotationSet->end())  {
			_debugStream << L"Found it!\n";
			PotentialRelationInstance *instance = resultCollector->getPotentialRelationInstance(i);

			instance->setRelationType((*it).second);
			instance->setReverse((((*it).first).relationIsReversed() || rel->argsAreReversed()) &&
							    !(((*it).first).relationIsReversed() && rel->argsAreReversed()));

			// coerce entity types (we assume the annotation is correct)
			if (rel->_type1 != ((*it).first)._type1) {
				if (!rel->argsAreReversed())
					instance->setLeftEntityType((((*it).first)._type1).getName());
				else
					instance->setRightEntityType((((*it).first)._type1).getName());
			}
			if (rel->_type2 != ((*it).first)._type2) {
				if (!rel->argsAreReversed())
					instance->setRightEntityType((((*it).first)._type2).getName());
				else
					instance->setLeftEntityType((((*it).first)._type2).getName());
			}

			if (instance->getRelationType() != BAD_EDT_SYM &&
				instance->getRelationType() != SAME_ENTITY_SYM &&
				instance->getRelationType() != NEG_HYPO_SYM)
			{
				vectors.add(instance->getTrainingNgram());
			}
			_debugStream << instance->toString() << L"\n";
		}
		else 	_debugStream << L"Didn't find it!\n";

	}

	vectors.print(vector_file);

	//train(vector_file, output_file_prefix);

	delete annotationSet;
}


ArabicRelationModel::PotentialRelationMap *ArabicRelationModel::readPacketAnnotation(const char *packet_file) {
	PotentialRelationMap *annotationSet = _new PotentialRelationMap(1024);
	boost::scoped_ptr<UTF8InputStream> packet_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& packet(*packet_scoped_ptr);
	wchar_t line[200];

	packet.open(packet_file);
	if (packet.fail()) {

		char message[300];
		sprintf(message, "Could not open packet file `%s'", packet_file);
		throw UnexpectedInputException("ArabicRelationModel::readPacketAnnotation()", message);
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
			throw UnexpectedInputException("ArabicRelationModel::readPacketAnnotation()",  message);
		}
		packet.getLine(line, 200);
		in.open(line);
		if (in.fail()) {
			char message[300];
			char cline[300];
			ArabicStringTransliterator::transliterateToEnglish(cline, line, 300);
			sprintf(message, "Could not open annotation file `%s'", cline);
			throw UnexpectedInputException("ArabicRelationModel::readPacketAnnotation()", message);
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

void ArabicRelationModel::extractTrainingRelations(PotentialRelationCollector &resultCollector) {
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


/*void ArabicRelationModel::extractTrainingRelations(ArabicPotentialRelationCollector &resultCollector) {

	char input_list[501];
	char parse_dir[501];
	char entity_types[501];
	char line[200];
	char prefix[200];
	char parse_file[200];
	bool use_default;

	if (!ParamReader::getParam("input_list",input_list, 500))	{
		throw UnexpectedInputException("RelationMode::extractTrainingRelations()",
										"Param `input_list' not defined");
	}
	if (!ParamReader::getParam("parse_dir",parse_dir, 500))	{
		throw UnexpectedInputException("RelationMode::extractTrainingRelations()",
										"Param `parse_dir' not defined");
	}
	if (!ParamReader::getParam("entity_type_list",entity_types, 500))	{
		throw UnexpectedInputException("RelationMode::extractTrainingRelations()",
										"Param `entity_type_list' not defined");
	}
	if (ParamReader::getParam(Symbol(L"use_default_entity_types")) == Symbol(L"true"))
		use_default = true;
	else if (ParamReader::getParam(Symbol(L"use_default_entity_types")) == Symbol(L"false"))
		use_default = false;
	else {
		throw UnexpectedInputException("RelationMode::extractTrainingRelations()",
										"Param `use_default_entity_types' must be set to 'true' or 'false'");
	}

	EntityTypes entityTypes(entity_types, use_default);
	PotentialRelationExtractor extractor(&entityTypes, &resultCollector);

	ifstream in;
	in.open(input_list);
	if (in.fail()) {
		char message[300];
		sprintf(message, "Could not open input file list `%s'", input_list);
			throw UnexpectedInputException("ArabicRelationModel::extractTrainingRelations()", message);
	}
	while (!in.eof()) {
		in.getline(line, 200);
		if (line[0] != '\0' && line[0] != '#') {
			// strip \r if it's there
			char* c = line;
			while (*c != '\0' && c != &line[199]) {
				if (*c == '\r') {
					*c = '\0';
					break;
				}
				c++;
			}
			char *end;
			strcpy(prefix, line);
			char *short_prefix;
			if ((end = strstr(prefix, ".out.sgm")) != NULL)
				*end = '\0';
			int len = static_cast<int>(strlen(prefix));
			for (int i = len - 1; i >= 0; i--) {
				if (prefix[i] == '\\' || prefix[i] == '/') {
					short_prefix = &prefix[i + 1];
					break;
				}
			}
			sprintf(parse_file, "%s\\", parse_dir);
			strcat(parse_file, short_prefix);
			strcat(parse_file, ".arparse");
			// Process the document.
			extractor.processDocument(line, parse_file);
		}
	}
	in.close();
}*/
