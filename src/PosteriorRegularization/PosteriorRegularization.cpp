// experiment with posterior regularization on a simple, artificial clustering problem.
#include "Generic/common/leak_detection.h"
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/common/foreach_pair.hpp"

#include "PosteriorRegularization.h"
#include "constraints/ClassRequiresFeatureConstraint.h"
#include "constraints/AtMostNOfClassConstraint.h"
#include "constraints/AtLeastNOfClassIfFeatureConstraint.h"
#include "../GraphicalModels/VectorUtils.h"
#include "../GraphicalModels/DataSet.h"
#include "../GraphicalModels/Graph.h"
#include "../GraphicalModels/Factor.h"
#include "../GraphicalModels/Variable.h"
#include "../GraphicalModels/Message.h"
#include "../GraphicalModels/learning/EM.h"
#include "../GraphicalModels/learning/PREM.h"
#include "../GraphicalModels/io/Reporter.h"
#include "../GraphicalModels/pr/FeatureImpliesClassConstraint.h"

wchar_t* CLASS_NAMES[] = { L"AIR", L"FN", L"MOD", L"LOC", L"CO", L"GAR", L"GAR_LOC" };
const double BMMFeatureFactor::SMOOTHING = 0.1;
const double ArgTypePriorFactor::SMOOTHING = 0.5;

using namespace std;
using boost::make_shared;
using boost::static_pointer_cast;
using boost::shared_ptr;
using std::vector;
using GraphicalModel::DataSet;
using GraphicalModel::Constraint;
using GraphicalModel::Message_ptr;
using GraphicalModel::SymmetricDirichlet;
using GraphicalModel::FeatureImpliesClassConstraint;

typedef boost::shared_ptr<GraphicalModel::DataSet<Document> > DataSet_ptr;

void Document::finalize(const std::vector<FV_ptr>& featureVectors) {
	assert(slots.size() == featureVectors.size());

	for (size_t i=0; i<slots.size(); ++i) {
		const SlotVariable_ptr& slot = slots[i];

		BMMFeatureFactor_ptr ff = make_shared<BMMFeatureFactor>(slot);
		BOOST_FOREACH(const std::wstring& feature, *featureVectors[i]) {
			ff->addFeature(feature);
		}
		connect(*ff, *slot);
		featureFactors.push_back(ff);

		ArgTypePriorFactor_ptr apf = make_shared<ArgTypePriorFactor>();
		connect(*apf, *slot);
		priorFactors.push_back(apf);
	}
}

void BMMFeatureFactor::finishedLoading(unsigned int n_classes) {
	BMMFeatureFactorParent::finishedLoading(n_classes, false,
			make_shared<SymmetricDirichlet>(SMOOTHING));
}

void ArgTypePriorFactor::finishedLoading(unsigned int n_classes) {
	ATPFParent::finishedLoading(n_classes,
			make_shared<SymmetricDirichlet>(ArgTypePriorFactor::SMOOTHING));
}

void Document::finishedLoading() {
	BMMFeatureFactor::finishedLoading(NUM_CLASSES);
	ArgTypePriorFactor::finishedLoading(NUM_CLASSES);
}

void Document::clearFactorModificationsImpl() {
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, featureFactors) {
		featureFactor->clearModifiers();
	}
	BOOST_FOREACH(const ArgTypePriorFactor_ptr& priorFactor, priorFactors) {
		priorFactor->clearModifiers();
	}
}


void Document::numberFactorsImpl(unsigned int& next_id) {
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, featureFactors) {
		featureFactor->setID(next_id++);
	}
	BOOST_FOREACH(const ArgTypePriorFactor_ptr& priorFactor, priorFactors) {
		priorFactor->setID(next_id++);
	}
}

unsigned int Document::nFactorsImpl() const {
	return featureFactors.size() + priorFactors.size();
}

unsigned int Document::nVariablesImpl() const {
	return slots.size();
}

