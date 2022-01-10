// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/relations/ar_PotentialRelationCollector.h"
#include "Generic/relations/PotentialTrainingRelation.h"
#include "Generic/relations/PotentialRelationInstance.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Generic/preprocessors/RelationTypeMap.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/common/limits.h"
#include "Arabic/relations/ar_PotentialRelationInstance.h"
#include "Arabic/parse/ar_STags.h"
#include <boost/scoped_ptr.hpp>

using namespace std;

const Symbol NONE_SYM = Symbol(L"NONE");
const Symbol NULL_SYM = Symbol();
const Symbol BAD_EDT_SYM = Symbol(L"BAD EDT");
const Symbol SAME_ENTITY_SYM = Symbol(L"SAME ENTITY");
const Symbol NEG_HYPO_SYM = Symbol(L"NEG/HYPO");

ArabicPotentialRelationCollector::ArabicPotentialRelationCollector(int collectionMode, const RelationTypeMap *relationTypes) :
												_docTheory(0), _mentionSet(0), _annotationSet(0)
{
	_collectionMode = collectionMode;
	_relationTypes = relationTypes;

	if (_collectionMode != EXTRACT &&
		_collectionMode != TRAIN &&
		_collectionMode != CORRECT_ANSWERS)
	{
		char c[200];
		sprintf(c, "%d is not a valid relation collection mode: use EXTRACT, TRAIN, or CORRECT_ANSWERS", _collectionMode);
				throw InternalInconsistencyException("ArabicPotentialRelationCollector::ArabicPotentialRelationCollector()", c);
	}

	if (_collectionMode == CORRECT_ANSWERS) {
		std::string packet = ParamReader::getRequiredParam("relation_packet_file");
		std::string ca_file = ParamReader::getRequiredParam("correct_answer_file");
		_annotationSet = readPacketAnnotation(packet.c_str());
		_outputStream.open(ca_file.c_str());
	}
	_n_relations = 0;

	_debugStream.init(Symbol(L"relation_collector_debug"));
	_primary_parse = ParamReader::getParam(Symbol(L"primary_parse"));
	if (!SentenceTheory::isValidPrimaryParseValue(_primary_parse))
		throw UnrecoverableException("ArabicPotentialRelationCollector::ArabicPotentialRelationCollector()",
			"Parameter 'primary_parse' has an invald value");

}

void ArabicPotentialRelationCollector::loadDocTheory(DocTheory* theory) {
	if (_collectionMode != TRAIN)
		finalize(); // get rid of old stuff
	_docTheory = theory;
}

void ArabicPotentialRelationCollector::produceOutput(const wchar_t *output_dir,
											   const wchar_t *doc_filename)
{
	collectPotentialDocumentRelations();

	if (_collectionMode == EXTRACT && _trainingRelations.length() > 0) {
		wstring output_file = wstring(output_dir) + LSERIF_PATH_SEP + wstring(doc_filename) + L".relsent";

		_outputStream.open(output_file.c_str());
		outputPotentialTrainingRelations();
		_outputStream.close();
		_outputFiles.add(output_file);
	}
	else if (_collectionMode == CORRECT_ANSWERS) {
		outputCorrectAnswerFile();
	}

}


void ArabicPotentialRelationCollector::collectPotentialDocumentRelations() {
	_debugStream << "\nWorking on document " << _docTheory->getDocument()->getName().to_string() << "\n\n";
	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		Parse *parse = _docTheory->getSentenceTheory(i)->getPrimaryParse();
		MentionSet *mentionSet = _docTheory->getSentenceTheory(i)->getMentionSet();
		_debugStream << "*** Sentence " << i << " ***\n";
		collectPotentialSentenceRelationsFromParse(parse, mentionSet);
	}
}

void ArabicPotentialRelationCollector::collectPotentialSentenceRelationsFromChunk(const Parse *parse,
																 MentionSet *mentionSet)
{
	_mentionSet = mentionSet;
	const SynNode *root = parse->getRoot();
	_debugStream << "Root = " << root->getTag().to_string() << "\n";
	for (int i = 0; i < root->getNChildren(); i++) {
		const SynNode *child = root->getChild(i);
		_debugStream << "\tChild = " << child->getTag().to_string() << "\n";
		if (LanguageSpecificFunctions::isNPtypeLabel(child->getTag()))
			processNP(child, i);
	}
}

