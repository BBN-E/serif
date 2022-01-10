// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/ParamReader.h"
#include "common/NgramScoreTable.h"
#include "common/hash_map.h"
#include "common/UTF8Token.h"
#include "common/SymbolConstants.h"
#include "theories/RelationConstants.h"
#include "relations/RelationTypeSet.h"
#include "relations/PotentialRelationInstance.h"
#include "relations/RelationFilter.h"
#include "relations/RelationUtilities.h"
#include "maxent/MaxEntEvent.h"
#include "maxent/MaxEntModel.h"
#include "Chinese/relations/ch_MaxEntRelationModel.h"
#include "Chinese/relations/ch_PotentialRelationInstance.h"
#include <math.h>
#include <boost/scoped_ptr.hpp>

const Symbol MaxEntRelationModel::IS_RELATION = Symbol(L"IS_RELATION");
const Symbol MaxEntRelationModel::NO_RELATION = Symbol(L"NO_RELATION");
const Symbol MaxEntRelationModel::NULL_SYM = Symbol(L"NULL");
const int MaxEntRelationModel::MAX_PREDICATES = 200;
const int initialTableSize = 16000;
const int numClusters = 4;

MaxEntRelationModel::MaxEntRelationModel() :
								_typeModel(0), _existanceModel(0),
								_clusterTable(0)
{
	// Load param settings for SPLIT_LEVEL_DECISION
	char buffer[501];
	if (!ParamReader::getParam("is_split_level_decision",buffer, 500))		throw  UnexpectedInputException("MaxEntRelationModel()",									 "Parameter 'is_split_level_decision' not specified");
	if (strcmp(buffer, "true") == 0)
		SPLIT_LEVEL_DECISION = true;
	else if (strcmp(buffer, "false") == 0)
		SPLIT_LEVEL_DECISION = false;
	else
		throw  UnexpectedInputException("MaxEntRelationModel()",
									 "Parameter 'is_split_level_decision' must be set to 'true' or 'false'");


	// Load param settings for training options
	int mode;
	int stop_criterion;
	int percent_held_out;
	double variance = 1.0;
	int num_features_to_add = 1;

	if (!ParamReader::getParam("relation_train_mode",buffer, 500))		throw UnexpectedInputException("MaxEntRelationModel()",									 "Parameter 'relation_train_mode' not specified");
	if (strcmp(buffer, "GIS") == 0)
		mode = MaxEntModel::GIS;
	else if (strcmp(buffer, "IIS") == 0)
		mode = MaxEntModel::IIS;
	else if (strcmp(buffer, "IIS_GAUSSIAN") == 0) {
		mode = MaxEntModel::IIS_GAUSSIAN;
		if (!ParamReader::getParam("relation_train_gaussian_variance",buffer, 500))			throw  UnexpectedInputException("MaxEntRelationModel()",									 "Parameter 'relation_train_gaussian_variance' not specified");
		variance = atof(buffer);
	}
	else if (strcmp(buffer, "IIS_FEATURE_SELECTION") == 0) {
		mode = MaxEntModel::IIS_FEATURE_SELECTION;
		if (ParamReader::getParam("num_relation_features_to_add",buffer, 500)) {
			num_features_to_add = atoi(buffer);
			if (num_features_to_add > 10 || num_features_to_add < 1)
				throw UnexpectedInputException("MaxEntRelationModel()",
								"Parameter 'num_relation_features_to_add' must range between 1 and 10.");
		}
	}
	else
		throw UnexpectedInputException("MaxEntRelationModel()",
									 "Invalid setting for parameter 'relation_train_mode'");

	if (!ParamReader::getParam("relation_train_stop_criterion",buffer, 500))		throw  UnexpectedInputException("MaxEntRelationModel()",									 "Parameter 'relation_train_stop_criterion' not specified");
	if (strcmp(buffer, "PROBS_CONVERGE") == 0)
		stop_criterion = MaxEntModel::PROBS_CONVERGE;
	else if (strcmp(buffer, "HELD_OUT_LIKELIHOOD") == 0)
		stop_criterion = MaxEntModel::HELD_OUT_LIKELIHOOD;
	else
		throw  UnexpectedInputException("MaxEntRelationModel()",
									 "Invalid setting for parameter 'relation_train_stop_criterion'");

	if (!ParamReader::getParam("relation_train_percent_held_out",buffer, 500))		throw  UnexpectedInputException("MaxEntRelationModel()",									 "Parameter 'relation_train_percent_held_out' not specified");
	percent_held_out = atoi(buffer);
	if (percent_held_out < 0 || percent_held_out > 100)
		throw  UnexpectedInputException("MaxEntRelationModel()",
									 "Invalid setting for parameter 'relation_train_percent_held_out'");


	// Set up relation types
	int n_types = 0;
	Symbol *symArray = _new Symbol[RelationTypeSet::N_RELATION_TYPES * 2];
	for (int i = 0; i < RelationTypeSet::N_RELATION_TYPES; i++) {

		// don't add NONE if we're making a split level decision
		if (SPLIT_LEVEL_DECISION && RelationTypeSet::isNull(i))
			continue;

		symArray[n_types++] = RelationTypeSet::getRelationSymbol(i);
		// add reverse if it exists
		if (!RelationTypeSet::isSymmetric(i) && !RelationTypeSet::isNull(i))
			symArray[n_types++] = RelationTypeSet::getRelationSymbol(RelationTypeSet::reverse(i));
	}
	_typeModel = _new MaxEntModel(n_types, symArray, mode, stop_criterion, percent_held_out, variance, num_features_to_add);

	if (SPLIT_LEVEL_DECISION) {
		symArray[0] = NO_RELATION;
		symArray[1] = IS_RELATION;
		_existanceModel = _new MaxEntModel(2, symArray, mode, stop_criterion, percent_held_out, variance, num_features_to_add);
	}

	_clusterTable = _new WordClusterTable(initialTableSize);
	initializeWordClusterTable();

	delete [] symArray;
}