/*unsigned int Document::nRootFactors() const {
	return featureFactors.size() + priorFactors.size();
	}

SlotVariable& Document::variable(unsigned int i) const {
	return *slots[i];
}

GraphicalModel::ModifiableUnaryFactor& Document::factor(unsigned int i) const {
	if (i<featureFactors.size()) {
		return *featureFactors[i];
	} else {
		return *priorFactors[i - featureFactors.size()];
	}
}

GraphicalModel::ModifiableUnaryFactor& Document::rootFactor(unsigned int i) const {
	return factor(i);
}
*/

/*
void Document::clearFactorModifications() {
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, featureFactors) {
		featureFactor->clearModifiers();
	}
	BOOST_FOREACH(const ArgTypePriorFactor_ptr& priorFactor, priorFactors) {
		priorFactor->clearModifiers();
	}
}*/


double Document::inferenceImpl() {
	double LL = 0.0;
	// first reset everything
	BOOST_FOREACH(const SlotVariable_ptr& slot, slots) {
		slot->clearMessages();
	}
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, featureFactors) {
		featureFactor->clearMessages();
	}
	BOOST_FOREACH(const ArgTypePriorFactor_ptr& priorFactor, priorFactors) {
		priorFactor->clearMessages();
	}

	// all factors are roots and all roots are factors, so simply send messages
	// from all factors and we are done.
	BOOST_FOREACH(const BMMFeatureFactor_ptr& featureFactor, featureFactors) {
		featureFactor->sendMessages();
	}
	BOOST_FOREACH(const ArgTypePriorFactor_ptr& priorFactor, priorFactors) {
		priorFactor->sendMessages();
	}
	std::vector<double> messageProducts(NUM_CLASSES);
	BOOST_FOREACH(const SlotVariable_ptr& slot, slots) {
		slot->receiveMessage();

		slot->productOfIncomingMessages(messageProducts);
		LL += logSumExp(messageProducts);
	}
	return LL;
}

void SlotVariable::dump() const {
	wcout << L"Incoming msgs: " << endl;
	BOOST_FOREACH(const Message_ptr& msg, incomingMessages()) {
		wcout << L"\t";
		for (unsigned int i =0; i<msg->size(); ++i) {
			wcout << (*msg)[i] << L"\t";
		}
		wcout << endl;
	}
	wcout << L"Marginals: ";
	BOOST_FOREACH(double p, marginals()) {
		wcout << p << L"\t";
	}
	wcout << endl;
}

void Document::initializeRandomlyImpl() {
	std::vector<double> tmp(NUM_CLASSES);

	BOOST_FOREACH(const SlotVariable_ptr& slot, slots) {
		double sum = 0;
		BOOST_FOREACH(double& x, tmp) {
			x = rand() % 25 + 5;
			sum += x;
		}
		BOOST_FOREACH(double& x, tmp) {
			x/=sum;
		}
		slot->setMarginals(tmp);
	}
}

void Document::observeImpl() {
	for (size_t i=0; i<slots.size(); ++i) {
		featureFactors[i]->observe(*slots[i]);
		priorFactors[i]->observe(*slots[i]);
	}
}

void Document::clearCountsImpl() {
	ArgTypePriorFactor::clearCounts();
	BMMFeatureFactor::clearCounts();
}

template <typename Dumper>
void Document::updateParametersFromCounts(const Dumper& dumper) {
	ArgTypePriorFactor::updateParametersFromCounts(dumper);
	BMMFeatureFactor::updateParametersFromCounts(dumper);
}

