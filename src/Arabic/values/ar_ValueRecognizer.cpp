// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Generic/names/discmodel/PIdFSentence.h"
#include "Arabic/values/ar_ValueRecognizer.h"
#include "Generic/common/WordConstants.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/ValueMentionSet.h"

using namespace std;

ArabicValueRecognizer::ArabicValueRecognizer()
	: DO_VALUES(false), _pidfDecoder(0), _docTheory(0),
	_value_finder(RULE_VALUE_FINDER)
{
	
	DO_VALUES = ParamReader::getOptionalTrueFalseParamWithDefaultVal("do_values_stage", false);
	if (!DO_VALUES) return;

	std::string type = ParamReader::getRequiredParam("value_finder_type");
	if (type.compare("pidf") == 0) {
		_value_finder = PIDF_VALUE_FINDER;
	} else if (type.compare("rule") == 0) { 
		_value_finder = RULE_VALUE_FINDER;
	} else {
		throw UnexpectedInputException("ArabicValueRecognizer::ArabicValueRecognizer()",
										"value_finder_type must be set to 'pidf' or 'rule'.");
	}

	if (_value_finder == PIDF_VALUE_FINDER) {
		// Read parameters
		std::string tag_set_file = ParamReader::getRequiredParam("values_tag_set_file");
		std::string features_file = ParamReader::getRequiredParam("values_features_file");
		std::string model_file = ParamReader::getRequiredParam("values_model_file");
		
		_pidfDecoder = _new PIdFModel(PIdFModel::DECODE, tag_set_file.c_str(),
									  features_file.c_str(), model_file.c_str(), 0);
	}
	
}

ArabicValueRecognizer::~ArabicValueRecognizer() {
	delete _pidfDecoder;
}

void ArabicValueRecognizer::resetForNewDocument(DocTheory *docTheory) {
	_docTheory = docTheory;

	if (!DO_VALUES || _value_finder != PIDF_VALUE_FINDER) return;

	_pidfDecoder->resetForNewDocument(docTheory);
}

int ArabicValueRecognizer::getValueTheories(ValueMentionSet **results, int max_theories, 
									  TokenSequence *tokenSequence)
{
	if (!DO_VALUES) {
		results[0] = _new ValueMentionSet(tokenSequence, 0);
		return 1;
	}

	// make the contents of POSTDATE regions automatically into dates
	if (_docTheory != NULL) {
		int sent_no = tokenSequence->getSentenceNumber();
		EDTOffset sent_offset = _docTheory->getSentence(sent_no)->getStartEDTOffset();
		if (_docTheory->getMetadata() != 0 &&
			_docTheory->getMetadata()->getCoveringSpan(sent_offset, POSTDATE_SYM) != 0 &&
			tokenSequence->getNTokens() > 0) {
			try {
				ValueMentionUID uid = ValueMention::makeUID(sent_no, 0);
				// T0 DO: replace TIMEX2 with something that isn't hard coded
				ValueMention *vm = _new ValueMention(sent_no, uid, 0, 
					tokenSequence->getNTokens() - 1, Symbol(L"TIMEX2"));
				results[0] = _new ValueMentionSet(tokenSequence, 1);
				results[0]->takeValueMention(0, vm);
				return 1;
			} catch (InternalInconsistencyException &e) {
				// Should never happen unless max # of value mentions is strangely set to 0
				SessionLogger::err("value_mention_uid") << e;
			}
		}
	}
	
	SpanList valueSpans;
	if (_value_finder == PIDF_VALUE_FINDER) {
		valueSpans = getPIdFValues(tokenSequence);
	} else {
		valueSpans = identifyRuleRepositoryValues(tokenSequence);
	}

	results[0] = createValueMentionSet(tokenSequence, valueSpans);	
	return 1;
}

std::vector<ValueRecognizer::ValueSpan> ArabicValueRecognizer::getPIdFValues(TokenSequence *tokenSequence) {
	
	PIdFSentence sentence(_pidfDecoder->getTagSet(), *tokenSequence);	
	_pidfDecoder->decode(sentence);
	return collectValueSpans(sentence, _pidfDecoder->getTagSet());

}
