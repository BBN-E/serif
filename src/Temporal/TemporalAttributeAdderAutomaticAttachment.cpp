#include "Generic/common/leak_detection.h"
#include "TemporalAttributeAdderAutomaticAttachment.h"

#include <boost/foreach.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/common/SessionLogger.h"
#include "Temporal/TemporalAttribute.h"
#include "Temporal/ManualTemporalInstanceGenerator.h"
#include "Temporal/TemporalDB.h"
#include "Temporal/features/TemporalFeature.h"
#include "PredFinder/elf/ElfRelation.h"

TemporalAttributeAdderAutomatic::TemporalAttributeAdderAutomatic(TemporalDB_ptr db) :
    TemporalAttributeAdder(db),
_threshold(ParamReader::getRequiredFloatParam("temporal_threshold"))
{
}

void TemporalAttributeAdderAutomatic::addTemporalAttributes(ElfRelation_ptr relation,
				const DocTheory* dt, int sn)
{
	TemporalInstances instances;

	_instanceGenerator->instances(dt->getDocument()->getName(), 
		dt->getSentenceTheory(sn), relation, instances);

	BOOST_FOREACH(const TemporalInstance_ptr inst, instances) {
		double prob = scoreInstance(inst, dt, sn);
		if (prob > _threshold) {
			std::wstringstream src;
			src << L"AutoTempAdd(" << inst->attribute()->type() << L"=" 
				<< std::setprecision(3) << prob << L")";
			addAttributeToRelation(relation, inst->attribute(), src.str(), dt,
					inst->proposalSource());
		}
	}
}