void ArabicPotentialRelationCollector::processNP(const SynNode *node, int& index) {
	std::vector<Mention *> mentions;

	_debugStream << "\t\tProcessing NP: ";
	if (node->hasMention()) {
		mentions.push_back(_mentionSet->getMentionByNode(node));
		_debugStream << "Found mention. ";
	}

	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->hasMention()) {
			mentions.push_back(_mentionSet->getMentionByNode(child));
			_debugStream << "Found mention. ";
		}
	}
	_debugStream << "\n";

	// peek ahead at the next sibling to look for PREPs
	const SynNode *parent = node->getParent();
	while (index + 1 < parent->getNChildren() && parent->getChild(index + 1)->getTag() == ArabicSTags::IN) {
		index++;
		// skip over any more adjacent PREPs
		while (index + 1 < parent->getNChildren() && parent->getChild(index + 1)->getTag() == ArabicSTags::IN)
			index++;
		if (index + 1 < parent->getNChildren() &&
			LanguageSpecificFunctions::isNPtypeLabel(parent->getChild(index + 1)->getTag()))
		{
			index++;
			processSubNP(parent->getChild(index), mentions);
		}
	}

	for (size_t j = 0; j < mentions.size(); j++) {
		for (size_t k = j + 1; k < mentions.size(); k++) {
			addPotentialRelation(mentions[j], mentions[k]);
		}
	}
}

void ArabicPotentialRelationCollector::processSubNP(const SynNode *node, std::vector<Mention *> &mentions) {

	_debugStream << "\t\tProcessing Sub NP: ";
	if (node->hasMention()) {
		mentions.push_back(_mentionSet->getMentionByNode(node));
		_debugStream << "Found mention. ";
	}

	for (int i = 0; i < node->getNChildren(); i++) {
		const SynNode *child = node->getChild(i);
		if (child->hasMention()) {
			mentions.push_back(_mentionSet->getMentionByNode(child));
			_debugStream << "Found mention. ";
		}
	}
	_debugStream << "\n";
}

void ArabicPotentialRelationCollector::collectPotentialSentenceRelationsFromParse(const Parse *parse,
																		   MentionSet *mentionSet)
{
	_mentionSet = mentionSet;
	traverseNode(parse->getRoot());
}

void ArabicPotentialRelationCollector::traverseNode(const SynNode *node) {

	if (node->isPreterminal())
		return;

	if (LanguageSpecificFunctions::isNPtypeLabel(node->getTag()) ||
		LanguageSpecificFunctions::getNameLabel() == node->getTag())
	{
		std::vector<Mention*> subMentions;
		processMaximalNP(node, subMentions);
		for (size_t i = 0; i < subMentions.size(); i++) {
			for (size_t j = i + 1; j < subMentions.size(); j++) {
				addPotentialRelation(subMentions[i], subMentions[j]);
			}
		}
	}
	else {
		for (int i = 0; i < node->getNChildren(); i++)
			traverseNode(node->getChild(i));
	}
}


void ArabicPotentialRelationCollector::processMaximalNP(const SynNode *node,
														std::vector<Mention *> &mentions)
{
	if (node->isPreterminal())
		return;

	Symbol tag = node->getTag();
	if (tag == ArabicSTags::SBAR ||
		tag == ArabicSTags::VP ||
		tag == ArabicSTags::S ||
		tag == ArabicSTags::S_ADV)
	{
		traverseNode(node);
	}
	else {
		if (node->hasMention() && _mentionSet->getMention(node->getMentionIndex())->isOfRecognizedType())
			mentions.push_back(_mentionSet->getMention(node->getMentionIndex()));
		else {
			for (int i = 0; i < node->getNChildren(); i++)
				processMaximalNP(node->getChild(i), mentions);
		}

	}
}


