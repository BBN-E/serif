#include "Generic/common/leak_detection.h"
#include "TemporalBiasFeature.h"
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"

using std::wstring;
using std::wstringstream;
using boost::dynamic_pointer_cast;

TemporalBiasFeature::TemporalBiasFeature(unsigned int attributeType, 
	const Symbol& relation)
: TemporalFeature(L"Bias", relation), _attributeType(attributeType)
{ }

TemporalBiasFeature_ptr TemporalBiasFeature::create(
		const std::vector<std::wstring>& parts, const Symbol& relation)
{
	if (parts.size() != 1) {
		throw UnexpectedInputException("TemporalBiasFeature::create",
			"Cannot parse TemporalBiasFeature");
	}

	unsigned int attributeType = boost::lexical_cast<unsigned int>(parts[0]);

	return boost::make_shared<TemporalBiasFeature>(attributeType, relation);
}

std::wstring TemporalBiasFeature::pretty() const {
	wstringstream str;

	str << L"Bias(" << _attributeType ;
	
	if (!relation().is_null()) {
		str << L", " << relation().to_string();
	}

	str << L")";

	return str.str();
}

std::wstring TemporalBiasFeature::dump() const {
	return pretty();
}

std::wstring TemporalBiasFeature::metadata() const {
	std::wstringstream str;

	TemporalFeature::metadata(str);

	str << L"\t" << _attributeType;

	return str.str();
}

bool TemporalBiasFeature::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (TemporalBiasFeature_ptr tbf = 
				dynamic_pointer_cast<TemporalBiasFeature>(other)) {
			return tbf->_attributeType == _attributeType;
		}
	}
	return false;
}

size_t TemporalBiasFeature::calcHash() const {
	size_t ret = topHash();
	boost::hash_combine(ret, _attributeType);
	return ret;
}
			
bool TemporalBiasFeature::matches(TemporalInstance_ptr inst, const DocTheory* dt, 
		unsigned int sn) const 
{
	if (!relationNameMatches(inst)) {
		return false;
	}

	return inst->attribute()->type() == _attributeType;
}

