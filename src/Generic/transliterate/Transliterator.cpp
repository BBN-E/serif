// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/transliterate/Transliterator.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/TokenSequence.h"

// Convenience methods
std::wstring Transliterator::getTransliteration(const Mention *mention) {
	const TokenSequence* tokenSequence = mention->getMentionSet()->getParse()->getTokenSequence();
	int start = mention->getNode()->getStartToken();
	int end = mention->getNode()->getEndToken();

	return getTransliteration(tokenSequence, start, end);
}

// Factory Support
Transliterator* Transliterator::Factory::build() {
	throw UnexpectedInputException("Transliterator::build",
		 "Transliteration requires the \"Transliterator\" feature module.");
}

boost::shared_ptr<Transliterator::Factory> &Transliterator::_factory() {
	static boost::shared_ptr<Factory> factory(new Factory());
	return factory;
}

void Transliterator::setFactory(boost::shared_ptr<Factory> factory) {
	_factory() = factory; 
}

Transliterator *Transliterator::build() { return _factory()->build(); }
