#include "Generic/common/leak_detection.h"
#include "TemporalAttribute.h"

#include "Generic/common/Symbol.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/Value.h"
#include <boost/functional/hash.hpp>

using boost::hash_combine;

TemporalAttribute::TemporalAttribute(unsigned int type, const ValueMention* vm) 
: _type(type), _vm(vm), _probability(0.0), _fingerprint(calcFingerprint()) {}

TemporalAttribute::TemporalAttribute(const TemporalAttribute& other, double prob)
: _type(other.type()), _vm(other.valueMention()), _probability(prob),
_fingerprint(other._fingerprint) {}

unsigned int TemporalAttribute::type() const {
	return _type;
}

const ValueMention* TemporalAttribute::valueMention() const {
	return _vm;
}

double TemporalAttribute::probability() const {
	return _probability;
}

size_t TemporalAttribute::fingerprint() const {
	return _fingerprint;
}

size_t TemporalAttribute::calcFingerprint() const {
	size_t hash = 0;
	
	hash_combine(hash, _type);
	hash_combine(hash, _vm->getDocValue()->getStartToken());
	hash_combine(hash, _vm->getDocValue()->getEndToken());
	return hash;
}
