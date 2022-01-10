// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "English/edt/MentionGroups/extractors/en_OperatorExtractor.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"

#include <boost/regex.hpp>

EnglishOperatorExtractor::EnglishOperatorExtractor() :
	AttributeValuePairExtractor<Mention>(Symbol(L"Mention"), Symbol(L"operator"))
{
	validateRequiredParameters();

	_operators.insert(Symbol(L"holder"));
	_operators.insert(Symbol(L"operator"));
	_operators.insert(Symbol(L"owner"));
	_operators.insert(Symbol(L"possessor"));
	_operators.insert(Symbol(L"user"));
	_operators.insert(Symbol(L"utilizer"));
}

std::vector<AttributeValuePair_ptr> EnglishOperatorExtractor::extractFeatures(const Mention& context,
                                                                       LinkInfoCache& cache,
                                                                       const DocTheory *docTheory)
{
	// Copied from EnglishNameRecognizer - probably this should be somewhere central
	static const boost::wregex email_re(
		// Allow one or more .-separated atoms
		L"((?<local_part>)"
		L"[-_+a-zA-Z0-9]+"
		L"(\\.[-_+a-zA-Z0-9]+)*"
		L")"

		// This is non-standard, but we allow a sequence of dots
		// because Craigslist (among others) uses this to obfuscate emails
		L"(\\.+)?"

		// As before, there has to be one and only one at symbol
		L"@"

		// Allow two or more .-separated atoms
		L"((?<domain>)"
		L"[-_+a-zA-Z0-9]+"
		L"(\\.[-_+a-zA-Z0-9]+)+"
		L")"
	);

	// Match an IPv4 quad
	static const boost::wregex ip_re(L"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");

	// Match a phone number
	static const boost::wregex phone_re(
		L"("
		
		// Optional prefix + country code
		L"(\\+?\\d{1,2}[- \\.]?)?"

		// U.S.-style area code + 7 digits
		L"("
		L"(\\(?\\d{3}\\)?[- \\.])?"
		L"\\d{3}[- \\.]\\d{4}"
		L")"

		L"|"

		// International-style digit clusters
		L"("
		L"\\d{2,4}[- \\.]\\d{3,4}[- \\.]\\d{3,4}"
		L")"

		L")"
	);

	// Store matching Proposition arguments that indicate that this operator mention has that address
	std::vector<AttributeValuePair_ptr> results;
	int sentno = context.getSentenceNumber();
	PropositionSet *propSet = docTheory->getSentenceTheory(sentno)->getPropositionSet();
	MentionSet *mentionSet = docTheory->getSentenceTheory(sentno)->getMentionSet();
	for (int p = 0; p < propSet->getNPropositions(); p++) {
		Proposition *prop = propSet->getProposition(p);
		if (prop->getPredType() == Proposition::NOUN_PRED && _operators.find(prop->getPredSymbol()) != _operators.end()) {
			bool match = false;
			Symbol key;
			Symbol value;
			for (int a = 0; a < prop->getNArgs(); a++) {
				Argument* arg = prop->getArg(a);
				if (arg->getType() == Argument::MENTION_ARG) {
					if (arg->getRoleSym() == Argument::REF_ROLE && arg->getMention(mentionSet)->getUID() == context.getUID()) {
						match = true;
					} else if (arg->getRoleSym() == Symbol(L"of")) {
						value = arg->getMention(mentionSet)->getNode()->getHeadWord();
						if (boost::regex_match(value.to_string(), email_re)) {
							key = Symbol(L"email");
						} else if (boost::regex_match(value.to_string(), ip_re)) {
							key = Symbol(L"ip");
						} else if (boost::regex_match(value.to_string(), phone_re)) {
							key = Symbol(L"phone");
						}
					}
				}
			}
			if (match && !key.is_null() && !value.is_null()) {
				results.push_back(AttributeValuePair<Symbol>::create(key, value, getFullName()));
			}
		}
	}

	// Store matching Mention text
	if (context.getMentionType() == Mention::NAME) {
		Symbol key;
		std::wstring value = context.toCasedTextString();
		if (boost::regex_match(value, email_re)) {
			key = Symbol(L"email");
		} else if (boost::regex_match(value, ip_re)) {
			key = Symbol(L"ip");
		} else if (boost::regex_match(value, phone_re)) {
			key = Symbol(L"phone");
		}
		if (!key.is_null()) {
			results.push_back(AttributeValuePair<Symbol>::create(key, Symbol(value), getFullName()));
		}
	}

	return results;
}
