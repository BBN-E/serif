#include "Generic/common/leak_detection.h"
#include "TemporalAttributeAdder.h"

#include <sstream>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/TimexUtils.h"
#include "ActiveLearning/alphabet/MultiAlphabet.h"
#include "Temporal/FeatureMap.h"
#include "Temporal/TemporalAttribute.h"
#include "Temporal/ManualTemporalInstanceGenerator.h"
#include "Temporal/TemporalDB.h"
#include "Temporal/TemporalTypeTable.h"
#include "Temporal/features/TemporalFeature.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "PredFinder/elf/ElfIndividual.h"
#include "PredFinder/inference/EIUtils.h"

using boost::make_shared;

TemporalAttributeAdder::TemporalAttributeAdder(TemporalDB_ptr db) :
	_typeTable(db->makeTypeTable()), _db(db),
_instanceGenerator(_new ManualTemporalInstanceGenerator(_typeTable)),
	_features(db->features(ParamReader::getRequiredFloatParam("temporal_feature_threshold"))),
	_record_temporal_source(
		ParamReader::getOptionalTrueFalseParamWithDefaultVal(
				"record_temporal_source", false)),
	_debug_matching(ParamReader::isParamTrue("debug_temporal"))
{
}

void TemporalAttributeAdder::addAttributeToRelation(ElfRelation_ptr relation,
		TemporalAttribute_ptr attribute, const std::wstring& scoreString,
		const DocTheory* doc_theory, const std::wstring& proposalSource)
{
	if (const ValueMention* vm = attribute->valueMention()) {
		if (!relationAlreadyContainsVM(relation, vm)) {
			ElfIndividual_ptr individual = boost::make_shared<ElfIndividual>(
					doc_theory, L"xsd:string", vm);

			stripSuffixes(individual);
			bool convertedWeek = convertWeek(individual);

			std::wstring attName = _typeTable->name(attribute->type());

			if (attName.find(L"t:") != 0) {
				SessionLogger::warn("bad_temporal_attribute_name")
					<< L"Temporal attribute does not begin with 't:' : "
					<< attName;
			}
			if(L"t:HoldsAt" == attName){
				attName = L"t:HoldsWithin";
			}
			std::wstring specString = L"[[,],[,]]";
			ValueMention* emptyDate = 0;	
			if(attName == L"t:ClippedBackward"){
				specString =  EIUtils::getKBPSpecString(doc_theory, vm, emptyDate);
			} else 	if(attName == L"t:ClippedForward"){
				specString =  EIUtils::getKBPSpecString(doc_theory, emptyDate, vm);
			} else if(attName == L"t:HoldsWithin"){
				specString =  EIUtils::getKBPSpecStringHoldsWithin(doc_theory, vm);
			}
			else{
				std::wcout<<"Unknown attName: "<<attName<<std::endl;
			}

			individual->set_value(specString);
			






			ElfRelationArg_ptr tempArg = boost::make_shared<ElfRelationArg>(
					attName, individual);
			relation->insert_argument(tempArg);

			if (_record_temporal_source) {
				relation->add_source(scoreString);

				if (proposalSource != L"") {
					std::wstringstream attachSrc;
					attachSrc << attName << L"-attached-by-" << proposalSource;
					relation->add_source(attachSrc.str());
				}
			}
		} else {
			if (_record_temporal_source) {
				relation->add_source(L"Suppressed(" + scoreString + L")");
			}
		}
	} else {
		SessionLogger::warn("temp_att_no_vm") << L"Temporal attribute has no "
			<< L" value mention in TemporalAttributeAdder::addAttributeToRelation";
	}
}

void TemporalAttributeAdder::stripSuffixes(ElfIndividual_ptr individual) {
	using namespace boost::gregorian;
	const std::wstring timex = individual->get_value();
	date d =TimexUtils::dateFromTimex(timex, false);
	if (d == date(not_a_date_time)) {
		d = TimexUtils::dateFromTimex(timex, true);
		if (d != date(not_a_date_time)) {
			std::wstring new_timex = TimexUtils::dateToTimex(d);
			individual->set_value(new_timex);
		}
		std::vector<std::wstring> yyyymmdd;
		TimexUtils::parseYYYYMMDD(timex, yyyymmdd, true);
	
		if(yyyymmdd.size() == 2){
			individual->set_value(yyyymmdd[0]+L"-"+yyyymmdd[1]);
		} 
		else if(yyyymmdd.size() == 1){
			individual->set_value(yyyymmdd[0]);
		}		
	}
}