MaxEntRelationModel::MaxEntRelationModel(char *file_prefix) :
								_typeModel(0), _existanceModel(0),
								_clusterTable(0)
{
	char buffer[501];
	if (!ParamReader::getParam("is_split_level_decision",buffer, 500))		throw UnrecoverableException("MaxEntRelationModel()",									 "Parameter 'is_split_level_decision' not specified");

	if (strcmp(buffer, "true") == 0)
		SPLIT_LEVEL_DECISION = true;
	else if (strcmp(buffer, "false") == 0)
		SPLIT_LEVEL_DECISION = false;
	else
		throw UnrecoverableException("MaxEntRelationModel()",
									 "Parameter 'is_split_level_decision' must be set to 'true' or 'false'");

	sprintf(buffer, "%s.maxent", file_prefix);
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(buffer);
	if (SPLIT_LEVEL_DECISION)
		_existanceModel = _new MaxEntModel(in);
	_typeModel = _new MaxEntModel(in);
	in.close();

	_clusterTable = _new WordClusterTable(initialTableSize);
	initializeWordClusterTable();
}

MaxEntRelationModel::~MaxEntRelationModel() {

	delete _typeModel;
	delete _existanceModel;

	WordClusterTable::iterator cIter;
	for (cIter = _clusterTable->begin(); cIter != _clusterTable->end(); ++cIter)
	{
		delete [] (*cIter).second;
	}
	delete _clusterTable;
}

void MaxEntRelationModel::initializeWordClusterTable() {
	char buffer[500];
	if (!ParamReader::getParam("word_cluster_table",buffer, 500))		throw UnrecoverableException("MaxEntRelationModel::initializeWordClusterTable()",									 "Parameter 'word_cluster_table' not specified");

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(buffer));
	UTF8InputStream& in(*in_scoped_ptr);
	UTF8Token token;
	wchar_t bitstring[200];

	while (!in.eof()) {
		in >> token;
		Symbol word = token.symValue();
		in >> token;
		wcsncpy(bitstring, token.chars(), 200);
		int total_len = static_cast<int>(wcslen(bitstring));
		Symbol *clusters = _new Symbol[numClusters];
		int clusterBitSize = 32 / numClusters;
		for (int i = 0; i < numClusters; i++) {
			int len = (numClusters - i) * clusterBitSize;
			if (len < total_len) {
				bitstring[len] = '\0';
				clusters[i] = Symbol(bitstring);
			}
			else {
				clusters[i] = Symbol();
			}
		}

		(*_clusterTable)[word] = clusters;
	}
	in.close();

}