FV_ptr fv(unsigned int slotNum, unsigned int slotIdx, const wstring& slotEntityType,
		const vector<wstring>& words)
{
	FV_ptr vec = make_shared<FV>();
	wstring token = words[slotIdx];
	vec->push_back(token);
	std::vector<wstring> tokParts;
	boost::split(tokParts, token, boost::is_any_of(L" "));

	if (slotEntityType != L"O") {
		vec->push_back(slotEntityType + L"_entity_type");
	}

	if (tokParts.size() > 0) {
		BOOST_FOREACH(const wstring& part, tokParts) {
			vec->push_back(part + L"_part");
		}
	}

	switch(tokParts.size()) {
		case 1:
			vec->push_back(L"single_word");
			break;
		case 2:
			vec->push_back(L"two_words");
			break;
		case 3:
			vec->push_back(L"three_words");
			break;
		case 4:
			vec->push_back(L"four_words");
			break;
		default:
			vec->push_back(L"more_than_four_words");
			break;
	}
	
	if (slotNum == 0) {
		vec->push_back(L"first_slot");
	} else if (slotNum == 1) {
		vec->push_back(L"second_slot");
	} else if (slotNum == 2) {
		vec->push_back(L"third_slot");
	} else if (slotNum == 3) {
		vec->push_back(L"fourth_or_later_slot");
	}

	if (slotIdx > 0) {
		vec->push_back(words[slotIdx-1] + L"_prev_unigram");
		if (slotIdx > 1) {
			vec->push_back(words[slotIdx-2] 
						+ L"_" + words[slotIdx-1] + L"_prev_bigram");
		}
	}

	if (slotIdx < words.size() - 1) {
		vec->push_back(words[slotIdx+1] + L"_next_unigram");
		if (slotIdx < words.size() -2) {
			vec->push_back(words[slotIdx+1] + L"_" 
						+ words[slotIdx+2] + L"_next_bigram");
		}
	}

	return vec;
} 

class Label {
	public: 
		Label(const wstring& str) {
			bco = str[0];
			if (str == L"O") {
				gs = L"null";
			} else {
				if (str.length() < 3 || str[1] != L'-') {
					wcerr << str << endl;
					throw BadLineException(L"Cannot parse label");
				}
				gs = str.substr(2);
			}
		}
		wchar_t bco;
		wstring gs;
};

struct SlotEntry {
	SlotEntry(unsigned int index, const wstring& type, const wstring& entityType)
		: index(index), type(type), entityType(entityType) {}
	unsigned int index;
	wstring type;
	wstring entityType;
};

void finishDocument(bool& in_slot, vector<SlotEntry>& slotIndicesAndTypes,
		vector<wstring>& words, wstring& accum, wstring& slotLabel, 
		wstring& slotEntityType, bool annotated, 
		const DataSet_ptr& data)
{
	if (in_slot) {
		slotIndicesAndTypes.push_back(
				SlotEntry(words.size(), slotLabel, slotEntityType));
		words.push_back(accum);
		accum = L"";
	}
	Document_ptr docInstances = make_shared<Document>();
	unsigned int slot_num = 0;
	vector<FV_ptr> fvs;

	BOOST_FOREACH(SlotEntry indexAndType, slotIndicesAndTypes) {
		SlotVariable_ptr slotInstance = make_shared<SlotVariable>();
		slotInstance->annotated = annotated;
		slotInstance->gold_label = SlotVariable::classIndex(indexAndType.type);
		slotInstance->tok_idx = indexAndType.index;
		docInstances->slots.push_back(slotInstance);
		fvs.push_back(fv(slot_num, indexAndType.index, indexAndType.entityType,
				words));
		++slot_num;
	}

	if (!docInstances->slots.empty()) {
		docInstances->words = words;
		docInstances->finalize(fvs);
		//data->documents.push_back(docInstances);
		data->addGraph(docInstances);
	} 
	words.clear();
	accum = L"";
	slotLabel = L"";
	slotEntityType = L"";
	slotIndicesAndTypes.clear();
	in_slot = false;
}

