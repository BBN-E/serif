#include "Generic/common/leak_detection.h"
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/common/foreach_pair.hpp"
#include "BaselineDecoder.h"
#include "Generic/common/SessionLogger.h"
#include "GraphicalModels/pr/FeatureImpliesClassConstraint.h"
#include "ACEEvent.h"
#include "ProblemDefinition.h"
#include "ACEEventGoldStandard.h"
#include "AnswerMatrix.h"

const int BaselineDecoder::NOT_AGGRESSIVE = 0;
const int BaselineDecoder::H_ONLY = 1;
const int BaselineDecoder::H_AND_M_ONLY = 2;

BaselineDecoder::BaselineDecoder(const ProblemDefinition_ptr& problem,
		bool mention_wise, int aggressive, bool multi) 
: ACEDecoder(), _aggressive(aggressive), _problem(problem), 
	_multi(multi), _mention_wise(mention_wise)
{

	SessionLogger::warn("baseline_no_instance_constraints")
		<< L"Recall baseline decoder does not take account of "
		<< L"instance constraints.";

	BOOST_FOREACH(const ConstraintDescription& constraint, _problem->constraintDescriptions()) {
		// weight
		// constraintType
		// constraintData
		if (constraint.constraintType == L"HeadWordImpliesClass") {
			addFICCConstraint<ACEHeadWordFactor>(constraint, *_problem, 
					_headwordConstraints);
		} else if (constraint.constraintType == L"PropDepImpliesClass") {
			addFICCConstraint<ACEPropDependencyFactor>(constraint, *_problem, 
					_propConstraints);
		} else if (constraint.constraintType == L"LegalEntities") {
			const boost::wregex part_re(L"(\\w+)\\[([A-Z_,]+)\\]");
			boost::match_results<wstring::const_iterator> what;
			if (regex_match(constraint.constraintData, what, part_re)) {
				wstring className(what[1].first, what[1].second);
				wstring entityTypesStr(what[2].first, what[2].second);

				std::vector<std::wstring> entityTypes;
				boost::split(entityTypes, entityTypesStr, boost::is_any_of(L","));
				std::vector<unsigned int> encodedEntityTypes;
				BOOST_FOREACH(const std::wstring& str, entityTypes) {
					try {
						encodedEntityTypes.push_back(
								ACEEntityTypeFactor::featureIndex(str));
					} catch (GraphicalModel::BadLookupException&) {
					}
				}
				_legalEntities.insert(std::make_pair(
						problem->classNumber(className), encodedEntityTypes));
			} else {
				std::wstringstream err;
				err << L"Baseline decoder could not parse entity type line "
					<< constraint.constraintData;
				throw UnexpectedInputException("BaselineDecoder::BaselineDecoder",
						err);
			}
		} else {
			SessionLogger::warn("unknown_baseline_constraint") 
				<< L"Baseline decoder doesn't know how to handle constraints of "
				<< L"type " << constraint.constraintType << L"; ignoring.";
		}
	}
}

void BaselineDecoder::applyToEntity(unsigned int entityIdx,
		const ACEEvent& event,
		const std::vector<unsigned int>& features,
		const ConstraintMap& constraints, std::vector<unsigned int>& roleCounts,
		AnswerMatrix& ret) const
{
	BOOST_FOREACH(unsigned int feature_idx, features) {
		BOOST_FOREACH_PAIR(unsigned int role, 
				const std::vector<unsigned int>& constrainedFeatures,
				constraints)
		{
			if (std::find(constrainedFeatures.begin(), constrainedFeatures.end(), 
						feature_idx) != constrainedFeatures.end())
			{
				if (legalForRole(event, entityIdx, role)) {
					ret.forEntity(entityIdx)[role] = 1.0;
					++roleCounts[role];
				}
			}
		}
	}
}

bool BaselineDecoder::legalForRole(const ACEEvent& event, unsigned int entityIdx,
		unsigned int role)  const
{
	LegalEntities::const_iterator probe = _legalEntities.find(role);

	if (probe != _legalEntities.end()) {
		if (std::find(probe->second.begin(), probe->second.end(), 
					event.factors(entityIdx).entityTypeFactor().feature())
				!=probe->second.end()) 
		{
			return true;
		}
	}

	return false;
}

std::string BaselineDecoder::name() const {
	std::stringstream ret;

	ret << "Base(";

	switch (_aggressive) {
		case NOT_AGGRESSIVE:
			ret << "cons";
			break;
		case H_ONLY:
			ret << "agg";
			break;
		case H_AND_M_ONLY:
			ret << "v-agg";
			break;
	}

	if (_multi) {
		ret << " multi";
	}

	if (_mention_wise) {
		ret << " m-ord";
	} else {
		ret << " e-ord";
	}

	ret << ")";

	return ret.str();
}


void BaselineDecoder::findForRole(const ACEEvent& event, unsigned int role, 
		std::vector<unsigned int>& roleCounts, AnswerMatrix& ret) const
{
	// if not, take the first legal mention
	for (size_t i=0; i<ret.size(); ++i) {
		size_t entityIdx = _mention_wise?event.orderByMention[i]:i;
		if (!ret.assignedARole(entityIdx)) {
			if (legalForRole(event, entityIdx, role)) { 
				ret.forEntity(entityIdx)[role] = 1.0;
				++roleCounts[role];
				return;
			}
		}
	}

	// out of luck - there's no legal unused entity
}

AnswerMatrix BaselineDecoder::decode(const GoldStandardACEInstance& gsInst) const {
	const ACEEvent& event = *gsInst.serifEvent;
	AnswerMatrix ret(event.nEntities(), _problem->nClasses());
	std::vector<unsigned int> roleCounts(_problem->nClasses(), 0);

	for (size_t i=0; i<event.nEntities(); ++i) {
		applyToEntity(i, event, event.factors(i).headWordFactor().features(),
				_headwordConstraints, roleCounts, ret);
		applyToEntity(i, event, event.factors(i).propDependencyFactor().features(),
				_propConstraints, roleCounts, ret);
	}

	BOOST_FOREACH(const ProblemDefinition::DecoderGuide& decoderGuide,
			_problem->decoderGuides())
	{
		unsigned int role = decoderGuide.first;
		const std::wstring& decoderString = decoderGuide.second;

		if (!decoderString.empty()) {
			for (size_t i =0; i<(_multi?decoderString.size():1); ++i) {
				if (i>=roleCounts[role]) {
					switch (decoderString[i]) {
						case L'H':
							if (_aggressive >= H_ONLY) {
								findForRole(event, role, roleCounts, ret);
							}
							break;
						case L'M':
							if (_aggressive >= H_AND_M_ONLY) {
								findForRole(event, role, roleCounts, ret);
							}
							break;
						case L'L':
						case L'R':
						default:
							;// do nothing
					}
				}
			}
		}
	}
	ret.normalize();

	return ret;
}