void MaxEntRelationModel::train(char *training_file, char* output_file_prefix) {

	char buffer[500];
	if (!ParamReader::getParam("relation_train_pruning_cutoff",buffer, 500))		throw UnrecoverableException("MaxEntRelationModel::train()",									 "Parameter 'relation_train_pruning_cutoff' not specified");
	int	cutoff = atoi(buffer);

	if (!ParamReader::getParam("relation_train_threshold",buffer, 500))		throw UnrecoverableException("MaxEntRelationModel::train()",									 "Parameter 'relation_train_threshold' not specified");
	double threshold = atof(buffer);



	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(training_file);
	int numEntries;
	in >> numEntries;

	MaxEntEvent *event = _new MaxEntEvent(MAX_PREDICATES);
	ChinesePotentialRelationInstance *inst = _new ChinesePotentialRelationInstance();
	Symbol *ngram = _new Symbol[CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE];

	for (int i = 0; i < numEntries; i++) {

		int count = readTrainingVector(in, ngram);
		inst->setFromTrainingNgram(ngram);

		// Check to make sure entity types are valid
		EntityType left(inst->getLeftEntityType());
		EntityType right(inst->getRightEntityType());
		char message[500];
		if (!left.canBeRelArg()) {
			sprintf(message, "%s%s", "Given relation type cannot be a relation arg: ", left.getName().to_string());
			throw UnexpectedInputException("MaxEntRelationModel::train()", message);
		}
		if (!right.canBeRelArg()) {
			sprintf(message, "%s%s", "Given relation type cannot be a relation arg: ", right.getName().to_string());
			throw UnexpectedInputException("MaxEntRelationModel::train()", message);
		}

		int t = RelationTypeSet::getTypeFromSymbol(inst->getRelationType());
		if (t == RelationTypeSet::INVALID_TYPE) {
			sprintf(message, "%s%s", "Invalid Relation Type: ", inst->getRelationType().to_string());
			throw UnexpectedInputException("MaxEntRelationModel::train()", message);
		}
		if (!RelationTypeSet::isSymmetric(t) && inst->isReversed())
		{
			t = RelationTypeSet::reverse(t);
			inst->setRelationType(RelationTypeSet::getRelationSymbol(t));
		}
		inst->setReverse(false);


		if (SPLIT_LEVEL_DECISION)  {
			fillMaxEntExistanceEvent(event, inst);
			_existanceModel->addEvent(event, count);
			if (inst->getRelationType() == RelationConstants::NONE)
				continue;
		}

		fillMaxEntTypeEvent(event, inst);
		_typeModel->addEvent(event, count);
	}
	in.close();
	delete inst;
	delete event;
	delete [] ngram;

	if (SPLIT_LEVEL_DECISION) {
		cout << "\nDeriving relation existance model...\n";
		_existanceModel->deriveModel(cutoff, threshold);
	}
	cout << "\nDeriving relation type model...\n";
	_typeModel->deriveModel(cutoff, threshold);

	printModel(output_file_prefix);
}

void MaxEntRelationModel::printModel(const char *file_prefix) {
	char buffer[500];

	cout << "Writing Model file...\n";

	sprintf(buffer, "%s.maxent", file_prefix);
	UTF8OutputStream stream;
	stream.open(buffer);
	if (SPLIT_LEVEL_DECISION) {
		_existanceModel->print_to_open_stream(stream);
	}
	_typeModel->print_to_open_stream(stream);
	stream.close();
}

int MaxEntRelationModel::findBestRelationType(ChinesePotentialRelationInstance *instance) {

	MaxEntEvent *event = _new MaxEntEvent(MAX_PREDICATES);

	if (SPLIT_LEVEL_DECISION) {
		double yes_score, no_score;
		fillMaxEntExistanceEvent(event, instance);

		event->setOutcome(IS_RELATION);
		yes_score = _existanceModel->getScore(event);

		event->setOutcome(NO_RELATION);
		no_score = _existanceModel->getScore(event);

		if (no_score > yes_score)
			return RelationTypeSet::getTypeFromSymbol(RelationConstants::NONE);
	}

	fillMaxEntTypeEvent(event, instance);

	double max_value = -1000;
	double score;
	int max_index = 0;
	for (int i = 0; i < RelationTypeSet::N_RELATION_TYPES; i++) {

		if (SPLIT_LEVEL_DECISION && RelationTypeSet::isNull(i))
			continue;

		Symbol outcome = RelationTypeSet::getRelationSymbol(i);
		event->setOutcome(outcome);
		score = _typeModel->getScore(event);
		if (score > max_value) {
			max_value = score;
			max_index = i;
		}
		// score the reverse too
		if (!RelationTypeSet::isSymmetric(i)) {
			outcome = RelationTypeSet::getRelationSymbol(-i);
			event->setOutcome(outcome);
			score = _typeModel->getScore(event);
			if (score > max_value) {
				max_value = score;
				max_index = -i;
			}
		}
	}
	if (RelationUtilities::debugStreamIsOn()) {
		Symbol outcome = RelationTypeSet::getRelationSymbol(max_index);
		RelationUtilities::getDebugStream() << outcome.to_string() << ":\n";
		event->setOutcome(outcome);
		_typeModel->getScoreAndDebug(event, RelationUtilities::getDebugStream());
		RelationUtilities::getDebugStream() << "\n";
	}

	delete event;
	return max_index;
}