void loadData(const string& filename, const DataSet_ptr& data) 
{
	wifstream inp(filename.c_str());

	wstring line;
	bool first_line = true;
	bool annotated = false;
	bool in_slot = false;
	vector<wstring> words;
	wstring accum = L"";
	vector<SlotEntry> slotIndicesAndTypes;
	wstring slotLabel = L"";
	wstring slotEntityType = L"";
	wstring entityType = L"";

	try {
		while (getline(inp, line)) {
			if (!line.empty()) {
				if (line.find(L"Biography of") == 0) {
					if (first_line) {
						first_line = false;
					} else {
						finishDocument(in_slot, slotIndicesAndTypes, words, accum,
								slotLabel, slotEntityType, annotated, data);
					}

					wstring year = line.substr(13,4);
					if (year ==  L"2006" || year == L"2007" || year == L"2008"
							|| year == L"2009")
					{
						annotated = true;
					} else {
						annotated = false;
					}
				} else {
					vector<wstring> lineParts;
					boost::split(lineParts, line, boost::is_any_of(L" "));
					if (lineParts.size() != 4 || lineParts[1].empty()) {
						throw BadLineException(L"Wrong number or empty parts");
					} else {
						wstring token = lineParts[0];
						transform(token.begin(), token.end(), token.begin(), ::tolower);
						for(unsigned int i = 0; i < token.size(); ++i) {
							if (iswdigit(token[i])) {
								token[i] = L'$';
							}
						}

						Label label = Label(lineParts[1]);
						wstring entityType = lineParts[2];

						if (in_slot) {
							if (label.bco == L'B') {
								slotIndicesAndTypes.push_back(
										SlotEntry(words.size(), slotLabel,
											slotEntityType));
								words.push_back(accum);
								accum = token;
								slotLabel = label.gs;
								slotEntityType = entityType;
							} else if (label.bco == L'O') {
								slotIndicesAndTypes.push_back(
										SlotEntry(words.size(), slotLabel,
											slotEntityType));
								words.push_back(accum);
								accum = L"";
								in_slot = false;
								words.push_back(token);
							} else if (label.bco == L'C') {
								accum += L" " + token;
							} else {
								throw BadLineException(L"Unknown label, in slot");
							}
						} else {
							if (label.bco == L'O') {
								words.push_back(token);
							} else if (label.bco == 'B') {
								accum = token;
								in_slot = true;
								slotLabel = label.gs;
								slotEntityType = entityType;
							} else {
								throw BadLineException(L"Unknown label, out slot");
							}
						}
					}
				}
			}
		}
		finishDocument(in_slot, slotIndicesAndTypes, words, accum,
				slotLabel, slotEntityType, annotated, data);
	} catch (BadLineException& e) {
		wcerr << L"Failed to parse line: " << line << endl;
		wcerr << e.msg << endl;
		throw;
	}
	wcout << L"Loaded " << data->graphs.size() << L" documents." << endl;
}

double score(const GraphicalModel::DataSet<Document>& data, int cls, int cluster) {
	int true_positives = 0;
	int false_positives = 0;
	int false_negatives = 0;

	BOOST_FOREACH(Document_ptr doc, data.graphs) {
		BOOST_FOREACH(SlotVariable_ptr slot, doc->slots) {
			if (slot->annotated) {
				unsigned int best_label = (unsigned int)slot->bestLabel();
				if (best_label == cluster) {
					if (slot->gold_label == cls) {
						++true_positives;
					} else {
						++false_positives;
					}
				} else if (slot->gold_label == cls) {
					++false_negatives;
				}
			}
		}
	}

	if (true_positives == 0) {
		return 0.0;
	} else {
		double precision = ((double)true_positives) / (true_positives + false_positives);
		double recall = ((double)true_positives) / (true_positives + false_negatives);
		return 2.0*precision*recall/(precision+recall);
	}
}

