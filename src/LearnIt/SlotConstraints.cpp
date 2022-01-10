#include "Generic/common/leak_detection.h"
#include "SlotConstraints.h"
#include <cassert>
#include <boost/make_shared.hpp>
#include "Generic/common/Sexp.h"
#include "Generic/patterns/PatternTypes.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/ValueMentionPattern.h"

using boost::make_shared;

SlotConstraints::SlotType SlotConstraints::stringToType(const std::wstring& ws){
	if(ws==L"mention") return MENTION;
	if(ws==L"value") return VALUE;
	throw std::invalid_argument("Bad SlotType name");
}

std::wstring SlotConstraints::getMentionTypeAsString() const {
	assert ((_slot_type==MENTION) || (_slot_type==VALUE));
	return (_slot_type==MENTION)?L"mention":L"value";
}

std::wstring SlotConstraints::getMentionTypeAsLabelTypeString() const {
	assert ((_slot_type==MENTION) || (_slot_type==VALUE));
	return (_slot_type==MENTION)?L"mentionlabel":L"valuementionlabel";
}

bool SlotConstraints::operator==(const SlotConstraints& rhs) const {
	return ((_slot_type == rhs._slot_type) &&
			(_brandy_constraints == rhs._brandy_constraints) &&
			(_seed_type == rhs._seed_type));
}

SlotConstraints::SlotConstraints(SlotType slot_type, 
	const std::wstring& brandy_constraints, const std::wstring& seed_type, 
	const std::wstring &elf_ontology_type, const std::wstring &elf_role, 
	bool use_best_name, bool allow_desc_training):
	_slot_type(slot_type), _brandy_constraints(brandy_constraints), 
	_seed_type(seed_type), _elf_ontology_type(elf_ontology_type), 
	_elf_role(elf_role), _use_best_name(use_best_name),
	_allow_desc_training(allow_desc_training)
{
	initBrandyPattern();
}

SlotConstraints::SlotConstraints(const std::wstring& slot_type, 
	const std::wstring& brandy_constraints, const std::wstring& seed_type, 
	const std::wstring &elf_ontology_type, const std::wstring &elf_role, 
	bool use_best_name, bool allow_desc_training):
	_slot_type(stringToType(slot_type)), 
	_brandy_constraints(brandy_constraints), _seed_type(seed_type), 
	_elf_ontology_type(elf_ontology_type), _elf_role(elf_role), 
	_use_best_name(use_best_name), _allow_desc_training(allow_desc_training)
{
	initBrandyPattern();
}


void SlotConstraints::initBrandyPattern() {
	assert ((_slot_type==MENTION) || (_slot_type==VALUE));
	std::wstring pattern = L"("+getMentionTypeAsString()+L" "+_brandy_constraints+L")";
        std::wistringstream pat_str(pattern, std::wistringstream::in);
	Sexp sexp(pat_str, true);
	if (_slot_type==MENTION) {
		_brandyPattern = make_shared<MentionPattern>(&sexp, Symbol::HashSet(1), PatternWordSetMap());
	} else {
		_brandyPattern = make_shared<ValueMentionPattern>(&sexp, Symbol::HashSet(1), PatternWordSetMap());
	}
}
