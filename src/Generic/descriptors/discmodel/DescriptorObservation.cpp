// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/discTagger/DTAltModelSet.h"
#include "Generic/wordnet/xx_WordNet.h"

const Symbol DescriptorObservation::_className(L"descriptor");
DTAltModelSet DescriptorObservation::_altModels;

DescriptorObservation::DescriptorObservation(bool use_wordnet): 
		  DTObservation(_className), _use_wordnet(use_wordnet),
		  _headwordWC(WordClusterClass::nullCluster()), _n_offsets(0), _is_person_hyponym(false)
{
	_propSet = 0;
	_wordFeatures = IdFWordFeatures::build();

	_n_alt_predictions = 0;
	if (ParamReader::isParamTrue("p1_desc_use_alt_models"))
		_altModels.initialize(P1DescFeatureType::modeltype, "alternative_p1_desc_model_file");
}

DescriptorObservation::~DescriptorObservation() {
	delete _wordFeatures;
}

DTObservation *DescriptorObservation::makeCopy() {
	DescriptorObservation *copy = _new DescriptorObservation(_use_wordnet);

	copy->populate(_mentionSet, _node, _propSet);
	return copy;
}

void DescriptorObservation::populate(const MentionSet *mentionSet,
									 const SynNode* node,
									 const PropositionSet *propSet)
{
	_mentionSet = mentionSet;
	_node = node;
	//fill the tokens array by getting the root of the tree and getting the terminal symbols.
	const SynNode* root = node;
	while (root->getParent() != 0) {
		root = root->getParent();
	}
	_n_words = root->getTerminalSymbols(_sentence_words, MAX_SENTENCE_TOKENS);
	_idfWordFeature = _wordFeatures->features(node->getHeadWord(), 
		(node->getHeadPreterm()->getStartToken() == 0), true);
	if (_use_wordnet) {
		if (SymbolUtilities::isClusterableWord(node->getHeadWord())) {
			_headwordWC = WordClusterClass(
				SymbolUtilities::stemDescriptor(node->getHeadWord()), true);
		}
		else {
			_headwordWC = WordClusterClass::nullCluster();
		}
		_n_offsets = SymbolUtilities::fillWordNetOffsets(
			node->getHeadWord(), _wordnetOffsets, MAX_WN_OFFSETS);
		_is_person_hyponym = WordNet::getInstance()->isPerson(node->getHeadWord(), true);
	}
	else  {
		if (SymbolUtilities::isClusterableWord(node->getHeadWord())) {
			_headwordWC = WordClusterClass(node->getHeadWord(), true);
		}
		else {
			_headwordWC = WordClusterClass::nullCluster();
		}
		_n_offsets = 0;
		_is_person_hyponym = false;
	}

	addPropositionSet(propSet);

	_prev_word_prep = checkPrevWordPrep(node);

	if (_altModels.getNAltDecoders() > 0) 
		_n_alt_predictions = _altModels.addDecoderPredictionsToObservation(this);
}

bool DescriptorObservation::checkPrevWordPrep(const SynNode* node) {
	static const Symbol::SymbolGroup PREPS = Symbol::makeSymbolGroup(
		L"from until after near to under than over before since toward "
		L"among without upon through in during on despite against by for "
		L"outside with within into as at like around of between about");
	if (node->getStartToken()>0) {
		Symbol prevWord = _sentence_words[node->getStartToken()-1];
		return (PREPS.find(prevWord) != PREPS.end());
	} else {
		return false;
	}
}

void DescriptorObservation::addPropositionSet(const PropositionSet *propositionSet) {
	
	_propSet = propositionSet;
	_valid_props.clear();

	if (propositionSet == 0)
		return;
	
	const Mention* ment = _mentionSet->getMentionByNode(_node);
	int nprop = _propSet->getNPropositions();
	for (int i = 0; i < nprop; i++) {
		Proposition* prop = _propSet->getProposition(i);
		for (int j = 0; j < prop->getNArgs(); j++) {
			if (prop->getArg(j) == 0) {
				continue;
			}
			if (prop->getArg(j)->getType() == Argument::MENTION_ARG) {
				if (prop->getArg(j)->getMention(_mentionSet)->getNode() == _node) {
					_valid_props.push_back(i);
				}
			}
		}
	}			
}

void DescriptorObservation::setAltDecoderPrediction(int i, Symbol prediction) {
	_altDecoderPredictions[i] = prediction;
}

Symbol DescriptorObservation::getNthAltDecoderName(int i) {
	return _altModels.getDecoderName(i);
}

Symbol DescriptorObservation::getNthAltDecoderPrediction(int i) {
	return _altDecoderPredictions[i];
}

int DescriptorObservation::getNAltDecoderPredictions() {
	return _n_alt_predictions;
}