vector<unsigned int> score(const GraphicalModel::DataSet<Document>& data) {
	vector<unsigned int> bestAlignment(NUM_CLASSES);

	for (int cls=0; cls<NUM_CLASSES; ++cls) {
		int best = -1;
		double best_score = -1.0;
		for (int cluster = 0; cluster<NUM_CLASSES; ++cluster) {
			double x = score(data, cls, cluster);
			wcout << L"align class " << cls << L" to cluster " << cluster 
				<< L" results in F " << x << endl;
			if (x > best_score) {
				best = cluster;
				best_score = x;
			}
		}
		wcout << L"Best alignment for class " << CLASS_NAMES[cls] 
			<< L" is to cluster " << best << L" with an f-measure of "
			<< best_score << endl;
		bestAlignment[cls] = best;
	}
	return bestAlignment;
}

wchar_t * COLOR_CSS = L".cluster0 { background-color: #FF0033; }\n"
L".cluster1 { background-color: #99FF00; }\n"
L".cluster2 { background-color: #99CCFF; }\n"
L".cluster3 { background-color: #FFFFCC; }\n"
L".cluster4 { background-color: #9966FF; }\n"
L".cluster5 { background-color: #FFCC33; }\n"
L".cluster6 { background-color: #00FF7F; }\n";

void dumpFeatureVector(int cnt, Document_ptr doc) {
	wcout << "Document " << cnt << endl;
	for (unsigned int slot_count = 0; slot_count < doc->slots.size(); ++slot_count) {
		wcout << L"Slot " << slot_count << L": ";
		BOOST_FOREACH(unsigned int feat, doc->featureFactors[slot_count]->features()) {
			wcout << BMMFeatureFactor::featureName(feat) << L" ";
		}
		wcout << endl;
		wcout << L"\t";
		for (int i=0; i<NUM_CLASSES; ++i) {
			wcout << CLASS_NAMES[i] << L"=" << setprecision(2) 
				<< doc->slots[slot_count]->marginals()[i] << L"\t";
		}
		wcout << endl;
	}
}

template <typename Model>
void applyModel(const string& modelName, Model& model, 
		GraphicalModel::DataSet<Document>& data) {
	model.e(data);
	unsigned int cnt = 0;
	vector<unsigned int> bestAlignments = score(data);
	wofstream out((string("debug_") + modelName + ".html").c_str());
	out << L"<html><head><title>foo</title><style>" << COLOR_CSS << L"</style></head><body>";
	out << "L<b>Key:</b> ";
	for (int i = 0; i < NUM_CLASSES; ++i) {
		out << L"<div class='cluster" << bestAlignments[i] << L"'><b>" << CLASS_NAMES[i]
			<< L"</b></div>";
	}
	BOOST_FOREACH(const Document_ptr& doc, data.graphs) {
		++cnt;
		vector<SlotVariable_ptr>::const_iterator slotIt = doc->slots.begin();
		/*dumpFeatureVector(cnt, doc, alphabet); */
		out << L"<p>";
		for (int i=0; i<doc->words.size(); ++i) {
			if (slotIt != doc->slots.end()) {
				if (i == (*slotIt)->tok_idx) {
					out << L"<span class='cluster" << (*slotIt)->bestLabel()
						<< L"'><b>";
				}
			}
			out << doc->words[i] << L" ";
			if (slotIt != doc->slots.end()) {
				if (i == (*slotIt)->tok_idx) {
					out << L"</b></span>";
					++slotIt;
				}
			}
		}
		out << L"</p>";
	}
}


