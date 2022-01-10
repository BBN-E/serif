#include "Generic/common/leak_detection.h"
#include "TemporalAttributeAdderManualAttachment.h"

#include <map>
#include <boost/foreach.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/common/SessionLogger.h"
#include "Temporal/TemporalAttribute.h"
#include "Temporal/ManualTemporalInstanceGenerator.h"
#include "Temporal/TemporalDB.h"
#include "Temporal/features/TemporalFeature.h"
#include "PredFinder/elf/ElfRelation.h"

TemporalAttributeAdderManual::TemporalAttributeAdderManual(TemporalDB_ptr db):
	TemporalAttributeAdder(db)
{ }

void TemporalAttributeAdderManual::addTemporalAttributes(ElfRelation_ptr relation,
				const DocTheory* dt, int sn)
{
	TemporalInstances instances;

	_instanceGenerator->instances(dt->getDocument()->getName(), 
		dt->getSentenceTheory(sn), relation, instances);

	std::multimap<const ValueMention*, TemporalInstance_ptr> grouped_by_vm;
	typedef std::multimap<const ValueMention*, TemporalInstance_ptr>::const_iterator InstancesByVMIter;
	typedef std::pair<InstancesByVMIter,InstancesByVMIter> InstancesByVMRange;

	BOOST_FOREACH(TemporalInstance_ptr inst, instances) {
		grouped_by_vm.insert(std::make_pair(inst->attribute()->valueMention(), inst));
	}

	InstancesByVMIter vm_it, inst_it;
	for (vm_it = grouped_by_vm.begin(); vm_it != grouped_by_vm.end(); vm_it = inst_it) {
		std::wstringstream src;
		src << L"ManTempAdd(";

		const ValueMention* key = vm_it->first;
		InstancesByVMRange instRange = grouped_by_vm.equal_range(key);

		TemporalInstance_ptr bestInstance = TemporalInstance_ptr();
		double best_prob = 0.0;
		bool first = true;

		if (_debug_matching) {
			SessionLogger::info("temporal_debug") << "Relation is "
				<< relation->toDebugString(0);
			if (vm_it->first) {
				SessionLogger::info("temporal_debug") << "Attribute is " <<
					vm_it->first->toString(dt->getSentenceTheory(sn)->getTokenSequence());
			}
		}

		for (inst_it = instRange.first; inst_it != instRange.second; ++inst_it) {
			double prob = scoreInstance(inst_it->second, dt, sn);
			if (prob > best_prob) {
				bestInstance = inst_it->second;
				best_prob = prob;
			}

			if (!first) {
				src << L";";
			} else {
				first = false;
			}
			src << inst_it->second->attribute()->type() << L"=" <<
				std::setprecision(3) << 100.0* prob;
		}

		src << L")";

		if (bestInstance) {
			addAttributeToRelation(relation, bestInstance->attribute(), src.str(), 
				dt, bestInstance->proposalSource());
		}
	}
}

