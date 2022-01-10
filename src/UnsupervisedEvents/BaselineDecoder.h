#ifndef _BASELINE_DECODER_H_
#define _BASELINE_DECODER_H_

#include <string>
#include <map>
#include "Generic/common/SessionLogger.h"
#include "GraphicalModels/pr/FeatureImpliesClassConstraint.h"
#include "ProblemDefinition.h"
#include "ACEDecoder.h"
#include "ACEEntityFactorGroup.h"

class AnswerMatrix;
BSP_DECLARE(BaselineDecoder)
class BaselineDecoder : public ACEDecoder {
public:
	BaselineDecoder(const ProblemDefinition_ptr& problem, 
			bool mention_wise, int aggressive, bool multi);
	std::string name() const;
	AnswerMatrix decode(const GoldStandardACEInstance& aceInst) const;

	static const int NOT_AGGRESSIVE;
	static const int H_ONLY;
	static const int H_AND_M_ONLY;
private:
	int _aggressive;
	bool _multi;
	bool _mention_wise;
	typedef std::map<unsigned int, std::vector<unsigned int> > ConstraintMap;
	ConstraintMap _headwordConstraints, _propConstraints;
	ProblemDefinition_ptr _problem;

	void applyToEntity(unsigned int entityIdx,
			const ACEEvent& event,
		const std::vector<unsigned int>& features,
		const ConstraintMap& constraints, std::vector<unsigned int>& roleCounts,
		AnswerMatrix& ret) const;

	void findForRole(const ACEEvent& event, unsigned int role, 
		std::vector<unsigned int>& roleCounts, AnswerMatrix& ret) const;

	typedef std::map<unsigned int, std::vector<unsigned int> > LegalEntities;
	LegalEntities _legalEntities;
	bool legalForRole(const ACEEvent& event, unsigned int entityIdx,
		unsigned int role) const;

	template<class FactorType>
	static void addFICCConstraint(const ConstraintDescription& constraint,
			const ProblemDefinition& problem, ConstraintMap& constraintMap)
	{
		GraphicalModel::FICCDescription desc = GraphicalModel::parseFICCDescription(
				constraint.constraintData);

		if (desc.classes.size() != 1) {
			SessionLogger::warn("multiple_classes") << L"Baseline decoder ignoring "
				<< L"constraint with data " << constraint.constraintData
				<< L"because it has multiple classes.";
		} else {
			unsigned int cls = problem.classNumber(desc.classes[0]);
			ConstraintMap::iterator probe = constraintMap.find(cls);

			if (probe == constraintMap.end()) {
				constraintMap.insert(make_pair(cls, std::vector<unsigned int>()));
				probe = constraintMap.find(cls);
			}

			for (std::vector<std::wstring>::const_iterator it = desc.features.begin();
					it != desc.features.end(); ++it)
			{
				try {
					probe->second.push_back(FactorType::featureIndex(*it));
				} catch (GraphicalModel::BadLookupException&) {}
			}
		}
	}
};

#endif