void createConstraints(const GraphicalModel::DataSet<Document>& dataset, 
		vector<shared_ptr<GraphicalModel::Constraint<Document> > >& constraints)
{
	vector<unsigned int> flightClasses, airlineClasses, modelClasses,
		garbageClasses, locationClasses, countryClasses, locOrCoClasses,
		countryOrGarbageLocClasses, locLikeClasses, locOrGarLocClasses,
		locationOnlyClasses, atMostOneCountryClasses, atMostTwoLocationsClasses,
		atLeastOneLocationIfLocClasses;
	vector<wstring> flightFeatures, airlineFeatures, modelFeatures,
		garbageFeatures, locationFeatures, countryFeatures, locOrCoFeatures,
		countryOrGarbageLocFeatures, locLikeFeatures, locOrGarLocFeatures,
		locationOnlyFeatures, atLeastOneLocationIfLocFeatures;

	flightClasses.push_back(FLIGHT_NUMBER);
	airlineClasses.push_back(AIRLINE);
	modelClasses.push_back(AIRCRAFT_MODEL);
	garbageClasses.push_back(GARBAGE);
	locationClasses.push_back(LOCATION);
	locOrCoClasses.push_back(LOCATION);
	locOrCoClasses.push_back(COUNTRY);
	countryClasses.push_back(COUNTRY);
	countryOrGarbageLocClasses.push_back(COUNTRY);
	countryOrGarbageLocClasses.push_back(GAR_LOC);
	locLikeClasses.push_back(LOCATION);
	locLikeClasses.push_back(COUNTRY);
	locLikeClasses.push_back(GAR_LOC);
	locOrGarLocClasses.push_back(LOCATION);
	locOrGarLocClasses.push_back(GAR_LOC);
	locationOnlyClasses.push_back(LOCATION);
	locationOnlyClasses.push_back(COUNTRY);
	locationOnlyClasses.push_back(GAR_LOC);

	atMostOneCountryClasses.push_back(COUNTRY);
	atMostTwoLocationsClasses.push_back(LOCATION);
	atLeastOneLocationIfLocClasses.push_back(LOCATION);

	flightFeatures.push_back(L"flight_part");

	airlineFeatures.push_back(L"airlines_part");
	airlineFeatures.push_back(L"airways_part");

	modelFeatures.push_back(L"boeing_part");
	modelFeatures.push_back(L"airbus_part");
	modelFeatures.push_back(L"vickers_part");
	modelFeatures.push_back(L"dc-$$_part");
	modelFeatures.push_back(L"dc-$_part");
	modelFeatures.push_back(L"f-$$_part");
	
	garbageFeatures.push_back(L"decompression_part");
	garbageFeatures.push_back(L"collision_part");
	garbageFeatures.push_back(L"crash_part");
	garbageFeatures.push_back(L"crashed_part");
	garbageFeatures.push_back(L"crashes_part");
	garbageFeatures.push_back(L"explodes_part");
	garbageFeatures.push_back(L"collides_part");
	garbageFeatures.push_back(L"bird strike");
	garbageFeatures.push_back(L"hijacked_part");
	garbageFeatures.push_back(L"including_prev_unigram");
	garbageFeatures.push_back(L"including_the_prev_bigram");
	garbageFeatures.push_back(L"PERSON_entity_type");

	locationFeatures.push_back(L"into_prev_unigram");
	locationFeatures.push_back(L"near_prev_unigram");
	locationFeatures.push_back(L"over_the_prev_bigram");
	locationFeatures.push_back(L"north_of_prev_bigram");
	locationFeatures.push_back(L"mountains_part");
	locationFeatures.push_back(L"mount_part");
	locationFeatures.push_back(L"at_prev_unigram");
	locationFeatures.push_back(L"takeoff_from_prev_bigram");
	locationFeatures.push_back(L"landing_at_prev_bigram");
	locationFeatures.push_back(L"river_part");

	countryOrGarbageLocFeatures.push_back(L"belgium");
	countryOrGarbageLocFeatures.push_back(L"finland");
	countryOrGarbageLocFeatures.push_back(L"norway");
	countryOrGarbageLocFeatures.push_back(L"scotland");
	countryOrGarbageLocFeatures.push_back(L"egypt");
	countryOrGarbageLocFeatures.push_back(L"ireland");
	countryOrGarbageLocFeatures.push_back(L"canada");
	countryOrGarbageLocFeatures.push_back(L"france");

	locLikeFeatures.push_back(L"LOCATION_entity_type");

	locOrCoFeatures.push_back(L"over_prev_unigram");
	locOrCoFeatures.push_back(L"in_prev_unigram");

	locOrGarLocFeatures.push_back(L"indiana");
	locOrGarLocFeatures.push_back(L"california");
	locOrGarLocFeatures.push_back(L"to_the_prev_bigram");

	locationOnlyFeatures.push_back(L"LOCATION_entity_type");

	atLeastOneLocationIfLocFeatures.push_back(L"LOCATION_entity_type");

	typedef FeatureImpliesClassConstraint<BMMFactorConstraintView> FICC;
	constraints.push_back(make_shared<FICC>(dataset, flightFeatures, 
				flightClasses, 0.8, BMMFactorConstraintView()));
	constraints.push_back(make_shared<FICC>(dataset, airlineFeatures, 
				airlineClasses, 0.9, BMMFactorConstraintView()));
	constraints.push_back(make_shared<FICC>(dataset,
				modelFeatures, modelClasses, 0.9, BMMFactorConstraintView()));
	constraints.push_back(make_shared<FICC>(dataset,
				garbageFeatures, garbageClasses, 0.8, BMMFactorConstraintView()));
	constraints.push_back(make_shared<FICC>(dataset,
				locationFeatures, locationClasses, 0.7, BMMFactorConstraintView()));
	constraints.push_back(make_shared<FICC>(dataset,
				locOrCoFeatures, locOrCoClasses, 0.8, BMMFactorConstraintView()));
	constraints.push_back(make_shared<FICC>(dataset,
				countryOrGarbageLocFeatures, countryOrGarbageLocClasses, 0.9, 
				BMMFactorConstraintView()));
	constraints.push_back(make_shared<FICC>(dataset,
				locLikeFeatures, locLikeClasses, 0.9, BMMFactorConstraintView()));
	constraints.push_back(make_shared<FICC>(dataset,
				locOrGarLocFeatures, locOrGarLocClasses, 0.9, BMMFactorConstraintView()));
	constraints.push_back(make_shared<ClassRequiresFeatureConstraint>(dataset,
	 locationOnlyFeatures, locationOnlyClasses, 0.1));
	constraints.push_back(make_shared<AtMostNOfClassConstraint>(
				atMostOneCountryClasses, 1.0));
	constraints.push_back(make_shared<AtMostNOfClassConstraint>(
				atMostTwoLocationsClasses, 2.0));
	constraints.push_back(make_shared<AtLeastNOfClassIfFeatureConstraint>(
				dataset, atLeastOneLocationIfLocClasses, 
				atLeastOneLocationIfLocFeatures, 1.0));
}

struct DataDumper {
	void postE(unsigned int i, double ll, const DataSet<Document>& data) const {
		wcout << L"Iteration " << i << L", LL=" << ll << endl;
	}
	void postE(const DataSet<Document>& data) const {

	}
	void postM(unsigned int i, const DataSet<Document>& data) const {}
};

class FixMe {};

int main(int argc, char** argv) {
	srand((unsigned int)time(NULL));
	if (argc < 2 ) {
		cerr << "Must provide data file" << endl;
	} else {
		DataSet_ptr data = make_shared<GraphicalModel::DataSet<Document> >();
		loadData(argv[1], data);
		Document::finishedLoading();
		wcout << "\n\n\nTrying PR..." << endl;
		vector<shared_ptr<Constraint<Document> > > constraints;
		createConstraints(*data, constraints);
		//GraphicalModel::PREM<Document> prModel(*data, NUM_CLASSES, constraints);
		//prModel.em(*data, 15, Reporter());
		const char* source = "posterior regularization main";
		const char* msg = "Change to logging broke this. Fix it if you plan to use it.";
		throw FixMe();
		//applyModel("PR", prModel, *data);
	}
}

