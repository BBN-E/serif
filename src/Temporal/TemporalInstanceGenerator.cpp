#include "Generic/common/leak_detection.h"
#include "TemporalInstanceGenerator.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>
#include "Generic/common/ParamReader.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/patterns/QueryDate.h"
#include "PredFinder/elf/ElfRelation.h"
#include "LearnIt/MainUtilities.h"
#include "TemporalAttribute.h"
#include "TemporalTypeTable.h"

using std::wstring;
using boost::make_shared;

TemporalInstanceGenerator::TemporalInstanceGenerator(TemporalTypeTable_ptr typeTable) 
: _typeTable(typeTable) 
{
	std::vector<Symbol> symVec = ParamReader::getSymbolVectorParam(
			"apply_temporal_module_to");

	BOOST_FOREACH(Symbol s, symVec) {
		SessionLogger::info("apply_temporal") << L"Temporal module will be "
			<< "applied to " << s.to_string();
		_eligibleRelations.insert(s);
	}
}

TemporalInstanceGenerator::~TemporalInstanceGenerator() {}

wstring TemporalInstanceGenerator::makePreviewString(const SentenceTheory* st,
	ElfRelation_ptr relation, unsigned int attributeType, const ValueMention* vm)
{
	std::wstringstream previewString;
	const TokenSequence* toks = st->getTokenSequence();
	std::vector<ElfRelationArg_ptr> args = relation->get_args();

	previewString << L"<i>" << relation->get_name() << L"(";
	bool first = true;
	BOOST_FOREACH(const ElfRelationArg_ptr& arg, relation->get_args()) {
		if (!first) {
			previewString << L", ";
		} else {
			first = false;
		}
		previewString << arg->get_text();
	}
	previewString << L"); " << _typeTable->name(attributeType) << L"("
		<< vm->toCasedTextString(toks) << L")</i><br/>" ;

	for (int i=0; i<toks->getNTokens(); ++i) {
		const Token* tok = toks->getToken(i);
		bool start_vm = (i == vm->getStartToken());
		bool end_vm = (i == vm->getEndToken());
		bool start_arg = false;
		bool end_arg = false;

		BOOST_FOREACH(ElfRelationArg_ptr arg, args) {
			if (arg->get_start() == tok->getStartEDTOffset()) {
				start_arg = true;
			}
			if (arg->get_end() == tok->getEndEDTOffset()) {
				end_arg = true;
			}
		}

		if (start_vm) {
			previewString << "<font style=\"background-color: #FF0000\">";
		}
		if (start_arg) {
			previewString << "<font style=\"background-color: #00FF00\">";
		}
		previewString << MainUtilities::encodeForXML(tok->getSymbol().to_string());

		if (end_vm) {
			previewString << "</font>";
		}
		if (end_arg) {
			previewString << "</font>";
		}
		previewString << L" ";
	}

	wstring ret = previewString.str();
	// ensure there are no newlines
	for (size_t i=0; i< ret.length(); ++i) {
		if (ret[i] == L'\n') {
			ret[i] = L' ';
		}
	}
	return ret;
}


void TemporalInstanceGenerator::instances(const Symbol& docid, 
		const SentenceTheory* st, ElfRelation_ptr relation, 
		TemporalInstances& instances)
{
	if (_eligibleRelations.find(relation->get_name()) != _eligibleRelations.end()) {
		instancesInternal(docid, st, relation, instances);
	}
}

const TemporalTypeTable_ptr TemporalInstanceGenerator::typeTable() const {
	return _typeTable;
}