void ArabicPotentialRelationCollector::addPotentialRelation(Mention *ment1, Mention *ment2) {

	// return if same mention
	if (ment1->getIndex() == ment2->getIndex())
		return;

	if (_collectionMode == EXTRACT || _collectionMode == TRAIN || _collectionMode == CORRECT_ANSWERS) {
		PotentialTrainingRelation *tRel = _new PotentialTrainingRelation(ment1, ment2, _docTheory,
																		 _mentionSet->getSentenceNumber());
		_trainingRelations.add(tRel);
	}

	if (_collectionMode == TRAIN) {
		ArabicPotentialRelationInstance *iRel = _new ArabicPotentialRelationInstance();
		iRel->setStandardInstance(ment1, ment2,
			_docTheory->getSentenceTheory(_mentionSet->getSentenceNumber())->getTokenSequence());
		_relationInstances.add(iRel);
	}

	_n_relations++;
}


void ArabicPotentialRelationCollector::outputPotentialTrainingRelations() {
	_outputStream << _trainingRelations.length() << L"\n";
	for (int i = 0; i < _trainingRelations.length(); i++) {
		_outputStream << *_trainingRelations[i];
	}
}


void ArabicPotentialRelationCollector::outputCorrectAnswerFile() {
	_outputStream << "(" << _docTheory->getDocument()->getName().to_string() << "\n";
	outputCorrectEntities();
	outputCorrectRelations();
	_outputStream << ")" << "\n";
}

void ArabicPotentialRelationCollector::outputCorrectEntities() {
	_outputStream << "  (Entities" << "\n";

	for (int i = 0; i < _docTheory->getNSentences(); i++) {
		MentionSet *mentions = _docTheory->getSentenceTheory(i)->getMentionSet();
		TokenSequence *tokens = _docTheory->getSentenceTheory(i)->getTokenSequence();
		for (int j = 0; j < mentions->getNMentions(); j++) {
			Mention *m = mentions->getMention(j);
			if (m->isOfRecognizedType()) {
				_outputStream << "    (" << m->getEntityType().getName().to_string();
				_outputStream << " " << getDefaultSubtype(m->getEntityType()).to_string();
				_outputStream << " SPC FALSE E" << m->getUID() << "\n";
				EDTOffset start = tokens->getToken(m->getNode()->getStartToken())->getStartEDTOffset();
				EDTOffset end = tokens->getToken(m->getNode()->getEndToken())->getEndEDTOffset();
				_outputStream << "      (" << convertMentionType(m) << " ";
				if (m->hasRoleType())
					_outputStream << m->getRoleType().getName().to_string() << " ";
				else
					_outputStream << "NONE ";
				_outputStream << start << " " << end;
				_outputStream << " " << start << " " << end << " " << m->getUID() << "-1)\n";
				_outputStream << "    )\n";
			}
		}
	}
	_outputStream << "  )" << "\n";
}


void ArabicPotentialRelationCollector::outputCorrectRelations() {
	_outputStream << "  (Relations" << "\n";

	for (int i = 0; i < getNRelations(); i++) {
		PotentialTrainingRelation *rel = getPotentialTrainingRelation(i);
		PotentialRelationMap::iterator it = _annotationSet->find(*rel);
		if (it != _annotationSet->end())  {
			PotentialTrainingRelation ann = ((*it).first);
			if (ann.getRelationType() != BAD_EDT_SYM &&
				ann.getRelationType() != SAME_ENTITY_SYM &&
				ann.getRelationType() != NEG_HYPO_SYM &&
				ann.getRelationType() != NONE_SYM)
			{
				Symbol type = _relationTypes->lookup(ann.getRelationType());
				_outputStream << "    (" << type.to_string() << " EXPLICIT R" << i << "\n";
				if (!ann.relationIsReversed()) {
					_outputStream << "      (" << rel->getLeftMention() << "-1 ";
					_outputStream << rel->getRightMention() << "-1 " << i << "-1)\n";
				}
				else {
					_outputStream << "      (" << rel->getRightMention() << "-1 ";
					_outputStream << rel->getLeftMention() << "-1 " << i << "-1)\n";
				}
				_outputStream << "    )\n";
			}
			else if (ann.getRelationType() == NEG_HYPO_SYM ||
					 ann.getRelationType() == NONE_SYM)
			{
				_outputStream << "    (" << NONE_SYM.to_string() << " EXPLICIT R" << i << "\n";
				if (!ann.relationIsReversed()) {
					_outputStream << "      (" << rel->getLeftMention() << "-1 ";
					_outputStream << rel->getRightMention() << "-1 " << i << "-1)\n";
				}
				else {
					_outputStream << "      (" << rel->getRightMention() << "-1 ";
					_outputStream << rel->getLeftMention() << "-1 " << i << "-1)\n";
				}
				_outputStream << "    )\n";
			}

		}

	}
	_outputStream << "  )" << "\n";
}


