#include "Generic/common/leak_detection.h"
#include "Temporal/features/InstanceSourceFeature.h"

#include <sstream>
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "PredFinder/elf/ElfRelation.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"

// Studio is over-paranoid about bounds checking for the boost string 
// classification module, so we wrap its import statement with pragmas
// that tell studio to not display warnings about it.  For more info:
// <http://msdn.microsoft.com/en-us/library/ttcz0bys.aspx>
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4996)
# include "boost/algorithm/string/classification.hpp"
# pragma warning(pop)
#else
# include "boost/algorithm/string/classification.hpp"
#endif

using std::wstring;
using std::wstringstream;
using std::vector;
using boost::dynamic_pointer_cast;
using boost::make_shared;
using boost::algorithm::split;
using boost::is_any_of;

InstanceSourceFeature::InstanceSourceFeature(const std::wstring& instanceSource,
		const Symbol& relation)
	: TemporalFeature(L"InstanceSource", relation), _source(instanceSource)
{ }

InstanceSourceFeature_ptr InstanceSourceFeature::create(
		const std::vector<std::wstring>& parts, const Symbol& relation) 
{
	if (parts.empty()) {
		throw UnexpectedInputException("InstanceSourceFeature::create",
				"No metadata provided.");
	}

	if (parts.size() != 1) {
		throw UnexpectedInputException("InstanceSourceFeature::create",
				"Expected exactly one field of metadata");
	}

	return make_shared<InstanceSourceFeature>(parts[0], relation);
}

std::wstring InstanceSourceFeature::pretty() const {
	wstringstream str;
	str << L"InstanceSource(" << relation().to_string() << L", " << _source << L")";

	return str.str();
}

std::wstring InstanceSourceFeature::dump() const {
	return pretty();
}

std::wstring InstanceSourceFeature::metadata() const {
	wstringstream str;

	TemporalFeature::metadata(str);
	str << L"\t" << _source;
	return str.str();
}

bool InstanceSourceFeature::equals(const TemporalFeature_ptr& other) const {
	if (!topEquals(other)) {
		return false;
	}

	if (InstanceSourceFeature_ptr rsf =
			dynamic_pointer_cast<InstanceSourceFeature>(other)) {
		return _source == rsf->_source;
	}
	return false;
}

size_t InstanceSourceFeature::calcHash() const {
	size_t ret = topHash();
	boost::hash_combine(ret, _source);	
	return ret;
}

bool InstanceSourceFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	if (!relationNameMatches(inst)) {
		return false;
	}

	return inst->proposalSource() == _source;
}

void InstanceSourceFeature::addFeaturesToInstance(const TemporalInstance& inst,
		std::vector<TemporalFeature_ptr>& fv)
{
	if (!inst.proposalSource().empty()) {
		fv.push_back(make_shared<InstanceSourceFeature>(
			inst.proposalSource(), inst.relation()->get_name()));
	}
}

