#include "Generic/common/leak_detection.h"
#include "TemporalRelationFeature.h"
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/EntitySet.h"
#include "Temporal/TemporalInstance.h"
#include "Temporal/TemporalAttribute.h"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"

using std::wstring;
using std::wstringstream;
using boost::dynamic_pointer_cast;
using boost::make_shared;

TemporalRelationFeature::TemporalRelationFeature(const Symbol& relationType, 
		const Symbol& relation)
	: TemporalFeature(L"RelationTime", relation), _relationType(relationType)
{}

TemporalRelationFeature_ptr  TemporalRelationFeature::create(
	const std::vector<std::wstring>& parts, const Symbol& relation)
{
	if (parts.size() != 1) {
		throw UnexpectedInputException("TemporalRelationFeature::create",
				"Cannot parse TemporalRelationFeature");
	}

	Symbol relType = parts[0];

	return boost::make_shared<TemporalRelationFeature>(relType, relation);
}

std::wstring TemporalRelationFeature::pretty() const {
	wstringstream str;

	str << type () << L"(" << _relationType.to_string();

	if (!relation().is_null()) {
		str << L", " << relation().to_string();
	}

	str << L")";

	return str.str();
}

std::wstring TemporalRelationFeature::dump() const {
	return pretty();
}

std::wstring TemporalRelationFeature::metadata() const {
	std::wstringstream str;

	TemporalFeature::metadata(str);

	str << L"\t" << _relationType;

	return str.str();
}

bool TemporalRelationFeature::equals(const TemporalFeature_ptr& other) const {
	if (topEquals(other)) {
		if (TemporalRelationFeature_ptr trf = 
				dynamic_pointer_cast<TemporalRelationFeature>(other)) {
			return trf->_relationType == _relationType;
		}
	}
	return false;
}

size_t TemporalRelationFeature::calcHash() const {
	size_t ret = topHash();
	boost::hash_combine(ret, _relationType.to_string());
	return ret;
}
			
bool TemporalRelationFeature::matches(TemporalInstance_ptr inst, 
		const DocTheory* dt, unsigned int sn) const
{
	if (!relationNameMatches(inst)) {
		return false;
	}

	const RelMentionSet* relations = dt->getSentenceTheory(sn)->getRelMentionSet();
	const EntitySet* es = dt->getEntitySet();

	for (int i=0; i< relations->getNRelMentions(); ++i) {
		const RelMention* rel = relations->getRelMention(i);

		if (rel->getType() == _relationType) {
			if (rel->getTimeArgument() == inst->attribute()->valueMention()
					&& inst->attribute()->valueMention()) 
			{
				return relMatchesRel(rel, inst->relation(), es);
			}
		}
	}

	return false;
}

bool TemporalRelationFeature::relMatchesRel(const RelMention* serifRel,
		ElfRelation_ptr elfRel, const EntitySet* es)
{
	bool matchedFirst = false;
	bool matchedSecond = false;
	
	BOOST_FOREACH(ElfRelationArg_ptr relArg, elfRel->get_args()) {
		if (relArgMatchesMention(relArg, serifRel->getLeftMention(), es)) {
			matchedFirst = true;
		} else if (relArgMatchesMention(relArg, serifRel->getRightMention(), es)) {
			matchedSecond = true;
		}
	}

	return matchedFirst && matchedSecond;
}

bool TemporalRelationFeature::relArgMatchesMention(ElfRelationArg_ptr arg,
		const Mention* ment, const EntitySet* es) 
{
	if (ment) {
		ElfIndividual_ptr indiv = arg->get_individual();

		if (indiv) {
			if (indiv->has_entity_id()) {
				if (es->getEntity(indiv->get_entity_id())->containsMention(ment)) {
					return true;
				}
			} else if (indiv->has_mention_uid()) {
				if (ment->getUID() == indiv->get_mention_uid()) {
					return true;
				}
			}
		}
	}
	return false;
}