// It's not completely clear how to do this with split level decision...
int MaxEntRelationModel::findNBestRelationTypes(ChinesePotentialRelationInstance *instance,
											    int *results, double *probs, int max_results)
{

	MaxEntEvent *event = _new MaxEntEvent(MAX_PREDICATES);

	if (SPLIT_LEVEL_DECISION) {
		double yes_score, no_score;
		fillMaxEntExistanceEvent(event, instance);

		event->setOutcome(IS_RELATION);
		yes_score = _existanceModel->getScore(event);

		event->setOutcome(NO_RELATION);
		no_score = _existanceModel->getScore(event);

		if (no_score > yes_score) {
			results[0] = RelationTypeSet::getTypeFromSymbol(RelationConstants::NONE);
			probs[0] = exp(no_score) / (exp(yes_score) + exp(no_score));
			return 1;
		}
	}

	fillMaxEntTypeEvent(event, instance);

	int n_results = 0;
	double context_prob = 0;
	for (int i = 0; i < RelationTypeSet::N_RELATION_TYPES; i++) {

		if (SPLIT_LEVEL_DECISION && RelationTypeSet::isNull(i))
			continue;

		Symbol outcome = RelationTypeSet::getRelationSymbol(i);
		event->setOutcome(outcome);
		double score = _typeModel->getScore(event);
		// insert score in results array
		bool inserted = false;
		for (int j = 0; j < n_results; j++) {
			if (score > probs[j]) {
				for (int k = (n_results < max_results ? n_results : max_results - 1); k > j; k--) {
					probs[k] = probs[k-1];
					results[k] = results[k-1];
				}
				probs[j] = score;
				results[j] = i;
				inserted = true;
				if (n_results < max_results)
					n_results++;
				break;
			}
		}
		if (!inserted && n_results < max_results) {
			probs[n_results] = score;
			results[n_results++] = i;
		}

		context_prob += exp(score);

		// if type is not symmetric, score the reverse
		if (! RelationTypeSet::isSymmetric(i) && ! RelationTypeSet::isNull(i)) {
			outcome = RelationTypeSet::getRelationSymbol(RelationTypeSet::reverse(i));
			event->setOutcome(outcome);
			double score = _typeModel->getScore(event);
			// insert score in results array
			bool inserted = false;
			for (int j = 0; j < n_results; j++) {
				if (score > probs[j]) {
					for (int k = (n_results < max_results ? n_results : max_results - 1); k > j; k--) {
						probs[k] = probs[k-1];
						results[k] = results[k-1];
					}
					probs[j] = score;
					results[j] = RelationTypeSet::reverse(i);
					inserted = true;
					if (n_results < max_results)
						n_results++;
					break;
				}
			}
			if (!inserted && n_results < max_results) {
				probs[n_results] = score;
				results[n_results++] = RelationTypeSet::reverse(i);
			}

			context_prob += exp(score);
		}
	}

	// Normalize the probabilities by the context probability
	for (int p = 0; p < n_results; p++)
		probs[p] = exp(probs[p]) / context_prob;

	delete event;
	return n_results;
}

void MaxEntRelationModel::testModel(char *vector_file) {

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(vector_file);
	NgramScoreTable *_originalData = _new NgramScoreTable(13, in);
	in.close();

	cout << "\nTesting on vectors in " << vector_file << ":\n\n";
	int n_correct = 0;
	int n_yes_no_correct = 0;
	int n_total = 0;
	int *topFive = _new int[5];
	double *probs = _new double[5];

	ChinesePotentialRelationInstance *inst = _new ChinesePotentialRelationInstance();
	NgramScoreTable::Table::iterator iter;
	for (iter = _originalData->get_start(); iter != _originalData->get_end(); ++iter) {
		float count = (*iter).second;
		inst->setFromTrainingNgram((*iter).first);

		int t = RelationTypeSet::getTypeFromSymbol(inst->getRelationType());
		if (!RelationTypeSet::isSymmetric(t) && inst->isReversed())
		{
			t = RelationTypeSet::reverse(t);
			inst->setRelationType(RelationTypeSet::getRelationSymbol(t));
		}
		inst->setReverse(false);

		int types_found = findNBestRelationTypes(inst, topFive, probs, 5);
		//cout << "ACTUAL: " << inst->getRelationType().to_debug_string() << "\n";
		//cout << "PREDICTED: ";
		for (int i = 0; i < types_found; i++) {
			//cout << " " << RelationTypeSet::getRelationSymbol(topFive[i]).to_debug_string();
			//cout << " " << probs[i];
		}
		//cout << "\n\n";

		if (topFive[0] == t)
			n_correct++;

		if (RelationTypeSet::getRelationSymbol(t) == RelationConstants::NONE && topFive[0] == t)
			n_yes_no_correct++;
		else if (RelationTypeSet::getRelationSymbol(t) != RelationConstants::NONE &&
			RelationTypeSet::getRelationSymbol(topFive[0]) != RelationConstants::NONE)
			n_yes_no_correct++;

		n_total++;
	}

	cout << n_correct << " out of " << n_total << " total instances correct.\n";
	cout << n_yes_no_correct << " out of " << n_total << " yes/no decisions correct.\n";

	delete _originalData;
	delete [] topFive;
	delete [] probs;

}