bool TemporalAttributeAdder::convertWeek(ElfIndividual_ptr individual) {
	using namespace boost::gregorian;
	const std::wstring timex = individual->get_value();
	int year = -1, week = -1;
	if (TimexUtils::parseYYYYWW(timex, year, week)) {
		std::wstring new_value = EIUtils::DatePeriodToKBPSpecString(
				TimexUtils::ISOWeek(year, week));
		individual->set_value(new_value);
		return true;
	}
	return false;
}

bool TemporalAttributeAdder::relationAlreadyContainsVM(ElfRelation_ptr relation,
		const ValueMention* vm)
{
	BOOST_FOREACH(ElfRelationArg_ptr arg, relation->get_args()) {
		ElfIndividual_ptr indiv = arg->get_individual();
		if (indiv) {
			if (indiv->has_value_mention_id()) {
				if (indiv->get_value_mention_id() == vm->getUID().toInt()) {
					return true;
				}
			}
		}
	}
	return false;
}

double TemporalAttributeAdder::scoreInstance(TemporalInstance_ptr inst, 
		const DocTheory* dt, int sn)
{
	double score = 0.0;

	// evaluation hack because it's faster than adding IsAffiliatedWith
	// to our training data
	bool affiliated_hack = doAffiliatedHack(inst);

	if (_debug_matching) {
		SessionLogger::info("temporal_debug") << L"\tFor attribute " 
			<< inst->attribute()->type() << L":";
	}

	BOOST_FOREACH(const TemporalFeature_ptr& feature, _features) {
		if (feature->matches(inst, dt, sn)) {
			score += feature->weight(inst->attribute()->type());

			if (_debug_matching) {
				SessionLogger::info("temporal_debug") << L"\t\t"
					<< feature->pretty() << " = " 
					<< feature->weight(inst->attribute()->type());
			}
		}
	}

	if (_debug_matching) {
		SessionLogger::info("temporal_debug") << L"\tRaw: " << 
			 score << "; Prob: " << 1.0/(1.0 + exp(-score));
	}

	if (affiliated_hack) {
		undoAffiliatedHack(inst);
	}

	return 1.0/(1.0 + exp(-score));
}

TemporalAttributeAdder::~TemporalAttributeAdder() {}

bool TemporalAttributeAdder::doAffiliatedHack(TemporalInstance_ptr inst) {
	ElfRelation_ptr rel = inst->relation();
	if (rel->get_name() == L"eru:IsAffiliateOf") {
		rel->set_name(L"eru:HasEmployer");
		BOOST_FOREACH(ElfRelationArg_ptr arg, rel->get_args()) {
			if (arg->get_role() == L"eru:subjectOfIsAffiliateOf") {
				arg->set_role(L"eru:subjectOfHasEmployer");
			} else if (arg->get_role() == L"eru:objectOfIsAffiliateOf") {
				arg->set_role(L"eru:objectOfHasEmployer");
			}
		}
		return true;
	}
	return false;
}

void TemporalAttributeAdder::undoAffiliatedHack(TemporalInstance_ptr inst) {
	ElfRelation_ptr rel = inst->relation();
	if (rel->get_name() == L"eru:HasEmployer") {
		rel->set_name(L"eru:IsAffiliateOf");
		BOOST_FOREACH(ElfRelationArg_ptr arg, rel->get_args()) {
			if (arg->get_role() == L"eru:subjectOfHasEmployer") {
				arg->set_role(L"eru:subjectOfIsAffiliateOf");
			} else if (arg->get_role() == L"eru:objectOfHasEmployer") {
				arg->set_role(L"eru:objectOfIsAffiliateOf");
			}
		}
	} else {
		throw UnexpectedInputException("TemporalAttributeAdder::undoAffiliatedHack",
				"The relation doesn't have the role eru:HasEmployer, so it must not be"
				" a result of the affiliated hack");
	}
}
