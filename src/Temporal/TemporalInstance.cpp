#include "Generic/common/leak_detection.h"
#include "TemporalInstance.h"

#include <vector>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include <boost/functional/hash.hpp>
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SentenceTheory.h"
#include "PredFinder/elf/ElfRelation.h"
#include "TemporalAttribute.h"

using std::wstring;
using std::vector;
using boost::make_shared;
using boost::hash_combine;

TemporalInstance::TemporalInstance(const Symbol& docid, unsigned int sent,
	ElfRelation_ptr relation, TemporalAttribute_ptr temporalAttribute,
	const wstring& previewString, const wstring& proposalSource)
	: _relation(relation), _temporalAttribute(temporalAttribute), _docid(docid),
	_sent(sent), _previewString(previewString), _proposalSource(proposalSource)
{}

const ElfRelation_ptr TemporalInstance::relation() const {
	return _relation;
}

const TemporalAttribute_ptr TemporalInstance::attribute() const {
	return _temporalAttribute;
}

const Symbol& TemporalInstance::docid() const {
	return _docid;
}

unsigned int TemporalInstance::sentence() const {
	return _sent;
}

size_t TemporalInstance::fingerprint() const {
	size_t hsh = 0;

	hash_combine(hsh, _docid.to_string());
	hash_combine(hsh, _sent);
	hash_combine(hsh, attribute()->fingerprint());
	hash_combine(hsh, relation()->get_name());
	
	vector<ElfRelationArg_ptr> args = relation()->get_args();

	BOOST_FOREACH(const ElfRelationArg_ptr& arg, args) {
		hash_combine(hsh, arg->get_role());
		hash_combine(hsh, arg->get_text());
	}

	return hsh;
}

const wstring& TemporalInstance::previewString() const {
	return _previewString;
}

const wstring& TemporalInstance::proposalSource() const {
	return _proposalSource;
}

const Mention* TemporalInstance::mentionForRole(const wstring& role, const MentionSet* ms) {
	BOOST_FOREACH(ElfRelationArg_ptr arg, relation()->get_args()) {
		if (arg->get_role() == role) {
			ElfIndividual_ptr indiv = arg->get_individual();
			if (indiv && indiv->has_mention_uid()) {
				return ms->getMention(indiv->get_mention_uid());
			}
		}
	}

	return 0;
}
