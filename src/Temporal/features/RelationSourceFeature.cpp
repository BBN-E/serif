/*#include "Generic/common/leak_detection.h"
#include "Temporal/features/RelationSourceFeature.h"

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

RelationSourceFeature::RelationSourceFeature(const std::wstring& sourceComponent,
		const Symbol& relation)
	: TemporalFeature(L"Source", relation), _sourceComponent(sourceComponent)
{ }

RelationSourceFeature_ptr RelationSourceFeature::create(
		const std::vector<std::wstring>& parts, const Symbol& relation) 
{
	if (parts.empty()) {
		throw UnexpectedInputException("RelationSourceFeature::create",
				"No metadata provided.");
	}

	if (parts.size() != 1) {
		throw UnexpectedInputException("RelationSourceFeature::create",
				"Expected exactly one field of metadata");
	}

	return make_shared<RelationSourceFeature>(parts[0], relation);
}

std::wstring RelationSourceFeature::pretty() const {
	wstringstream str;
	str << L"Source(" << relation().to_string() << L", " << _sourceComponent << L")";

	return str.str();
}

std::wstring RelationSourceFeature::dump() const {
	return pretty();
}

std::wstring RelationSourceFeature::metadata() const {
	wstringstream str;

	TemporalFeature::metadata(str);
	str << L"\t" << _sourceComponent;
	return str.str();
}

bool RelationSourceFeature::equals(const TemporalFeature_ptr& other) const {
	if (!topEquals(other)) {
		return false;
	}

	if (RelationSourceFeature_ptr rsf =
			dynamic_pointer_cast<RelationSourceFeature>(other)) {
		return _sourceComponent == rsf->_sourceComponent;
	}
	return false;
}

size_t RelationSourceFeature::calcHash() const {
	size_t ret = topHash();
	boost::hash_combine(ret, _sourceComponent);	
	return ret;
}

bool RelationSourceFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	if (!relationNameMatches(inst)) {
		return false;
	}

	if (inst->provenance) {
		BOOST_FOREACH(const wstring& provComp, *inst->provenance) {
			if (provComp == _sourceComponent) {
				return true;
			}
		}
	}

	return false;
}

void RelationSourceFeature::addFeaturesToInstance(const TemporalInstance& inst,
		std::vector<TemporalFeature_ptr>& fv)
{
	if (inst.provenance) {
		BOOST_FOREACH(std::wstring& part, *inst.provenance) {
			fv.push_back(make_shared<RelationSourceFeature>(
							part, inst.relation()->get_name()));
		}
	}
}
*/
