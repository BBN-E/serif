#include "Generic/common/leak_detection.h"
#include "TemporalInstanceSerialization.h"

#include <boost/make_shared.hpp>
#include "Generic/common/UnrecoverableException.h"
#include "TemporalInstance.h"
#include "TemporalAttribute.h"
#include "TemporalTypes.h"

using boost::make_shared;

TemporalInstanceData::TemporalInstanceData() : _dummy(true) { }

TemporalInstanceData::TemporalInstanceData(unsigned int t, size_t hv,
										   const TemporalFV& vec)
										   : _fv(vec), _hashValue(hv), _type(t),
										   _dummy(false)
{}

TemporalInstanceData_ptr TemporalInstanceData::create(const TemporalInstance& inst, 
	const TemporalFV& vec)
{
	return make_shared<TemporalInstanceData>(inst.attribute()->type(), 
		inst.fingerprint(), vec);
}

size_t TemporalInstanceData::hashValue() const {
	if (!_dummy) {
		return _hashValue;
	} else {
		throw UnrecoverableException("TemporalInstanceData::hashValue()",
			"Cannot call methods of default-constructed dummy object");
	}
}

const TemporalFV& TemporalInstanceData::fv() const {
	if (!_dummy) {
		return _fv;
	} else {
		throw UnrecoverableException("TemporalInstanceData::hashValue()",
			"Cannot call methods of default-constructed dummy object");
	}
}

unsigned int TemporalInstanceData::type() const {
	if (!_dummy) {
		return _type;
	} else {
		throw UnrecoverableException("TemporalInstanceData::hashValue()",
			"Cannot call methods of default-constructed dummy object");
	}
}

TemporalFV& TemporalInstanceData::fv() {
	if (!_dummy) {
		return _fv;
	} else {
		throw UnrecoverableException("TemporalInstanceData::hashValue()",
			"Cannot call methods of default-constructed dummy object");
	}
}