void ArabicPotentialRelationCollector::finalize() {
	while (_trainingRelations.length() > 0) {
		PotentialTrainingRelation *r = _trainingRelations.removeLast();
		delete r;
	}
	while (_relationInstances.length() > 0) {
		PotentialRelationInstance *r = _relationInstances.removeLast();
		delete r;
	}

	_n_relations = 0;
}

void ArabicPotentialRelationCollector::outputPacketFile(const char *output_dir,
												  const char *packet_name)
{
	std::stringstream packet_file;
	packet_file << output_dir << SERIF_PATH_SEP << packet_name << ".pkt";

	ofstream packetStream;
	packetStream.open(packet_file.str().c_str());
	packetStream << _outputFiles.length() << "\n";
	for (int i = 0; i < _outputFiles.length(); i++) {
		packetStream << _outputFiles[i] << "\n";
	}
	packetStream.close();

}

PotentialTrainingRelation* ArabicPotentialRelationCollector::getPotentialTrainingRelation(int i) {
	if (i < 0 || i > _trainingRelations.length() - 1)
		throw InternalInconsistencyException("ArabicPotentialRelationCollector::getPotentialTrainingRelation",
											 "Array index out of bounds");
	else
		return _trainingRelations[i];
}

PotentialRelationInstance* ArabicPotentialRelationCollector::getPotentialRelationInstance(int i) {
	if (i < 0 || i > _relationInstances.length() - 1)
		throw InternalInconsistencyException("ArabicPotentialRelationCollector::getPotentialRelationInstance",
											 "Array index out of bounds");
	else
		return _relationInstances[i];
}

ArabicPotentialRelationCollector::PotentialRelationMap *ArabicPotentialRelationCollector::readPacketAnnotation
																			(const char *packet_file)
{
	PotentialRelationMap *annotationSet = _new PotentialRelationMap(1024);
	boost::scoped_ptr<UTF8InputStream> packet_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& packet(*packet_scoped_ptr);
	wchar_t line[200];

	packet.open(packet_file);
	if (packet.fail()) {

		char message[300];
		sprintf(message, "Could not open packet file `%s'", packet_file);
		throw UnexpectedInputException("ArabicPotentialRelationCollector::readPacketAnnotation()", message);
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
			throw UnexpectedInputException("ArabicPotentialRelationCollector::readPacketAnnotation()",  message);
		}
		packet.getLine(line, 200);
		in.open(line);
		if (in.fail()) {
			char message[300];
			sprintf(message, "Could not open annotation file `%s'", UnicodeUtil::toUTF8String(line));
			throw UnexpectedInputException("ArabicPotentialRelationCollector::readPacketAnnotation()", message);
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

const wchar_t* ArabicPotentialRelationCollector::convertMentionType(Mention* ment)
{
	Mention::Type type = ment->mentionType;
	Mention* subMent = 0;
	switch (type) {
		case Mention::NAME:
			return L"NAME";
		case Mention::DESC:
		case Mention::PART:
			return L"NOMINAL";
		case Mention::PRON:
			return L"PRONOUN";
		case Mention::APPO:
			// in ACE 2004, we shouldn't be printing these
			return L"WARNING_BAD_MENTION_TYPE";
		case Mention::LIST:
			// for list, the type is the type of the first child
			subMent = ment->getChild();
			return convertMentionType(subMent);
		default:
			return L"WARNING_BAD_MENTION_TYPE";
	}
}

Symbol ArabicPotentialRelationCollector::getDefaultSubtype(EntityType type) {
	if (type.matchesPER())
		return Symbol(L"NONE");
	else if (type.matchesLOC())
		return Symbol(L"Address");
	else
		return Symbol(L"Other");
}
