#ifndef _PROBLEM_DEFINITION_H_
#define _PROBLEM_DEFINITION_H_

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/BoostUtil.h"
#include "GraphicalModels/pr/Constraint.h"

struct ConstraintDescription {
	ConstraintDescription(double weight, const std::wstring& constraintType, 
			const std::wstring& constraintData)  : weight(weight),
		constraintType(constraintType), constraintData(constraintData) {}

	double weight;
	std::wstring constraintType;
	std::wstring constraintData;
};

struct InstanceConstraintDescription {
	InstanceConstraintDescription(const std::wstring& constraintType,
			const std::wstring& name, const std::wstring& constraintData)
		: constraintType(constraintType), constraintData(constraintData),
			name(name) {}
	std::wstring constraintType;
	std::wstring constraintData;
	std::wstring name;
};


class ACEEvent;

BSP_DECLARE(ProblemDefinition)
class ProblemDefinition {
	public:
		static ProblemDefinition_ptr load(const std::string& filename);
		const std::vector<std::wstring>& classNames() const {
			return _classNames;
		}

		const std::vector<ConstraintDescription>& constraintDescriptions() const {
			return _constraintDescriptions;
		}

		const std::vector<InstanceConstraintDescription>&
			instanceConstraintDescriptions() const
		{
			return _instanceConstraintDescriptions;
		}


		unsigned int nClasses() const { return _classNames.size(); }
		unsigned int classNumber(const std::wstring& className) const;
		const std::wstring& className(unsigned int idx) const;

		const std::vector<double>& typePriors() const { return _typePriors;}
		const std::wstring& eventType() const {return _eventType; }

		unsigned int maxPassageSize() const { return _max_passage_size; }

		const GraphicalModel::InstanceConstraintsType<ACEEvent>::ptr instanceConstraints() const {
			return _instanceConstraints;
		}

		void setInstanceConstraints(
				const GraphicalModel::InstanceConstraintsType<ACEEvent>::ptr& instanceConstraints) 
		{
			_instanceConstraints = instanceConstraints;
		}

		const GraphicalModel::ConstraintsType<ACEEvent>::type& corpusConstraints() const {
			return *_corpusConstraints;
		}

		void setCorpusConstraints(
				const GraphicalModel::ConstraintsType<ACEEvent>::ptr& corpusConstraints) {
			_corpusConstraints = corpusConstraints;
		}

		typedef std::pair<unsigned int, std::wstring> DecoderGuide;
		const std::vector<DecoderGuide>& decoderGuides() const {
			return _decoderStrings;
		}

	private:
		std::vector<std::wstring> _classNames;
		std::vector<ConstraintDescription> _constraintDescriptions;
		std::vector<InstanceConstraintDescription> _instanceConstraintDescriptions;
		std::vector<double> _typePriors;
		std::wstring _eventType;
		unsigned int _max_passage_size;
		std::vector<DecoderGuide> _decoderStrings;

		ProblemDefinition(const std::wstring& eventType,
				unsigned int max_passage_size,
				const std::vector<std::wstring>& classNames,
				const std::vector<double>& typePriors,
				const std::vector<ConstraintDescription>& constraintDescriptions,
				const std::vector<InstanceConstraintDescription>& instanceConstraintDescription)
			: _classNames(classNames),  _typePriors(typePriors), _max_passage_size(max_passage_size),
			_constraintDescriptions(constraintDescriptions), _eventType(eventType),
			_instanceConstraintDescriptions(instanceConstraintDescription) {}
		BOOST_MAKE_SHARED_6ARG_CONSTRUCTOR(ProblemDefinition,
				const std::wstring&,
				unsigned int,
				const std::vector<std::wstring>&,
				const std::vector<double>&,
				const std::vector<ConstraintDescription>&,
				const std::vector<InstanceConstraintDescription>&);

	GraphicalModel::InstanceConstraintsType<ACEEvent>::ptr _instanceConstraints;
	GraphicalModel::ConstraintsType<ACEEvent>::ptr _corpusConstraints;

};

#endif

