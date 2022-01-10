#ifndef _TEMPORAL_ATTRIBUTE_H_
#define _TEMPORAL_ATTRIBUTE_H_

#include <vector>
#include <string>
#include "Generic/common/bsp_declare.h"
#include "Generic/common/Symbol.h"

BSP_DECLARE(TemporalAttribute)
class ValueMention;

class TemporalAttribute {
public:
	TemporalAttribute(unsigned int type, const ValueMention* vm);
	TemporalAttribute(const TemporalAttribute& other, double prob);
	const ValueMention* valueMention() const;
	unsigned int type() const;
	double probability() const;
	size_t fingerprint() const;
private:
	unsigned int _type;
	const ValueMention* _vm;
	double _probability;
	size_t _fingerprint;

	size_t calcFingerprint() const;
};

typedef std::vector<TemporalAttribute_ptr> TemporalAttributes;

#endif