void MaxEntRelationModel::fillMaxEntTypeEvent(MaxEntEvent *event, ChinesePotentialRelationInstance *inst) {

	wchar_t str[100];

	event->reset();
	event->setOutcome(inst->getRelationType());

	// Collect atomic vector features
	Symbol *atoms = _new Symbol[CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE];

	// predicate
	wcscpy(str, L"p:");
	wcsncat(str, inst->getPredicate().to_string(), 100);
	atoms[0] = Symbol(str);
	event->addAtomicContextPredicate(atoms[0]);

	// left headword
	wcscpy(str, L"lh:");
	wcsncat(str, inst->getLeftHeadword().to_string(), 100);
	atoms[1] = Symbol(str);
	event->addAtomicContextPredicate(atoms[1]);


	// right headword
	wcscpy(str, L"rh:");
	wcsncat(str, inst->getRightHeadword().to_string(), 100);
	atoms[2] = Symbol(str);
	event->addAtomicContextPredicate(atoms[2]);


	// nested word
	if (inst->getNestedWord() != NULL_SYM) {
		wcscpy(str, L"nw:");
		wcsncat(str, inst->getNestedWord().to_string(), 100);
		atoms[3] = Symbol(str);
		event->addAtomicContextPredicate(atoms[3]);
	}
	else
		atoms[3] = Symbol();

	// left type
	wcscpy(str, L"lt:");
	wcsncat(str, inst->getLeftEntityType().to_string(), 100);
	atoms[4] = Symbol(str);
	event->addAtomicContextPredicate(atoms[4]);


	// right type
	wcscpy(str, L"rt:");
	wcsncat(str, inst->getRightEntityType().to_string(), 100);
	atoms[5] = Symbol(str);
	event->addAtomicContextPredicate(atoms[5]);


	// left role
	wcscpy(str, L"lr:");
	wcsncat(str, inst->getLeftRole().to_string(), 100);
	atoms[6] = Symbol(str);
	event->addAtomicContextPredicate(atoms[6]);


	// right role
	wcscpy(str, L"rr:");
	wcsncat(str, inst->getRightRole().to_string(), 100);
	atoms[7] = Symbol(str);
	event->addAtomicContextPredicate(atoms[7]);


	// nested role
	if (inst->getNestedRole() != NULL_SYM) {
		wcscpy(str, L"nr:");
		wcsncat(str, inst->getNestedRole().to_string(), 100);
		atoms[8] = Symbol(str);
		//event->addAtomicContextPredicate(atoms[8]);
	}
	else
		atoms[8] = Symbol();

	// last hanzi of predicate
	wcscpy(str, L"hanzi:");
	wcsncat(str, inst->getLastHanziOfPredicate().to_string(), 100);
	atoms[9] = Symbol(str);
	event->addAtomicContextPredicate(atoms[9]);

	// left mention type
	wcscpy(str, L"lmt:");
	wcsncat(str, inst->getLeftMentionType().to_string(), 100);
	atoms[10] = Symbol(str);
	event->addAtomicContextPredicate(atoms[10]);

	// right mention type
	wcscpy(str, L"rmt:");
	wcsncat(str, inst->getRightMentionType().to_string(), 100);
	atoms[11] = Symbol(str);
	event->addAtomicContextPredicate(atoms[11]);

	// left metonymy
	if (inst->getLeftMetonymy() != NULL_SYM) {
		wcscpy(str, L"lm:");
		wcsncat(str, inst->getLeftMetonymy().to_string(), 100);
		atoms[12] = Symbol(str);
		event->addAtomicContextPredicate(atoms[12]);
	}
	else
		atoms[12] = Symbol();

	// right metonymy
	if (inst->getRightMetonymy() != NULL_SYM) {
		wcscpy(str, L"rm:");
		wcsncat(str, inst->getRightMetonymy().to_string(), 100);
		atoms[13] = Symbol(str);
		event->addAtomicContextPredicate(atoms[13]);
	}
	else
		atoms[13] = Symbol();


	// collect clusters for predicate
	Symbol predicateClusters[numClusters];
	int n_predicate_clusters = 0;
	Symbol **clusters = _clusterTable->get(inst->getPredicate());
	if (clusters != 0) {
		for (int i = 0; i < numClusters - 1; i++) {
			if ((*clusters)[i] != Symbol()) {
				wchar_t cluster_num[5];
				_itow(i, cluster_num, 10);
				wcscpy(str, L"pc");
				wcsncat(str, cluster_num, 5);
				wcscat(str, L":");
				wcsncat(str, (*clusters)[i].to_string(), 100);
				predicateClusters[n_predicate_clusters++] = Symbol(str);
			}
		}
	}


	Symbol *ngram = _new Symbol[CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE];
	for (int i = 0; i < POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; i++) {
		if (atoms[i].is_null())
			continue;
		ngram[0] = atoms[i];
		for (int j = i + 1; j < POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; j++) {
			if (atoms[j].is_null())
				continue;
			ngram[1] = atoms[j];
			event->addComplexContextPredicate(2, ngram);
		}
	}

	// same as vector model: left type, right type, left role, right role, predicate, [nested role]
	ngram[0] = atoms[4];
	ngram[1] = atoms[5];
	ngram[2] = atoms[6];
	ngram[3] = atoms[7];
	ngram[4] = atoms[0];
	ngram[5] = atoms[8];
	if (!ngram[5].is_null())
		event->addComplexContextPredicate(6, ngram);
	else
		event->addComplexContextPredicate(5, ngram);
	// substiture last hanzi for predicate
	ngram[4] = atoms[9];
	if (!ngram[5].is_null())
		event->addComplexContextPredicate(6, ngram);
	else
		event->addComplexContextPredicate(5, ngram);
	// substitute clusters for predicate
	if (n_predicate_clusters > 0) {
		for (int i = 0; i < n_predicate_clusters; i++) {
			ngram[4] = predicateClusters[i];
			if (!ngram[5].is_null())
				event->addComplexContextPredicate(6, ngram);
			else
				event->addComplexContextPredicate(5, ngram);
		}
	}
	// take out predicate
	ngram[4] = ngram[5];
	if (!ngram[4].is_null())
		event->addComplexContextPredicate(5, ngram);
	else
		event->addComplexContextPredicate(4, ngram);

	// left hw, right hw, left role, right role, [nested hw], [nested role]
	ngram[0] = atoms[1];
	ngram[1] = atoms[2];
	ngram[2] = atoms[6];
	ngram[3] = atoms[7];
	ngram[4] = atoms[3];
	ngram[5] = atoms[8];
	if (!ngram[4].is_null() && !ngram[5].is_null())
		event->addComplexContextPredicate(6, ngram);
	else
		event->addComplexContextPredicate(4, ngram);

	// left hw, right hw, left role, right role, predicate, [nested hw], [nested role]
	ngram[0] = atoms[1];
	ngram[1] = atoms[2];
	ngram[2] = atoms[6];
	ngram[3] = atoms[7];
	ngram[4] = atoms[0];
	ngram[5] = atoms[3];
	ngram[6] = atoms[8];
	if (!ngram[5].is_null() && !ngram[6].is_null())
		event->addComplexContextPredicate(7, ngram);
	else
		event->addComplexContextPredicate(5, ngram);
	// substiture last hanzi for predicate
	ngram[4] = atoms[9];
	if (!ngram[5].is_null() && !ngram[6].is_null())
		event->addComplexContextPredicate(7, ngram);
	else
		event->addComplexContextPredicate(5, ngram);
	// substitute clusters for predicate
	if (n_predicate_clusters > 0) {
		for (int i = 0; i < n_predicate_clusters; i++) {
			ngram[4] = predicateClusters[i];
			if (!ngram[5].is_null() && !ngram[6].is_null())
				event->addComplexContextPredicate(7, ngram);
			else
				event->addComplexContextPredicate(5, ngram);
		}
	}

	// left hw, right hw, left type, right type, predicate, [nested hw]
	ngram[0] = atoms[1];
	ngram[1] = atoms[2];
	ngram[2] = atoms[4];
	ngram[3] = atoms[5];
	ngram[4] = atoms[0];
	ngram[5] = atoms[3];
	if (!ngram[5].is_null())
		event->addComplexContextPredicate(6, ngram);
	else
		event->addComplexContextPredicate(5, ngram);
	// substiture last hanzi for predicate
	ngram[4] = atoms[9];
	if (!ngram[5].is_null())
		event->addComplexContextPredicate(6, ngram);
	else
		event->addComplexContextPredicate(5, ngram);
	// substitute clusters for predicate
	if (n_predicate_clusters > 0) {
		for (int i = 0; i < n_predicate_clusters; i++) {
			ngram[4] = predicateClusters[i];
			if (!ngram[5].is_null())
				event->addComplexContextPredicate(6, ngram);
			else
				event->addComplexContextPredicate(5, ngram);
		}
	}


	// left hw, right hw, predicate, [nested hw]
	ngram[0] = atoms[1];
	ngram[1] = atoms[2];
	ngram[2] = atoms[0];
	ngram[3] = atoms[3];
	if (!ngram[3].is_null())
		event->addComplexContextPredicate(4, ngram);
	else
		event->addComplexContextPredicate(3, ngram);
	// substiture last hanzi for predicate
	ngram[2] = atoms[9];
	if (!ngram[3].is_null())
		event->addComplexContextPredicate(4, ngram);
	else
		event->addComplexContextPredicate(3, ngram);
	// substitute clusters for predicate
	if (n_predicate_clusters > 0) {
		for (int i = 0; i < n_predicate_clusters; i++) {
			ngram[2] = predicateClusters[i];
			if (!ngram[3].is_null())
				event->addComplexContextPredicate(4, ngram);
			else
				event->addComplexContextPredicate(3, ngram);
		}
	}

	// left type, right type, predicate
	ngram[0] = atoms[4];
	ngram[1] = atoms[5];
	ngram[2] = atoms[0];
	event->addComplexContextPredicate(3, ngram);
	// substiture last hanzi for predicate
	ngram[2] = atoms[9];
	event->addComplexContextPredicate(3, ngram);
	// substitute clusters for predicate
	if (n_predicate_clusters > 0) {
		for (int i = 0; i < n_predicate_clusters; i++) {
			ngram[2] = predicateClusters[i];
			event->addComplexContextPredicate(3, ngram);
		}
	}


	// left role, right role, predicate, [nested role]
	ngram[0] = atoms[6];
	ngram[1] = atoms[7];
	ngram[2] = atoms[0];
	ngram[3] = atoms[8];
	if (!ngram[3].is_null())
		event->addComplexContextPredicate(4, ngram);
	else
		event->addComplexContextPredicate(3, ngram);
	// substiture last hanzi for predicate
	ngram[2] = atoms[9];
	if (!ngram[3].is_null())
		event->addComplexContextPredicate(4, ngram);
	else
		event->addComplexContextPredicate(3, ngram);
	// substitute clusters for predicate
	if (n_predicate_clusters > 0) {
		for (int i = 0; i < n_predicate_clusters; i++) {
			ngram[2] = predicateClusters[i];
			if (!ngram[3].is_null())
				event->addComplexContextPredicate(4, ngram);
			else
				event->addComplexContextPredicate(3, ngram);
		}
	}

	// if nested role and word exist: nested role, nested word, right role, left type, right type, predicate
	if (!atoms[3].is_null() && !atoms[8].is_null()) {
		ngram[0] = atoms[8];
		ngram[1] = atoms[3];
		ngram[2] = atoms[7];
		ngram[3] = atoms[4];
		ngram[4] = atoms[5];
		ngram[5] = atoms[0];
		event->addComplexContextPredicate(6, ngram);
		// substitute last hanzi for predicate
		ngram[5] = atoms[9];
		event->addComplexContextPredicate(6, ngram);
		// substitute clusters for predicate
		if (n_predicate_clusters > 0) {
			for (int i = 0; i < n_predicate_clusters; i++) {
				ngram[5] = predicateClusters[i];
				event->addComplexContextPredicate(6, ngram);
			}
		}
		// leave off predicate all together
		event->addComplexContextPredicate(5, ngram);
	}

	// if nested role and word exist: nested role, nested word, right role, left type, right type,
	// left mention type, right mention type, predicate
	if (!atoms[3].is_null() && !atoms[8].is_null()) {
		ngram[0] = atoms[8];
		ngram[1] = atoms[3];
		ngram[2] = atoms[7];
		ngram[3] = atoms[4];
		ngram[4] = atoms[5];
		ngram[5] = atoms[10];
		ngram[6] = atoms[11];
		ngram[7] = atoms[0];
		event->addComplexContextPredicate(8, ngram);
		// substitute last hanzi for predicate
		ngram[7] = atoms[9];
		event->addComplexContextPredicate(8, ngram);
		// substitute clusters for predicate
		if (n_predicate_clusters > 0) {
			for (int i = 0; i < n_predicate_clusters; i++) {
				ngram[7] = predicateClusters[i];
				event->addComplexContextPredicate(8, ngram);
			}
		}
		// leave off predicate all together
		event->addComplexContextPredicate(7, ngram);
	}

	// if there is metonymy: left type, right type, [left metonymy], [right metonymy], predicate
	if (!atoms[12].is_null() && !atoms[13].is_null()) {
		ngram[0] = atoms[4];
		ngram[1] = atoms[5];
		ngram[2] = atoms[12];
		ngram[3] = atoms[13];
		ngram[4] = atoms[0];
		event->addComplexContextPredicate(5, ngram);
		// substiture last hanzi for predicate
		ngram[4] = atoms[9];
		event->addComplexContextPredicate(5, ngram);
		// substitute clusters for predicate
		if (n_predicate_clusters > 0) {
			for (int i = 0; i < n_predicate_clusters; i++) {
				ngram[4] = predicateClusters[i];
				event->addComplexContextPredicate(5, ngram);
			}
		}
	} else if (!atoms[12].is_null()) {
		ngram[0] = atoms[4];
		ngram[1] = atoms[5];
		ngram[2] = atoms[12];
		ngram[3] = atoms[0];
		event->addComplexContextPredicate(4, ngram);
		// substiture last hanzi for predicate
		ngram[3] = atoms[9];
		event->addComplexContextPredicate(4, ngram);
		// substitute clusters for predicate
		if (n_predicate_clusters > 0) {
			for (int i = 0; i < n_predicate_clusters; i++) {
				ngram[3] = predicateClusters[i];
				event->addComplexContextPredicate(4, ngram);
			}
		}
	} else if (!atoms[13].is_null()) {
		ngram[0] = atoms[4];
		ngram[1] = atoms[5];
		ngram[2] = atoms[13];
		ngram[3] = atoms[0];
		event->addComplexContextPredicate(4, ngram);
		// substiture last hanzi for predicate
		ngram[3] = atoms[9];
		event->addComplexContextPredicate(4, ngram);
		// substitute clusters for predicate
		if (n_predicate_clusters > 0) {
			for (int i = 0; i < n_predicate_clusters; i++) {
				ngram[3] = predicateClusters[i];
				event->addComplexContextPredicate(4, ngram);
			}
		}
	}

	// left type, right type, left role, right role, left mention type, right mention type, predicate, [nested role]
	ngram[0] = atoms[4];
	ngram[1] = atoms[5];
	ngram[2] = atoms[6];
	ngram[3] = atoms[7];
	ngram[4] = atoms[10];
	ngram[5] = atoms[11];
	ngram[6] = atoms[0];
	ngram[7] = atoms[8];
	if (!ngram[7].is_null())
		event->addComplexContextPredicate(8, ngram);
	else
		event->addComplexContextPredicate(7, ngram);
	// substiture last hanzi for predicate
	ngram[6] = atoms[9];
	if (!ngram[7].is_null())
		event->addComplexContextPredicate(8, ngram);
	else
		event->addComplexContextPredicate(7, ngram);
	// substitute clusters for predicate
	if (n_predicate_clusters > 0) {
		for (int i = 0; i < n_predicate_clusters; i++) {
			ngram[6] = predicateClusters[i];
			if (!ngram[7].is_null())
				event->addComplexContextPredicate(8, ngram);
			else
				event->addComplexContextPredicate(7, ngram);
		}
	}
	// take out predicate
	ngram[6] = ngram[7];
	if (!ngram[6].is_null())
		event->addComplexContextPredicate(7, ngram);
	else
		event->addComplexContextPredicate(6, ngram);

	// left mention type, right mention type, left type, right type, predicate
	ngram[0] = atoms[10];
	ngram[1] = atoms[11];
	ngram[2] = atoms[4];
	ngram[3] = atoms[5];
	ngram[4] = atoms[0];
	event->addComplexContextPredicate(5, ngram);
	// substitute last hanzi for predicate
	ngram[4] = atoms[9];
	event->addComplexContextPredicate(5, ngram);
	// substitute clusters for predicate
	if (n_predicate_clusters > 0) {
		for (int i = 0; i < n_predicate_clusters; i++) {
			ngram[4] = predicateClusters[i];
			event->addComplexContextPredicate(5, ngram);
		}
	}


	delete [] atoms;
	delete [] ngram;

}


