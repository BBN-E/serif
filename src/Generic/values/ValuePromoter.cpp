// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/values/ValuePromoter.h"
#include "Generic/theories/Value.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/ValueSet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueMention.h"

ValuePromoter::ValuePromoter() {
	_debugStream.init(Symbol(L"valuepromoter_debug"));
}

void ValuePromoter::promoteValues(DocTheory* docTheory) {

	if (docTheory->getNSentences() == 0) {
		ValueSet *valSet = _new ValueSet(0);
		docTheory->takeValueSet(valSet);
		return;
	}
	
	_n_values = 0;

	for (int sentno = 0; sentno < docTheory->getNSentences(); sentno++) {
		const ValueMentionSet *valMentionSet 
			= docTheory->getSentenceTheory(sentno)->getValueMentionSet();
		for (int i = 0; i < valMentionSet->getNValueMentions(); i++) {
			ValueMention *valMention = valMentionSet->getValueMention(i);
			//_debugStream << valMention->toString() << L"\n";
		
			if (_n_values < MAX_DOCUMENT_VALUES) {
				Value *val = _new Value(valMention, _n_values);
				_valueList[_n_values] = val;
				_n_values++;
			}
			else {
				SessionLogger::warn("max_document_values") << "Number of values exceeds MAX_DOCUMENT_VALUES. Ignoring detected value.";
			}
		}
	}

	ValueMentionSet *set = docTheory->getDocumentValueMentionSet();

	if (set) {
		_debugStream << "Starting Document level values\n";
		for (int i = 0; i < set->getNValueMentions(); i++) {
			ValueMention *valMention = set->getValueMention(i);
			//_debugStream << valMention->toString() << L"\n";
			
			if (_n_values < MAX_DOCUMENT_VALUES) {
				Value *val = _new Value(valMention, _n_values);
				_valueList[_n_values] = val;
				_n_values++;
			}
			else {
				SessionLogger::warn("max_document_values") << "Number of values exceeds MAX_DOCUMENT_VALUES. Ignoring detected value.";
			}
		}
	}

	ValueSet *valSet = _new ValueSet(_n_values);
	for (int j = 0; j < _n_values; j++) {
		valSet->takeValue(j, _valueList[j]);
		_valueList[j] = 0;
	}
	docTheory->takeValueSet(valSet);
}
