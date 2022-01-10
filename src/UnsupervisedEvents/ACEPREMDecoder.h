#ifndef _ACE_PREM_DECODER_H_
#define _ACE_PREM_DECODER_H_

#include <map>
#include <string>
#include "Generic/common/bsp_declare.h"
#include "GraphicalModels/DataSet.h"
#include "GraphicalModels/pr/Constraint.h"
#include "ACEDecoder.h"


BSP_DECLARE(ProblemDefinition)
BSP_DECLARE(ACEEvent)


BSP_DECLARE(ACEPREMDecoder)
class ACEPREMDecoder {
public:
	ACEPREMDecoder(const ProblemDefinition_ptr& problem,
			const std::string& trainDocTable,
			const std::string& testDocTable);
	void train();
	ACEEvent_ptr eventByKey(const std::wstring& key) const;
	ProblemDefinition& problem() { return *_problem; }
private:
	typedef GraphicalModel::ConstraintsType<ACEEvent>::type ACEConstraints;
	typedef GraphicalModel::ConstraintsType<ACEEvent>::ptr ACEConstraints_ptr;
	typedef GraphicalModel::InstanceConstraintsType<ACEEvent>::type 
		ACEInstanceConstraints;
	typedef GraphicalModel::InstanceConstraintsType<ACEEvent>::ptr 
		ACEInstanceConstraints_ptr;
	typedef std::map<std::wstring,size_t> KeyToEventMap;

	ProblemDefinition_ptr _problem;
	ACEConstraints_ptr _constraints;
	ACEInstanceConstraints_ptr _instanceConstraints;
	GraphicalModel::DataSet<ACEEvent> _aceEvents;
	KeyToEventMap _keyToEventMap;

	void createCorpusConstraints();
	void createInstanceConstraints();
	void createKeyToEventMap();
};

class PREMAdapter : public ACEDecoder {
	public:
		PREMAdapter(const ACEPREMDecoder_ptr& premDecoder) 
			: _premDecoder(premDecoder) {}
	protected:
		ACEEvent_ptr matchingEvent(const ACEEvent& event) const;

		typedef std::pair<unsigned int, double> IdxScore;
		static IdxScore maxOverSentence(const ACEEvent& event, size_t label);
		static IdxScore maxAvailableOverSentence(const ACEEvent& event, size_t role, 
				const AnswerMatrix& ret);
		const ProblemDefinition& problem() const { return _premDecoder->problem(); }
	protected:
		ACEPREMDecoder_ptr _premDecoder;
};


#endif