void MaxEntRelationModel::fillMaxEntExistanceEvent(MaxEntEvent *event, ChinesePotentialRelationInstance *inst) {

	fillMaxEntTypeEvent(event, inst);

	int type = RelationTypeSet::getTypeFromSymbol(inst->getRelationType());
	if (RelationTypeSet::isNull(type))
		event->setOutcome(NO_RELATION);
	else
		event->setOutcome(IS_RELATION);
}

int MaxEntRelationModel::readTrainingVector(UTF8InputStream &stream, Symbol *ngram) {

	UTF8Token token;
	char message[1000];

	stream >> token;
	if (token.symValue() != SymbolConstants::leftParen) {
		sprintf(message, "%s%s%s", "ERROR: ill-formed training vector. Token: ", token.symValue().to_string(), ". Expected left parenthesis.");
		throw UnexpectedInputException("MaxEntRelationModel::readTrainingVector()", message);

	}

    stream >> token;
	if (token.symValue() != SymbolConstants::leftParen) {
		sprintf(message, "%s%s%s", "ERROR: ill-formed training vector. Token: ", token.symValue().to_string(), ". Expected left parenthesis.");
        throw UnexpectedInputException("MaxEntRelationModel::readTrainingVector()", message);
	}

    for (int j = 0; j < CH_POTENTIAL_RELATION_INSTANCE_NGRAM_SIZE; j++) {
        stream >> token;
        ngram[j] = token.symValue();
    }

    stream >> token;
	if (token.symValue() != SymbolConstants::rightParen) {
		sprintf(message, "%s%s%s", "ERROR: ill-formed training vector. Token: ", token.symValue().to_string(), ". Expected right parenthesis.");
		throw UnexpectedInputException("MaxEntRelationModel::readTrainingVector()", message);
	}

	int count;
    stream >> count;

    stream >> token;
    if (token.symValue() != SymbolConstants::rightParen) {
		sprintf(message, "%s%s%s", "ERROR: ill-formed training vector. Token: ", token.symValue().to_string(), ". Expected right parenthesis.");
        throw UnexpectedInputException("MaxEntRelationModel::readTrainingVector()", message);
    }

	return count;

}
