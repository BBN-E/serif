#include "Generic/common/leak_detection.h"
#include "AllPossibilitiesTemporalInstanceGenerator.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/Value.h"
#include "Generic/patterns/QueryDate.h"
#include "PredFinder/elf/ElfRelation.h"
#include "TemporalAttribute.h"
#include "TemporalTypeTable.h"

using boost::make_shared;

AllPossibilitiesTemporalInstanceGenerator::AllPossibilitiesTemporalInstanceGenerator(
		TemporalTypeTable_ptr typeTable) : TemporalInstanceGenerator(typeTable) {}

void AllPossibilitiesTemporalInstanceGenerator::instancesInternal(
		const Symbol& docid, const SentenceTheory* st, 
		ElfRelation_ptr relation, TemporalInstances& instances)
{
	const ValueMentionSet* vms = st->getValueMentionSet();
	for (int i = 0; i< vms->getNValueMentions(); ++i) {
		const ValueMention* vm = vms->getValueMention(i);
		if (vm->getSentenceNumber() == st->getSentNumber()) {
			const Value* val = vm->getDocValue();
			// use only specific dates, not things like "weekly"
			if (val && vm->isTimexValue() && QueryDate::isSpecificDate(val)) {
				BOOST_FOREACH(const unsigned int attributeType, typeTable()->ids()) {
					TemporalInstance_ptr inst = 
					make_shared<TemporalInstance>(docid, 
						st->getSentNumber(), relation,
						make_shared<TemporalAttribute>(attributeType, vm),
						makePreviewString(st, relation, attributeType, vm));
					//inst->provenance = prov;
					instances.push_back(inst);
				}
			}
		}
	}
}

