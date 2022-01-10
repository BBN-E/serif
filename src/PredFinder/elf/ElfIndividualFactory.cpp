/**
 * Factory class for ElfIndividual and its subclasses.
 *
 * @file ElfIndividualFactory.cpp
 * @author nward@bbn.com
 * @date 2010.06.23
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/patterns/features/TokenSpanPFeature.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "ElfIndividualFactory.h"
#include "ElfDescriptor.h"
#include "ElfRelationArg.h"
#include "boost/make_shared.hpp"
#include "boost/foreach.hpp"
#include <stdexcept>

using boost::dynamic_pointer_cast;
/**
 * Static factory method that builds an ElfIndividual
 * from a SnippetFeatureSet found using manual
 * Distillation patterns. Depending on the contents of
 * feature_set, may produce any of the ElfIndividual
 * subclasses.
 *
 * @param doc_theory The DocTheory containing the matched
 * SnipppetFeatureSet, used for determining offsets.
 * @param feature_set The SnippetFeatureSet that contains
 * a SnippetFeature that we can convert to an ElfIndividual.
 *
 * @author nward@bbn.com
 * @date 2010.06.23
 **/
ElfIndividual_ptr ElfIndividualFactory::from_feature_set(const DocTheory* doc_theory, const PatternFeatureSet_ptr feature_set) {
	// Check for bad entity
	if (doc_theory == NULL)
		throw std::runtime_error("ElfIndividualFactory::from_feature_set(DocTheory*, PatternFeatureSet_ptr): Document theory null");

	// The individual we build from the feature set
	ElfIndividual_ptr individual;

	// Find matched features relevant to the individual
	bool valid_individual_feature = false;
	for (size_t i = 0; i < feature_set->getNFeatures(); i++) {
		// Get this feature from the set
		PatternFeature_ptr feature = feature_set->getFeature(i);

		// Return features determine everything
		if (ReturnPatternFeature_ptr rpf = dynamic_pointer_cast<ReturnPatternFeature>(feature)) {
			// Check if we've already found something for this individual
			if (valid_individual_feature)
				throw std::runtime_error("ElfIndividualFactory::from_feature_set(DocTheory*, PatternFeatureSet_ptr): "
				"Multiple individuals in PatternFeatureSet");

			// Check the return feature type
			if (ValueMentionReturnPFeature_ptr vmf = dynamic_pointer_cast<ValueMentionReturnPFeature>(rpf)) {
				// Construct from the ValueMention
				individual = boost::make_shared<ElfIndividual>(doc_theory, rpf->getReturnValue(L"type"), vmf->getValueMention());
			} else {
				// Construct this individual generically
				individual = boost::make_shared<ElfIndividual>(doc_theory, rpf);
			}

			// Passed all constructors without exception, we found something for this individual
			valid_individual_feature = true;
		}
	}

	// Make sure we had a valid feature set (only one thing that can be turned into an individual)
	if (!valid_individual_feature)
		throw UnexpectedInputException("ElfIndividualFactory::from_feature_set", 
		"Feature set had no individual-generating feature");

	// Done
	return individual;
}

