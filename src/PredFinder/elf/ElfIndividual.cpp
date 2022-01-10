/**
 * Parallel implementation of ElfIndividual object based on Python
 * implementation in ELF.py. Uses Xerces-C++ for XML writing.
 *
 * @file ElfIndividual.cpp
 * @author nward@bbn.com
 * @date 2010.06.23
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/MentionConfidence.h"
#include "Generic/values/NumberConverter.h"
#include "Generic/common/TimexUtils.h"
#include "LearnIt/MainUtilities.h"
#include "PredFinder/inference/EITbdAdapter.h"
#include "PredFinder/common/ElfMultiDoc.h"
#include "PredFinder/common/SXMLUtil.h"
#include "PredFinder/macros/ReadingMacroSet.h"
#include "ElfIndividual.h"
#include "ElfDescriptor.h"
#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "boost/lexical_cast.hpp"
#include <stdexcept>

using boost::dynamic_pointer_cast;

XERCES_CPP_NAMESPACE_USE

const boost::wregex ElfIndividual::individual_id_re(
	// Required: Namespace prefix
	L"(?<namespace_prefix>[a-z]+):"

	// Required: Identifier type
	L"(?<id_type>("
	L"ace-value-mention|"
	L"ace-mention|"
	L"ace-entity|"
	L"ace-event|"
	L"xdoc"
	L"))-"

	// Optional: LDC-style document ID
	L"(?<docid>[a-zA-Z]{3,}-?"
	L"(_[a-zA-Z]{3,}_)?"
	L"\\d{8}[^-]*-)?"

	// Required: Integer index
	L"(?<id>\\d+)"

	// Optional: Integer subindex
	L"(-(?<subid>\\d+))?"
	);

bool ElfIndividual::_show_id = true;

NumberConverter_ptr ElfIndividual::number_converter;

/**
 * Initializer defaults to an empty non-value
 * individual. Intended to be called by constructors.
 *
 * @author nward@bbn.com
 * @date 2011.06.13
 **/
void ElfIndividual::initialize(void) {
	// Set the various default values to empty
	_partner_uri = L"";
	_generated_uri = L"";
	_is_value = false;
	_value = L"";
	_value_mention_id = -1;
	_mention_uid = MentionUID();
	_mention_confidence = MentionConfidenceStatus::UNKNOWN_CONFIDENCE;
	_entity_id = -1;
	_coref_uri = L"";
	_xdoc_cluster_id = -1;
	_event_id = -1;
	_bound_uri = L"";
	_date_range.first = -1;
	_date_range.second = -1;
}

/**
 * Copy constructor.
 *
 * @param other ElfIndividual to deep copy from.
 *
 * @author nward@bbn.com
 * @date 2010.06.23
 **/
ElfIndividual::ElfIndividual(const ElfIndividual_ptr other) {
	// Copy the various possible IDs for this individual
	_partner_uri = other->_partner_uri;
	_generated_uri = other->_generated_uri;
	_is_value = other->_is_value;
	_value = other->_value;
	_value_mention_id = other->_value_mention_id;
	_mention_uid = other->_mention_uid;
	_mention_confidence = other->_mention_confidence;
	_entity_id = other->_entity_id;
	_coref_uri = other->_coref_uri;
	_xdoc_cluster_id = other->_xdoc_cluster_id;
	_event_id = other->_event_id;
	_bound_uri = other->_bound_uri;

	// Copy the individual's best name text and any types
	_name_or_desc = other->_name_or_desc;
	BOOST_FOREACH(ElfType_ptr type, other->_types) {
		_types.insert(type);
	}
	_date_range.first = other->_date_range.first;
	_date_range.second = other->_date_range.second;

}

/**
* Copy constructor that also coerces the individual being
* copied into the closest Serif matches(that is, it adds
* any Serif IDs to the copy of the individual where possible).
*
* @param doc_theory The DocTheory containing the relation,
* which will be used to align with Serif mentions.
* @param other ElfRelation to deep copy from.
*
* @author nward@bbn.com
* @date 2011.06.09
 **/
ElfIndividual::ElfIndividual(const DocTheory* doc_theory, const ElfIndividual_ptr other) {
	// Copy the various possible IDs for this individual
	_partner_uri = other->_partner_uri;
	_generated_uri = other->_generated_uri;
	_is_value = other->_is_value;
	_value = other->_value;
	_value_mention_id = other->_value_mention_id;
	_mention_uid = other->_mention_uid;
	_mention_confidence = other->_mention_confidence;
	_entity_id = other->_entity_id;
	_coref_uri = other->_coref_uri;
	_xdoc_cluster_id = other->_xdoc_cluster_id;
	_event_id = other->_event_id;
	_bound_uri = other->_bound_uri;
	_date_range.first = other->_date_range.first;
	_date_range.second = other->_date_range.second;

	// Copy the individual's best name text
	_name_or_desc = other->_name_or_desc;

	// Copy the type assertion
	ElfType_ptr type = other->get_type();
	_types.insert(type);

	// Try to find a token alignment to a mention and entity based on offsets if this came from one of our partners
	if (has_partner_uri()) {
		const Entity* entity = NULL;
		EDTOffset start_offset, end_offset;
		type->get_offsets(start_offset, end_offset);
		int sent_no, start_token, end_token;
		bool token_align = MainUtilities::getSerifStartEndTokenFromCharacterOffsets(doc_theory, start_offset, end_offset, sent_no, start_token, end_token);
		if (token_align) {
			// Get the sentence theory containing this string
			//   Give up if for some reason (bad sentence number?) we can't get the sentence theory
			SentenceTheory* sent_theory = doc_theory->getSentenceTheory(sent_no);
			if (sent_theory != NULL) {
				// First, try a mention alignment for this string
				const Mention* mention = MainUtilities::getMentionFromTokenOffsets(sent_theory, start_token, end_token);
				if (mention) {
					// Store the mention ID (silently overwrites)
					_mention_uid = mention->getUID();
					_mention_confidence = MentionConfidence::determineMentionConfidence(doc_theory, sent_theory, mention);

					// Get an entity if there is one
					entity = mention->getEntity(doc_theory);
					if (entity)
						_entity_id = entity->getID();
				} else {
					// Try a value mention alignment (probably a date)
					const ValueMention* value_mention = MainUtilities::getValueMentionFromTokenOffsets(sent_theory, start_token, end_token);
					if (value_mention) {
						// Store the value mention ID (silently overwrites)
						_value_mention_id = value_mention->getUID().toInt();
						_is_value = true;
						_value = _name_or_desc->get_value();

						// If it's a date, try to override with a normalized value
						if (value_mention->isTimexValue()) {
							const Value* doc_value = value_mention->getDocValue();
							if (doc_value != NULL) {
								Symbol timex_value = doc_value->getTimexVal();
								if (timex_value != Symbol()) {
									// Use the specific date value but keep the value text
									_value = get_text_according_to_type(type->get_value(), timex_value.to_string());
								}
							}
						}
					}
				}
			}
		}

		// Whether or not we found an entity to use in lookup, try to map this individual
		if (_name_or_desc.get() != NULL && type.get() != NULL) {
			XDocIdType xdoc_id;
			ElfMultiDoc::get_mapped_arg(_name_or_desc->get_value(), type->get_value(), doc_theory, entity, _bound_uri, xdoc_id);
			_xdoc_cluster_id = xdoc_id.as_int();
		}
	}
}

/**
 * Filtered copy constructor. Intended for use when
 * reading <arg>s and <individual>s from XML and
 * aligning them by id attribute.
 *
 * @param other ElfIndividual to deep copy from.
 *
 * @author nward@bbn.com
 * @date 2011.06.07
 **/
ElfIndividual::ElfIndividual(const ElfIndividual_ptr other, const std::wstring & type_string, EDTOffset start, EDTOffset end) {
	// Copy the various possible IDs for this individual
	_partner_uri = other->_partner_uri;
	_generated_uri = other->_generated_uri;
	_is_value = other->_is_value;
	_value = other->_value;
	_value_mention_id = other->_value_mention_id;
	_mention_uid = other->_mention_uid;
	_mention_confidence = other->_mention_confidence;
	_entity_id = other->_entity_id;
	_coref_uri = other->_coref_uri;
	_xdoc_cluster_id = other->_xdoc_cluster_id;
	_event_id = other->_event_id;
	_bound_uri = other->_bound_uri;
	_date_range.first = other->_date_range.first;
	_date_range.second = other->_date_range.second;

	// Copy the individual's best name text and the first matching type
	_name_or_desc = other->_name_or_desc;
	bool all_done = false;
	BOOST_FOREACH(ElfType_ptr type, other->_types) {
		if (all_done) continue;

		// Check the type
		if (type->get_value() != type_string)
			continue;

		// Check the type offsets
		EDTOffset type_start, type_end;
		type->get_offsets(type_start, type_end);
		if (type_start != start || type_end != end)
			continue;

		// Match!
		_types.insert(boost::static_pointer_cast<ElfType>(boost::make_shared<ElfString>(type)));
		all_done = true; // can't brk in a BOOST_FOREACH
	}

	// Hack for ISI missing <arg> types and UW bad <arg> offsets
	if (other->_types.size() == 1) {
		_types.insert(boost::static_pointer_cast<ElfType>(boost::make_shared<ElfString>(*(other->_types.begin()))));
	}

	// Try an offset-only match
	if (_types.size() == 0) {
		bool all_done = false;	
		BOOST_FOREACH(ElfType_ptr type, other->_types) {
			if (all_done) continue;

			// Check the type offsets
			EDTOffset type_start, type_end;
			type->get_offsets(type_start, type_end);
			if (type_start != start || type_end != end)
				continue;

			// Match!
			_types.insert(boost::static_pointer_cast<ElfType>(boost::make_shared<ElfString>(type)));
			all_done = true; // can't brk in a BOOST_FOREACH
		}
	}

	// Check for no match
	if (_types.size() == 0) {
		std::stringstream error;
		error << "ElfIndividual::ElfIndividual(ElfIndividual_ptr,std::wstring,EDTOffset,EDTOffset):";
		error << " matching type '" << type_string << "' not found in individual " << other->get_best_uri();
		throw std::runtime_error(error.str().c_str());
	}
}

/**
 * Merge a set of individuals that have the same best URI.
 *
 * @param others The set of individuals that will be merged,
 * enforcing sameness of ID.
 *
 * @author nward@bbn.com
 * @date 2011.06.09
 **/
ElfIndividual::ElfIndividual(const ElfIndividualSet others) {
	// Set all IDs to empty
	initialize();

	// Track all of the IDs we're merging by type
	std::multiset<std::wstring> partner_uris;
	std::multiset<std::wstring> generated_uris;
	std::multiset<int> value_mention_ids;
	std::multiset<MentionUID> mention_uids;
	std::multiset<int> entity_ids;
	std::multiset<int> event_ids;
	std::multiset<std::wstring> coref_uris;
	std::multiset<int> xdoc_cluster_ids;
	std::multiset<std::wstring> bound_uris;

	// Collect all of the possible IDs for the individuals we're merging
	BOOST_FOREACH(ElfIndividual_ptr individual, others) {
		// Error if we try to merge in a value
		if (individual->has_value())
			throw std::runtime_error("ElfIndividual::ElfIndividual(ElfIndividualSet): merge attempted containing literal value");

		// Ignore unset IDs/URIs
		if (individual->has_partner_uri())
			partner_uris.insert(individual->_partner_uri);
		if (individual->has_generated_uri())
			generated_uris.insert(individual->_generated_uri);
		if (individual->has_value_mention_id())
			value_mention_ids.insert(individual->_value_mention_id);
		if (individual->has_mention_uid())
			mention_uids.insert(individual->_mention_uid);
		if (individual->has_entity_id())
			entity_ids.insert(individual->_entity_id);
		if (individual->has_event_id())
			event_ids.insert(individual->_event_id);
		if (individual->has_coref_uri())
			coref_uris.insert(individual->_coref_uri);
		if (individual->has_xdoc_cluster_id())
			xdoc_cluster_ids.insert(individual->_xdoc_cluster_id);
		if (individual->has_bound_uri())
			bound_uris.insert(individual->_bound_uri);
	}

	// Determine which IDs/URIs to keep
	_partner_uri = GET_UNIQUE_ID(partner_uris, L"");
	_generated_uri = GET_UNIQUE_ID(generated_uris, L"");
	_value_mention_id = GET_UNIQUE_ID(value_mention_ids, -1);
	_mention_uid = GET_UNIQUE_ID(mention_uids, MentionUID());
	_entity_id = GET_UNIQUE_ID(entity_ids, -1);
	_event_id = GET_UNIQUE_ID(event_ids, -1);
	_coref_uri = GET_UNIQUE_ID(coref_uris, L"");
	_xdoc_cluster_id = GET_UNIQUE_ID(xdoc_cluster_ids, -1);
	_bound_uri = GET_UNIQUE_ID(bound_uris, L"");

	// Not sure what to do here, but I think this is meaningless now
	_mention_confidence = MentionConfidenceStatus::UNKNOWN_CONFIDENCE;

	// Check for invalid merges
	if (has_event_id() && (has_mention_uid() || has_entity_id() || has_xdoc_cluster_id()))
		throw std::runtime_error("ElfIndividual::ElfIndividual(ElfIndividualSet): merge attempted between event and non-event");

	// Determine which name/desc to use
	if (has_entity_id()) {
		// Get the name of the best entity match
		bool all_done = false;
		BOOST_FOREACH(ElfIndividual_ptr individual, others) {
			if (all_done) continue;
			if (individual->_entity_id == _entity_id) {
				_name_or_desc = individual->_name_or_desc;
				all_done = true; // can't brk in a BOOST_FOREACH
			}
		}
	} else if (has_mention_uid()) {
		// Get the name of the best mention match
		bool all_done = false;
		BOOST_FOREACH(ElfIndividual_ptr individual, others) {
			if (all_done) continue;
			if (individual->_mention_uid == _mention_uid) {
				_name_or_desc = individual->_name_or_desc;
				all_done = true; // can't brk in a BOOST_FOREACH
			}
		}
	} else {
		// Just grab the longest, but favor names over descs
		ElfString_ptr longest_name_or_desc;
		BOOST_FOREACH(ElfIndividual_ptr individual, others) {
			ElfString_ptr name_or_desc = individual->_name_or_desc;
			if (name_or_desc.get() != NULL &&
				(longest_name_or_desc.get() == NULL ||
				(name_or_desc->get_type() == ElfString::NAME && longest_name_or_desc->get_type() == ElfString::DESC) ||
				 (name_or_desc->get_value().length() > longest_name_or_desc->get_value().length() &&
				  (name_or_desc->get_type() == ElfString::NAME || longest_name_or_desc->get_type() == ElfString::DESC)))) {
				longest_name_or_desc = name_or_desc;
			}
		}
		_name_or_desc = longest_name_or_desc;
	}

	// Collect all of the type assertions (keeping all non-duplicates)
	BOOST_FOREACH(ElfIndividual_ptr individual, others) {
		BOOST_FOREACH(ElfType_ptr type, individual->_types) {
			_types.insert(boost::static_pointer_cast<ElfType>(boost::make_shared<ElfString>(type)));
		}
	}
}

/**
 * Construct an individual from a particular ElfString and ElfTypeSet.
 *
 * @param uri Document-unique identifier, presumably
 * generated by a factory method.
 * @param name A previously constructed ElfString
 * containing a name or descriptor string and
 * possibly offsets.
 * @param type A previously constructed ElfType
 * containing a type string and possibly offsets.
 *
 * @author nward@bbn.com
 * @date 2010.06.23
 **/
ElfIndividual::ElfIndividual(const std::wstring & uri, const ElfString_ptr name_or_desc, const ElfType_ptr type) {
	// Set all IDs to empty
	initialize();

	// Store the individual's within-document ID, best name text, and a type
	_generated_uri = uri;
	_name_or_desc = name_or_desc;
	_types.insert(type);

	// Try to extract a value if this contained a literal type
	if (type.get() != NULL && boost::starts_with(type->get_value(), L"xsd:")) {
		_value = get_text_according_to_type(type->get_value(), type->get_string());
		_is_value = true;
	}
}

/**
 * Constructs an ElfIndividual from a
 * SnippetReturnFeature found using manual
 * Distillation patterns.
 *
 * @param doc_theory The DocTheory containing the matched
 * ReturnPatternFeature, used for determining offsets.
 * @param feature The ReturnPatternFeature that contains
 * a mention, text, etc. we can convert to an ElfIndividual.
 * @return The constructed ElfIndividual.
 *
 * @author nward@bbn.com
 * @date 2010.06.28
 **/
ElfIndividual::ElfIndividual(const DocTheory* doc_theory, ReturnPatternFeature_ptr feature) {
	// Set all IDs to empty
	initialize();

	// Check for bad feature or document
	if (feature == NULL) {
		throw std::runtime_error("ElfIndividual::ElfIndividual(DocTheory*, ReturnPatternFeature_ptr): Feature null");
	}
	if (doc_theory == NULL) {
		throw std::runtime_error("ElfIndividual::ElfIndividual(DocTheory*, ReturnPatternFeature_ptr): Document theory null");
	}

	// Generate a set of types from the return feature
	if (!feature->hasReturnValue(L"type"))
		throw std::runtime_error("ElfIndividualFactory::from_return_feature(DocTheory*, ReturnPatternFeature_ptr): "
		"No type return specified in manual pattern");

	// Check for TBD role/type if there isn't already a role specified
	if (!feature->hasReturnValue(L"role")) {
		std::wstring tbd = feature->getReturnValue(L"tbd");
		if (tbd.size() != 0 &&
			(tbd.compare(L"generic_violence") == 0 ||
			 tbd.compare(L"killing") == 0 || tbd.compare(L"injury") == 0 ||
			 tbd.compare(L"bombing") == 0 || tbd.compare(L"attack") == 0))
		{
			// Generate the appropriate role/type return values
			feature->setReturnValue(L"role", EITbdAdapter::getEventRole(tbd));
			feature->setReturnValue(L"type", EITbdAdapter::getEventType(tbd, false));
		} else if (tbd.size() != 0) {
			std::stringstream error;
			error << "ElfIndividual::ElfIndividual(DocTheory*, ReturnPatternFeature_ptr): "
				"Unknown \"tbd\" individual feature in pattern file: " << tbd;
			throw std::runtime_error(error.str().c_str());
		}			
	}

	// Check for subclass return types
	const Mention* mention = NULL;
	Entity* entity = NULL;

	if (MentionReturnPFeature_ptr mf = dynamic_pointer_cast<MentionReturnPFeature>(feature)) {// WAS: feature->getReturnType() == SnippetFeature::MENTION) {
		// Check that we have a valid Mention
		mention = mf->getMention();
		if (mention == NULL)
			throw std::runtime_error("ElfIndividual::ElfIndividual(DocTheory*, ReturnPatternFeature_ptr): Mention was null");
		_mention_uid = mention->getUID();
		_mention_confidence = MentionConfidence::determineMentionConfidence(doc_theory, doc_theory->getSentenceTheory(mention->getSentenceNumber()), mention);

		// Check if this mention has an associated entity
		entity = mention->getEntity(doc_theory);
		if (entity != NULL) {
			// Initialize everything else from the Mention's entity
			_entity_id = entity->getID();
		}
	} else if (EventMentionReturnPFeature_ptr vmf = dynamic_pointer_cast<EventMentionReturnPFeature>(feature)) { // WAS: feature->getReturnType() == SnippetFeature::VM) {
		// Initialize everything else from the EventMention
		const EventMention* event_mention = vmf->getEventMention();
		if (event_mention == NULL)
			throw std::runtime_error("ElfIndividual::ElfIndividual(DocTheory*, ReturnPatternFeature_ptr): EventMention was null");
		Event* event = event_mention->getEvent(doc_theory);
		if (event == NULL)
			throw std::runtime_error("ElfIndividual::ElfIndividual(DocTheory*, ReturnPatternFeature_ptr): Event was null");
		_event_id = event->getID();
	}

	// Get a descriptor string and offsets from the sentence
	SentenceTheory* sent_theory = doc_theory->getSentenceTheory(feature->getSentenceNumber());
	if (sent_theory) {
		TokenSequence* token_sequence = sent_theory->getTokenSequence();
		if (token_sequence) {
			// Get the offsets for the return feature
			EDTOffset start = token_sequence->getToken(feature->getStartToken())->getStartEDTOffset();
			EDTOffset end = token_sequence->getToken(feature->getEndToken())->getEndEDTOffset();

			// Get the text for the feature's token span
			LocatedString* token_span = MainUtilities::substringFromEdtOffsets(doc_theory->getDocument()->getOriginalText(), start, end);
			std::wstring text = std::wstring(token_span->toString());
			delete token_span;

			// Check if we have a mention; the more specific text will be used for the type assertion and the arg
			EDTOffset mention_start, mention_end;
			std::wstring mention_text;
			if (_mention_uid != MentionUID()) {
				// Get the offsets for the mention itself
				mention_start = token_sequence->getToken(mention->getNode()->getStartToken())->getStartEDTOffset();
				mention_end = token_sequence->getToken(mention->getNode()->getEndToken())->getEndEDTOffset();

				// Get the evidence string directly from the document using the mention's offsets
				LocatedString* mention_token_span = MainUtilities::substringFromEdtOffsets(doc_theory->getDocument()->getOriginalText(), mention_start, mention_end);
				mention_text = std::wstring(mention_token_span->toString());
				delete mention_token_span;

				// Use the mention text we have for the type
				_types.insert(boost::make_shared<ElfType>(feature->getReturnValue(L"type"), mention_text, mention_start, mention_end));
			} else {
				// Use the best feature evidence we have for the type
				_types.insert(boost::make_shared<ElfType>(feature->getReturnValue(L"type"), text, start, end));
			}

			// Check if a name or desc override was specified (this is rare)
			if (feature->hasReturnValue(L"name")) {
				_name_or_desc = boost::make_shared<ElfName>(feature->getReturnValue(L"name"), 1.0, text, start, end);
			} else if (feature->hasReturnValue(L"desc")) {
				_name_or_desc = boost::make_shared<ElfDescriptor>(feature->getReturnValue(L"desc"), 1.0, text, start, end);
			} else if (_entity_id != -1) {
				// Get best name and mention from the entity (based on SlotFiller::_findBestEntityName)
				std::pair<std::wstring, const Mention*> best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
				if (best_name_mention.first != L"NO_NAME" && best_name_mention.second != NULL) {
					// Get the best name from the best name mention (getBestNameWithSourceMention returns tokenized text, boo)
					EDTOffset best_name_start, best_name_end;
					double best_name_confidence = 1.0; // Not using this yet
					std::wstring type_string = L""; // No longer used (we don't generate ACE subtypes)
					std::wstring name_string = ElfMultiDoc::find_best_text_for_mention(doc_theory, best_name_mention.second, type_string, best_name_start, best_name_end);

					// Get a URI replacement based on the entity name, if any
					XDocIdType xdoc_id;
					std::wstring best_uri = ElfMultiDoc::get_mapped_arg(name_string, feature->getReturnValue(L"type"), doc_theory, entity, _bound_uri, xdoc_id);
					_xdoc_cluster_id = xdoc_id.as_int();

					// Obtain the best name or descriptor for this entity
					switch (best_name_mention.second->getMentionType()) {
					case Mention::NAME:
						_name_or_desc = boost::make_shared<ElfName>(name_string, best_name_confidence, name_string, best_name_start, best_name_end);
						break;
					case Mention::DESC:
						_name_or_desc = boost::make_shared<ElfDescriptor>(name_string, best_name_confidence, name_string, best_name_start, best_name_end);
						break;
					default:
						// For ELF purposes, other mention types don't have a name or descriptor, but we may need the offsets later
						_name_or_desc = boost::make_shared<ElfDescriptor>(L"", best_name_confidence, name_string, best_name_start, best_name_end);
					}
				}
			} else if (TokenSpanReturnPFeature_ptr tsf = dynamic_pointer_cast<TokenSpanReturnPFeature>(feature)) {
				// Use text evidence from pattern match as descriptive text
				_name_or_desc = boost::make_shared<ElfDescriptor>(text, 1.0, text, start, end);
			}

			// Check if we didn't yet find a name/desc
			if (_name_or_desc.get() == NULL) {
				// Check if we didn't have an entity, or didn't have a best name mention for an entity
				if (_mention_uid != MentionUID()) {
					// Use text evidence from mention as descriptive text
					if (mention->getMentionType() == Mention::NAME) {
						_name_or_desc = boost::make_shared<ElfName>(mention_text, 1.0, mention_text, mention_start, mention_end);
					} else {
						_name_or_desc = boost::make_shared<ElfDescriptor>(mention_text, 1.0, mention_text, mention_start, mention_end);
					}
				} else {
					// No name or desc found or specified, just create a descriptor string from the text with offsets
					_name_or_desc = boost::make_shared<ElfDescriptor>(L"", 1.0);
                }
            }

			// Try to extract a value if this contained a literal type
			if (boost::starts_with(feature->getReturnValue(L"type"), L"xsd:")) {
				_value = get_text_according_to_type(feature->getReturnValue(L"type"), _name_or_desc->get_string());
				_is_value = true;
			}

			// Generate a globally-unique ID from the offsets
			std::wstring docid = std::wstring(doc_theory->getDocument()->getName().to_string());
			_generated_uri = generate_uri_from_offsets(docid, start, end);
		} else {
			std::stringstream error;
			error << "ElfIndividual::ElfIndividual(DocTheory*, ReturnPatternFeature_ptr): "
				"Token sequence null for sentence " << feature->getSentenceNumber();
			throw std::runtime_error(error.str().c_str());
		}
	} else {
		std::stringstream error;
		error << "ElfIndividual::ElfIndividual(DocTheory*, ReturnPatternFeature_ptr): "
			"Sentence theory null for sentence " << feature->getSentenceNumber();
		throw std::runtime_error(error.str().c_str());
	}
}

/**
 * Constructs an ElfIndividual from a
 * Serif Mention and type URI, presumably found with
 * LearnIt.
 *
 * @param doc_theory The DocTheory containing the specified
 * Mention, used for determining offsets.
 * @param type_string The URI of the ontology type to use for this individual.
 * @param value_mention The value mention we can convert to an ElfIndividual.
 *
 * @author nward@bbn.com
 * @date 2011.06.13
 **/
ElfIndividual::ElfIndividual(const DocTheory* doc_theory, const std::wstring & type_string, const Mention* mention) {
	// Set all IDs to empty
	initialize();

	// Check for bad mention or document
	if (doc_theory == NULL) {
		throw std::runtime_error("ElfIndividual::ElfIndividual(DocTheory*, std::wstring, Mention*): Document theory null");
	}
	if (mention == NULL) {
		throw std::runtime_error("ElfIndividual::ElfIndividual(DocTheory*, std::wstring, Mention*): Mention null");
	}

	// Get the mention ID
	_mention_uid = mention->getUID();
	_mention_confidence = MentionConfidence::determineMentionConfidence(doc_theory, doc_theory->getSentenceTheory(mention->getSentenceNumber()), mention);

	// Check if this mention has an associated entity
	const Entity* entity = mention->getEntity(doc_theory);
	if (entity != NULL) {
		// Initialize everything else from the Mention's entity
		_entity_id = entity->getID();
	}

	// Get a descriptor string and offsets from the sentence
	SentenceTheory* sent_theory = doc_theory->getSentenceTheory(mention->getSentenceNumber());
	if (sent_theory) {
		TokenSequence* token_sequence = sent_theory->getTokenSequence();
		if (token_sequence) {
			// Get the offsets for the mention itself
			EDTOffset start = token_sequence->getToken(mention->getNode()->getStartToken())->getStartEDTOffset();
			EDTOffset end = token_sequence->getToken(mention->getNode()->getEndToken())->getEndEDTOffset();

			// Get the evidence string directly from the document using the mention's offsets
			LocatedString* mention_text = MainUtilities::substringFromEdtOffsets(doc_theory->getDocument()->getOriginalText(), start, end);
			std::wstring text = std::wstring(mention_text->toString());
			delete mention_text;

			// Generate the specified type with the mention's offsets
			_types.insert(boost::make_shared<ElfType>(type_string, text, start, end));

			// Check if a name or desc override was specified (this is rare)
			if (entity != NULL) {
				// Get best name and mention from the entity (based on SlotFiller::_findBestEntityName)
				std::pair<std::wstring, const Mention*> best_name_mention = entity->getBestNameWithSourceMention(doc_theory);
				if (best_name_mention.first != L"NO_NAME" && best_name_mention.second != NULL) {
					// Get the best name from the best name mention (getBestNameWithSourceMention returns tokenized text, boo)
					EDTOffset best_name_start, best_name_end;
					double best_name_confidence = 1.0; // Not using this yet
					std::wstring ace_type_string = L""; // No longer used (we don't generate ACE subtypes)
					std::wstring name_string = ElfMultiDoc::find_best_text_for_mention(doc_theory, best_name_mention.second, ace_type_string, best_name_start, best_name_end);

					// Get a URI replacement based on the entity name, if any
					XDocIdType xdoc_id;
					if(!boost::ends_with(type_string, L"Title"))
						std::wstring best_uri = ElfMultiDoc::get_mapped_arg(name_string, type_string, doc_theory, entity, _bound_uri, xdoc_id);
					_xdoc_cluster_id = xdoc_id.as_int();

					// Obtain the best name or descriptor for this entity
					switch (best_name_mention.second->getMentionType()) {
					case Mention::NAME:
						_name_or_desc = boost::make_shared<ElfName>(name_string, best_name_confidence, name_string, best_name_start, best_name_end);
						break;
					case Mention::DESC:
						_name_or_desc = boost::make_shared<ElfDescriptor>(name_string, best_name_confidence, name_string, best_name_start, best_name_end);
						break;
					default:
						// For ELF purposes, other mention types don't have a name or descriptor, but we may need the offsets later
						_name_or_desc = boost::make_shared<ElfDescriptor>(L"", best_name_confidence, name_string, best_name_start, best_name_end);
					}
				}
			} else {
				// Get a URI replacement based on the mention text, if any
				XDocIdType xdoc_id;
				std::wstring best_uri = ElfMultiDoc::get_mapped_arg(text, type_string, doc_theory, entity, _bound_uri, xdoc_id);
				_xdoc_cluster_id = xdoc_id.as_int();
			}

			// Check if we didn't yet find a name/desc
			if (_name_or_desc.get() == NULL) {
				if (_mention_uid != MentionUID()) {
					if (mention->getMentionType() == Mention::NAME) {
						_name_or_desc = boost::make_shared<ElfName>(text, 1.0, text, start, end);
					} else {
						_name_or_desc = boost::make_shared<ElfDescriptor>(text, 1.0, text, start, end);
					}
				} else {
					// No name or desc found or specified, just create a descriptor string from the text with offsets
					_name_or_desc = boost::make_shared<ElfDescriptor>(L"", 1.0);
				}
			}

			// Try to extract a value if this contained a literal type
			if (boost::starts_with(type_string, L"xsd:")) {
				_value = get_text_according_to_type(type_string, text);
				_is_value = true;
			}

			// Generate a globally-unique ID from the offsets
			std::wstring docid = std::wstring(doc_theory->getDocument()->getName().to_string());
			_generated_uri = generate_uri_from_offsets(docid, start, end);
		} else {
			std::stringstream error;
			error << "ElfIndividual::ElfIndividual(DocTheory*, std::wstring, Mention*): Token sequence null for sentence " << mention->getSentenceNumber();
			throw std::runtime_error(error.str().c_str());
		}
	} else {
		std::stringstream error;
		error << "ElfIndividual::ElfIndividual(DocTheory*, std::wstring, Mention*): Sentence theory null for sentence " << mention->getSentenceNumber();
		throw std::runtime_error(error.str().c_str());
	}
}

/**
 * Reads this ElfIndividual from an XML <individual> element.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param individual The DOMElement containing an <individual>.
 *
 * @author nward@bbn.com
 * @date 2010.08.31
 **/
ElfIndividual::ElfIndividual(const DOMElement* individual, const std::wstring & docid, const std::wstring & document_source, bool exclusive_end_offsets) {
	// Set all IDs to empty
	initialize();

	// Read the required individual metadata attributes
    std::wstring id = SXMLUtil::getAttributeAsStdWString(individual, "id");
	if (id == L"")
		throw std::runtime_error("ElfIndividual::ElfIndividual(DOMElement*): No id attribute specified on <individual>");

	// Create a URI prefix for use when generating individual URIs, based on the document's source site
	std::wstring namespace_prefix(document_source.substr(0, document_source.find(L" ")));
	std::transform(namespace_prefix.begin(), namespace_prefix.end(), namespace_prefix.begin(), towlower);

	// Convert any non-URI individual ID to a URI using the document source as a namespace prefix
	std::wstringstream new_id;
	if (boost::regex_match(id, boost::basic_regex<wchar_t>(L"\\d+"))) {
		// Integer
		new_id << namespace_prefix << L":" << docid << L"-" << id;
		id = new_id.str();
	} else if(boost::regex_match(id, boost::basic_regex<wchar_t>(docid + L"-\\d+"))) {
		// Document ID + integer
		new_id << namespace_prefix << L":" << id;
		id = new_id.str();
	}

	// Parse the read ID into an internal identifier if possible
	boost::wcmatch id_match;
	if (boost::starts_with(id, namespace_prefix)) {
		// Read from a non-BBN partner site
		_partner_uri = id;
	} else if (boost::regex_match(id.c_str(), id_match, individual_id_re)) {
		// Try to extract the appropriate Serif-internal integer ID
		std::wstring id_type = id_match.str(L"id_type");
		int integer_id = boost::lexical_cast<int>(id_match.str(L"id"));
		if (id_type == L"ace-mention") {
			_mention_uid = MentionUID(integer_id);
		} else if (id_type == L"ace-entity")
			_entity_id = integer_id;
		else if (id_type == L"ace-event")
			_event_id = integer_id;
		else if (id_type == L"xdoc")
			_xdoc_cluster_id = integer_id;
	} else if (boost::starts_with(id, ReadingMacroSet::domain_prefix)) {
		// Bound URI from the domain
		_bound_uri = id;
	} else {
		// Must be something else
		_generated_uri = id;
	}

	// Note that we neither write out nor read in _mention_confidence at this point

	// Read the <name>/<desc> and <type>s in this <individual>
	DOMNodeList* child_nodes = individual->getChildNodes();
	for (XMLSize_t n = 0; n < child_nodes->getLength(); n++) {
		// Ignore non-element children
		DOMNode* child_node = child_nodes->item(n);
		if (child_node->getNodeType() != DOMNode::ELEMENT_NODE)
			continue;

		// Get the element and its tag name
		DOMElement* child_element = dynamic_cast<DOMElement*>(child_node);
		std::wstring tag = SerifXML::transcodeToStdWString(child_element->getTagName());

		// Check the child element tag name
		if (tag == L"name") {
			if (_name_or_desc.get() == NULL) {
				_name_or_desc = ElfString::from_xml(child_element, exclusive_end_offsets);
			} else {
				throw std::runtime_error("ElfIndividual::ElfIndividual(DOMElement*): Found more than one <name> or <desc> in <individual>");
			}
		} else if (tag == L"desc") {
			if (_name_or_desc.get() == NULL) {
				_name_or_desc = ElfString::from_xml(child_element, exclusive_end_offsets);
			} else {
				throw std::runtime_error("ElfIndividual::ElfIndividual(DOMElement*): Found more than one <name> or <desc> in <individual>");
			}
		} else if (tag == L"type") {
			_types.insert(boost::static_pointer_cast<ElfType>(ElfString::from_xml(child_element, exclusive_end_offsets)));
		}
	}

	// Make sure that at least one type was specified
	if (_types.size() == 0)
		throw std::runtime_error("ElfIndividual::ElfIndividual(DOMElement*): No <type>s in <individual>");
}

/**
 * Construct an ElfIndividual from a ValueMention. Always normalizes the
 * ValueMention's text according to type if possible.
 *
 * @param doc_theory The DocTheory containing the ValueMention.
 * @param type_string The URI of the ontology type to use for this individual.
 * @param value_mention The value mention we can convert to an ElfIndividual.
 *
 * @author nward@bbn.com
 * @date 2011.05.05
 **/
ElfIndividual::ElfIndividual(const DocTheory* doc_theory, const std::wstring & type_string, const ValueMention* value_mention) {
	// Set all IDs to empty
	initialize();

	// Check that we have a valid ValueMention
	if (value_mention == NULL)
		throw std::runtime_error("ElfIndividual::ElfIndividual(DocTheory*, ValueMention*): ValueMention was null");

	// Store the value mention ID
	std::wstring docid = std::wstring(doc_theory->getDocument()->getName().to_string());
	_value_mention_id = value_mention->getUID().toInt();
	_is_value = true;

	// Get a descriptor string and offsets from the sentence
	SentenceTheory* sent_theory = doc_theory->getSentenceTheory(value_mention->getSentenceNumber());
	if (sent_theory) {
		TokenSequence* token_sequence = sent_theory->getTokenSequence();
		if (token_sequence) {
			// Get the offsets for the return feature
			EDTOffset start = token_sequence->getToken(value_mention->getStartToken())->getStartEDTOffset();
			EDTOffset end = token_sequence->getToken(value_mention->getEndToken())->getEndEDTOffset();

			// Get the text for the token span
			LocatedString* token_span = MainUtilities::substringFromEdtOffsets(doc_theory->getDocument()->getOriginalText(), start, end);
			std::wstring text = std::wstring(token_span->toString());
			delete token_span;

			// If it's a date, try to override with a normalized value
			if (value_mention->isTimexValue()) {
				const Value* doc_value = value_mention->getDocValue();
				if (doc_value != NULL) {
					Symbol timex_value = doc_value->getTimexVal();
					if (timex_value != Symbol()) {
						// Use the specific date value but keep the value text
						_value = get_text_according_to_type(type_string, timex_value.to_string());
					}
				}
			}

			// Just use the filtered value mention text as the value, even though it's redundant
			if (_value == L"")
				_value = get_text_according_to_type(type_string, text);

			// Add the specified type and desc using the value mention text
			_name_or_desc = boost::make_shared<ElfDescriptor>(_value, 1.0, text, start, end);
			_types.insert(boost::make_shared<ElfType>(type_string, text, start, end));

			// Treat Blitz types as individuals even though we find them with a value model
			if (boost::starts_with(type_string, L"ic:PharmaceuticalSubstance") ||
				boost::starts_with(type_string, L"ic:PhysiologicalCondition")) {
				// Generate an individual ID and clear the value
				std::wstring docid = std::wstring(doc_theory->getDocument()->getName().to_string());
				_generated_uri = generate_uri_from_offsets(docid, start, end);
				_value = L"";
				_is_value = false;

				// Try a text-based lookup into the bound URIs table, assuming the final type
				XDocIdType xdoc_id;
				std::wstring lookup_type = L"";
				if (boost::starts_with(type_string, L"ic:PharmaceuticalSubstance"))
					lookup_type = L"ic:PharmaceuticalSubstance";
				if (boost::starts_with(type_string, L"ic:PhysiologicalCondition"))
					lookup_type = L"ic:PhysiologicalCondition";
				ElfMultiDoc::get_mapped_arg(_name_or_desc->get_value(), lookup_type, doc_theory, NULL, _bound_uri, xdoc_id);
			}
		}
	}
}


/**
 * Construct an ElfIndividual from two ValueMentions which provide a date range. 
 * Use provided normalization
 *
 * @param doc_theory The DocTheory containing the ValueMention.
 * @param type_string The URI of the ontology type to use for this individual.
 * @param value_mention The value mention we can convert to an ElfIndividual.
 *
 * @author nward@bbn.com
 * @date 2011.05.05
 **/
ElfIndividual::ElfIndividual(const DocTheory* doc_theory, const std::wstring & type_string, const std::wstring & value, const ValueMention* v1, const ValueMention* v2){
	initialize();
	_is_value = true;
	int sent_no = 0;
	if(v1){
		sent_no = v1->getSentenceNumber();
		_date_range.first = v1->getUID().toInt();
	}
	if(v2){
		sent_no = v2->getSentenceNumber();
		_date_range.second = v2->getUID().toInt();
	}
	_value = value;
	_is_value = true;
	// Pair of values instead of single value, generate range id that includes both
	_value_mention_id = -1;
	std::wstringstream value_uri;
	value_uri << L"bbn:value-range-"<<_date_range.first <<" "<<_date_range.second;
	_generated_uri = value_uri.str();

	SentenceTheory* sent_theory = doc_theory->getSentenceTheory(sent_no);
	if (sent_theory) {
		TokenSequence* token_sequence = sent_theory->getTokenSequence();
		if (token_sequence){
			// Get the offsets for the return feature
			EDTOffset start;
			EDTOffset end;
			if(v1 != 0 && v2 != 0){
				start = std::min(token_sequence->getToken(v1->getStartToken())->getStartEDTOffset(), token_sequence->getToken(v2->getStartToken())->getStartEDTOffset());
				end = std::max(token_sequence->getToken(v1->getEndToken())->getEndEDTOffset(), token_sequence->getToken(v2->getEndToken())->getEndEDTOffset());
			}
			else if(v1){
				start = token_sequence->getToken(v1->getStartToken())->getStartEDTOffset();
				end = token_sequence->getToken(v1->getEndToken())->getEndEDTOffset();
			}
			else if(v2){
				start = token_sequence->getToken(v2->getStartToken())->getStartEDTOffset();
				end = token_sequence->getToken(v2->getEndToken())->getEndEDTOffset();
			}
			// Get the text for the token span
			LocatedString* token_span = MainUtilities::substringFromEdtOffsets(doc_theory->getDocument()->getOriginalText(), start, end);
			std::wstring text = std::wstring(token_span->toString());
			delete token_span;
			// Add the specified type and desc using the value mention text
			_name_or_desc = boost::make_shared<ElfDescriptor>(_value, 1.0, text, start, end);
			_types.insert(boost::make_shared<ElfType>(type_string, text, start, end));
		}
	}



}

/**
 * Construct an ElfIndividual from a specified type and value string.
 * Intended only for use with literal values (integers, dates, etc.).
 * Always normalizes the value string according to type if possible.
 *
 * @param type_string The string URI of the type of the new value.
 * @param value The string value itself.
 * @param text An optional text evidence for the new value.
 * @param start An optional start offset for the text evidence.
 * @param end An optional end offset for the text evidence.
 *
 * @author nward@bbn.com
 * @date 2011.05.09
 **/
ElfIndividual::ElfIndividual(const std::wstring & type_string, const std::wstring & value, const std::wstring & text, EDTOffset start, EDTOffset end) {
	// Set all IDs to empty
	initialize();

	// Filter the value text
	//   We don't need a doc_theory here because that only applies to a rare case for NFLTeam lookup
	_value = get_text_according_to_type(type_string, value);
	_is_value = true;

	// No associated Serif ValueMention, generate a placeholder URI
	_value_mention_id = -1;
	boost::hash<std::wstring> string_hasher;
	std::wstringstream value_uri;
	value_uri << L"bbn:value-" << string_hasher(_value);
	_generated_uri = value_uri.str();

	// Add the specified type and desc using the value mention text
	_name_or_desc = boost::make_shared<ElfDescriptor>(_value, 1.0, text, start, end);
	_types.insert(boost::make_shared<ElfType>(type_string, text, start, end));
}

/**
 * Determines heuristically the best ID to use for this individual,
 * a decision generally based on types, and converts non-string IDs
 * to string URIs where necessary.
 *
 * @param docid Optional. When specified, guarantees that the returned
 * URI is globally unique. Usually only specified when serializing
 * documents; for most other comparisons the "local" URI is sufficient.
 * @return The best, most general URI for this individual, depending on
 * its types. In most cases, this is the broadest (that is, most likely to
 * be merged across sentences and documents) URI. Throws an exception if
 * no valid URI can be generated from this individual's set of IDs. If a
 * docid is specified, this is guaranteed to be globally unique; if not, it
 * may be globally unique if this individual contains an xdoc cluster ID or
 * a bound URI, but it is only guaranteed to be document unique in that case.
 *
 * @author nward@bbn.com
 * @date 2011.06.08
 **/
std::wstring ElfIndividual::get_best_uri(const std::wstring & docid) const {
	// Start with nothing (indicates logic error)
	std::wstring best_uri = L"";

	// If we have one, start with a partner site original URI, which will probably be overwritten
	if (has_partner_uri())
		best_uri = _partner_uri;

	// If we don't have additional Serif-based information, just use the generated URI,
	// which is probably based on offsets or a hash
	if (has_generated_uri())
		best_uri = _generated_uri;

	// Check which kind of underlying Serif ID we have
	if (has_mention_uid()) {
		// Generate a URI containing the mention UID
		std::wstringstream mention_uri;
		mention_uri << L"bbn:ace-mention-";
		if (docid != L"")
			mention_uri << docid << L"-";
		mention_uri << _mention_uid;
		best_uri = mention_uri.str();
	} else if (has_value_mention_id()) {
		// Generate a URI containing the value mention ID
		std::wstringstream value_mention_uri;
		value_mention_uri << L"bbn:ace-value-mention-";
		if (docid != L"")
			value_mention_uri << docid << L"-";
		value_mention_uri << get_value_mention_id();
		best_uri = value_mention_uri.str();
	} else if (has_event_id()) {
		// Generate a URI containing the event ID
		std::wstringstream event_uri;
		event_uri << L"bbn:ace-event-";
		if (docid != L"")
			event_uri << docid << L"-";
		event_uri << _event_id;
		best_uri = event_uri.str();
	}

	// For GPE-specs, we don't want to merge past the mention level,
	// because we're also asserting a bunch of gazetteer relations.
	if (has_type(L"kbp:GPE-spec")) {
		if (best_uri == L"")
			throw std::runtime_error("ElfIndividual::get_best_uri(): Could not determine best URI for GPE-spec.");
		return best_uri;
	}

	// For certain individual types, we don't want to merge past the mention
	// level, because they're linked by Serif to PER entities, and we want
	// them to have a distinct URI... *unless* we matched a bound URI.
	if (has_type(L"ic:Position") || has_type(L"kbp:Title") ||
		has_type(L"kbp:HeadOfCompanyTitle") || has_type(L"kbp:HeadOfNationStateTitle") ||
		has_type(L"kbp:HeadOfCityTownOrVillageTitle") || has_type(L"kbp:MinisterTitle")) 
	{
		if (has_bound_uri())
			best_uri = _bound_uri;
		if (has_coref_uri())
			best_uri = _coref_uri;
		if (best_uri == L"")
			throw std::runtime_error("ElfIndividual::get_best_uri(): Could not determine best URI for Title/Position.");
		return best_uri;
	}

	// For most mentions, if we have a document-level entity, use that instead
	//   Note that a merged individual could have an entity ID without a
	//   mention UID, if it was merged across mentions.
	if (has_entity_id()) {
		// Generate a URI containing the entity ID
		std::wstringstream entity_uri;
		entity_uri << L"bbn:ace-entity-";
		if (docid != L"")
			entity_uri << docid << L"-";
		entity_uri << _entity_id;
		best_uri = entity_uri.str();
	}

	// For most entities, if we have a cross-document cluster ID, use that instead
	//   Note that a merged individual could have a cluster ID without an
	//   entity ID, if it was merged across entities somehow.
	if (has_xdoc_cluster_id()) {
		// Generate a URI containing the xdoc cluster ID
		std::wstringstream xdoc_cluster_uri;
		xdoc_cluster_uri << L"bbn:xdoc-" << _xdoc_cluster_id;
		best_uri = xdoc_cluster_uri.str();
	}

	// For some individuals, we have our own coreference that we want to use
	// in place of whatever Serif might have done.
	if (has_coref_uri())
		best_uri = _coref_uri;

	// Always use a bound URI if available
	if (has_bound_uri())
		best_uri = _bound_uri;

	// Done
	if (best_uri == L"")
		throw std::runtime_error("ElfIndividual::get_best_uri(): Could not determine best URI.");
	return best_uri;
}

/**
 * Checks whether another individual is an exact ID match
 * with this one, at all levels. Intended for use when
 * matching individuals for removal as part of ElfInference.
 *
 * @param other ElfIndividual to compare all IDs/URIs with.
 * @return True if all ID/URI members are exactly equal.
 *
 * @author nward@bbn.com
 * @date 2011.09.21
 **/
bool ElfIndividual::has_equal_ids(const ElfIndividual_ptr other) const {
	// Check the various possible IDs for this individual
	return
	(
		_partner_uri == other->_partner_uri &&
		_generated_uri == other->_generated_uri &&
		_is_value == other->_is_value &&
		_value == other->_value &&
		_value_mention_id == other->_value_mention_id &&
		_mention_uid == other->_mention_uid &&
		_entity_id == other->_entity_id &&
		_coref_uri == other->_coref_uri &&
		_xdoc_cluster_id == other->_xdoc_cluster_id &&
		_event_id == other->_event_id &&
		_bound_uri == other->_bound_uri
	);
}

/**
 * Finds the lowest start offset and highest end offset of
 * any type assertion contained in this ElfIndividual.
 *
 * @param start The lowest ElfType _start. Pass-by-reference.
 * @param end The highest ElfType _end. Pass-by-reference.
 *
 * @author nward@bbn.com
 * @date 2011.08.11
 **/
void ElfIndividual::get_spanning_offsets(EDTOffset& start, EDTOffset& end) const {
	// Loop over all of the members' relations' args and individuals
	start = EDTOffset(INT_MAX);
	end = EDTOffset(0);
	BOOST_FOREACH(ElfType_ptr type, _types) {
		EDTOffset type_start, type_end;
		type->get_offsets(type_start, type_end);
		if (type_start.is_defined() && type_start < start)
			start = type_start;
		if (type_end.is_defined() && type_end > end)
			end = type_end;
	}
}

/**
 * Returns the countable set of all type strings used in
 * this ElfIndividual. Intended primarily for use in
 * ElfIndividualCluster::get_generated_uri() for determining
 * the best type string to use in its returned URI.
 *
 * @author nward@bbn.com
 * @date 2011.08.10
 **/
std::multiset<std::wstring> ElfIndividual::get_type_strings(void) const {
	// Loop over all of the types in this individual, accumulating the type strings
	std::multiset<std::wstring> type_strings;
	BOOST_FOREACH(ElfType_ptr type, _types) {
		// Ignore bad types
		if (type.get() != NULL && type->get_value() != L"")
			type_strings.insert(type->get_value());
	}
	return type_strings;
}

/**
 * Determine whether this individual has this type.
 * Ignores provenance.
 *
 * @param type_string The type to check.
 *
 * @author eboschee@bbn.com
 * @date 2011.01.07
 **/
bool ElfIndividual::has_type(const std::wstring & type_string) const {
	BOOST_FOREACH(ElfType_ptr type, _types) {
		if (type->get_value() == type_string)
			return true;
	}
	return false;
}

/**
 * Converts this entity to an XML <individual> element using the
 * Xerces-C++ library.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param doc An already-instantiated Xerces DOMDocument that
 * provides namespace context for the created element (since
 * Xerces doesn't support easy anonymous element import).
 * @return The constructed <individual> DOMElement
 * @param docid The unique ID for the document containing this
 * individual, used to guarantee that the serialized ID for this
 * individual is globally unique.
 *
 * @author nward@bbn.com
 * @date 2010.06.23
 **/
DOMElement* ElfIndividual::to_xml(DOMDocument* doc, const std::wstring & docid) const {
	// Create a new individual element
    DOMElement* individual = SXMLUtil::createElement(doc, "individual");
    
	// Determine the best URI to use for this <individual>
	if (_show_id) {
        SXMLUtil::setAttributeFromStdWString(individual, "id", get_best_uri(docid));
	}

	// Note that we are currently not printing _mention_confidence here

	// Add this individual's name or description (subclassing takes care of the element distinction)
	if (_name_or_desc.get() != NULL && _name_or_desc->get_value() != L"")
		individual->appendChild(_name_or_desc->to_xml(doc));

	// Loop over all possible types of this entity
	BOOST_FOREACH(ElfType_ptr type, _types) {
		individual->appendChild(type->to_xml(doc));
	}

	// Done
	return individual;
}

bool ElfIndividual::are_coreferent(const DocTheory* doc_theory, 
								   const ElfIndividual_ptr left_individual, 
								   const ElfIndividual_ptr right_individual) {
	// First, make sure we're not trying to corefer distinct Serif-based or cross-doc-based individuals
	if (left_individual->_mention_uid != right_individual->_mention_uid ||
		left_individual->_entity_id != right_individual->_entity_id ||
		left_individual->_xdoc_cluster_id != right_individual->_xdoc_cluster_id ||
		left_individual->_event_id != right_individual->_event_id)
		return false;

	// Second, let's see if we have name/desc mention overlap
	if (left_individual->_name_or_desc.get() != NULL && right_individual->_name_or_desc.get() != NULL)
		// If the names don't match, then the individuals don't match
		if (!ElfString::are_coreferent(doc_theory, left_individual->_name_or_desc, right_individual->_name_or_desc))
			return false;

	// Third, let's make sure we have at least one overlapping type assertion
	bool overlapping_type = false;
	bool all_done1 = false;
	BOOST_FOREACH(ElfType_ptr left_type, left_individual->_types) {
		if (all_done1) continue;
		bool all_done2 = false;
		BOOST_FOREACH(ElfType_ptr right_type, right_individual->_types) {
			if (all_done2) continue;
			// Only allow exact ontology type matches (no inheritance)
			if (left_type->get_value() == right_type->get_value() && ElfString::are_coreferent(doc_theory, left_type, right_type)) {
				overlapping_type = true;
				all_done2 = true; // can't brk in a BOOST_FOREACH
			}
		}
		if (overlapping_type)
			all_done1 = true; // can't brk in a BOOST_FOREACH
	}

	// Match if there was a type overlap
	return overlapping_type;
}

/**
 * Compares *this to another ElfIndividual.
 *
 *
 * @param other An ElfIndividual to be compared to *this.
 * @return -1, 0, or 1, depending on whether *this <, ==, or > other,
 * respectively
 *
 * @author afrankel@bbn.com
 * @date 2011.04.21
 **/
int ElfIndividual::compare(const ElfIndividual & other) const {
	// First, make sure that we're not going to dereference a null pointer.
	int name_or_desc_deep_diff = 0;
	if (_name_or_desc == NULL && other._name_or_desc != NULL) {
		return -1;
	} else if (other._name_or_desc == NULL && _name_or_desc != NULL) {
		return 1;
	} else if (_name_or_desc != NULL && other._name_or_desc != NULL) {
		// Instantiate an ElfString_less_than functor object, which we'll use
		// to compare the _name_or_desc fields of the two strings (by passing them
		// as arguments to the class's "()" operator). 
		ElfString_less_than obj; 
		if (obj(_name_or_desc, other._name_or_desc)) {
			name_or_desc_deep_diff = -1;
		} else if (obj(other._name_or_desc, _name_or_desc)) {
			name_or_desc_deep_diff = 1;
		} else {
			name_or_desc_deep_diff = 0;
		}
	}
	if (name_or_desc_deep_diff == 0) {
		return lexicographicallyCompareTypeSets(other);
	} else {
		return name_or_desc_deep_diff;
	}
}

/**
 * Compares two sets lexicographically, stepping through the items in
 * sort order until finding a mismatch (lower key qualifies as "less-than") or the end of a
 * collection (shorter collection wins). Returns -1 if t0 < t1, 0 if t0 == t1,
 * +1 if t0 > t1.
 *
 * @param set0 First set.
 * @param set1 Second set.
 *
 * @author afrankel@bbn.com
 * @date 2011.05.10
 **/
int ElfIndividual::lexicographicallyCompareTypeSets(const ElfIndividual & other) const {
	// The algorithm std::lexicographical_compare() almost gives us what we want,
	// but (despite its name) it returns a bool rather than an int, so we'd have to call 
	// it twice to get a {-1, 0, 1} return value.
	ElfTypeSet::const_iterator iter0 = _types.begin();
	ElfTypeSet::const_iterator iter1 = other._types.begin();
	static ElfString_less_than cmp;
	while (true) {
		if (iter0 == _types.end()) {
			if (iter1 == other._types.end()) {
				return 0; // the sets have the same content and the same length
			} else {
				return -1; // first set is shorter
			} 
		} else if (iter1 == other._types.end()) {
			return 1; // second set is shorter
		} else if (cmp(*iter0, *iter1)) {
			return -1;
		} else if (cmp(*iter1, *iter0)) {
			return 1;
		} else {
			++iter0;
			++iter1;
		}
	}
}

/**
 * Dump the fields for the individual, then dump all args that are
 * contained by the relation.
 * @param out The std::ostream to which output is to be written.
 * @param indent The number of levels of indentation (one level = two spaces; default = 0)
 * at which to dump the output for the relation. 
 **/
void ElfIndividual::dump(std::ostream &out, int indent /* = 0 */) const {
	std::wstring spaces(indent, L' ');
	std::wstring spaces_plus_2(indent + 2, L' ');
	out << spaces;
	out << "ElfIndividual: name_or_desc: <" << UnicodeUtil::toUTF8StdString(_name_or_desc->get_string());
	out << ">; best uri: <" << get_best_uri();
	out << ">; types:" << std::endl;
	BOOST_FOREACH(ElfType_ptr type, _types) {
		type->dump(out, indent+2);
	}
	out << std::endl;
}


/**
 * Static utility method to generate a namespace-prefixed
 * string identifier from a prefix and some documents.
 * Generated IDs are still only document-unique, but allow
 * us to mix ElfIndividuals.
 *
 * @param docid The unique document identifier, making this
 * generated ID globally unique.
 * @param start The start offset of the ElfIndividual
 * we're generating an ID for.
 * @param end The end offsets of the ElfIndividual
 * we're generating an ID for.
 *
 * @return The bbn-namespace entity ID string
 **/
std::wstring ElfIndividual::generate_uri_from_offsets(const std::wstring & docid, EDTOffset start, EDTOffset end) {
	// Create a prefixed string ID
	std::wstringstream uri;
	uri << "bbn:individual-" << docid << "-" << start.value() << "-" << end.value();
	return uri.str();
}
/**
 * Returns a string processed according to type. In most cases, this will
 * correspond to the original string, but for xsd:int, it will correspond
 * to a string from which any thousand separators have been stripped. Other
 * special cases may be added. This is a static class method, originally
 * implemented in ElfRelationArg.
 *
 * @param original_text The original text (e.g., "Ohio" or "5,000").
 * @return The processed text (e.g., "Ohio" or "5000").
 *
 * @author afrankel@bbn.com
 * @date 2010.08.04
 **/
std::wstring ElfIndividual::get_text_according_to_type(const std::wstring & type_string, const std::wstring & original_text) {
	if (type_string == L"xsd:int") {
		return get_normalized_int_text(original_text);
	} else if (type_string == L"xsd:date") {
        // Even though xsd:date supports times and timezones, we only care about day resolution
		std::wstring ymd_only_text = TimexUtils::toYearMonthDayOnly(original_text);
		if (ymd_only_text == L"") {
			std::stringstream error;
			error << "ElfIndividual::get_text_according_to_type(std::wstring, std::wstring, DocTheory*): No YYYY-MM-DD in xsd:date '" << original_text << "'";
			throw std::runtime_error(error.str().c_str());
		} else {
			return ymd_only_text;
		}
	} else {
		// Additional cases requiring special handling may be added in additional "else if"
		// branches above.
		return original_text;
	}
}

/**
 * Strip every character that is not a minus sign or a digit. Used to produce
 * a value string that represents an xsd:int. This is a static class method,
 * originally implemented in ElfRelationArg.
 *
 * @param original_text The original text (e.g., "5,000").
 * @return The text with everything but minus sign and digits removed (e.g., "5000").
 *
 * @author afrankel@bbn.com
 * @date 2010.07.26
 **/
std::wstring ElfIndividual::get_normalized_int_text(const std::wstring & original_text) {
	// Check for valid input string
	if (original_text.empty()) {
		std::stringstream error;
		error << "ElfIndividual::get_normalized_int_text(std::wstring): Empty input text";
		throw std::runtime_error(error.str().c_str());
	}

	// Convert presumed number text to lowercase and parse it into an integer
	std::wstring lowercased_text(original_text);
	std::transform(lowercased_text.begin(), lowercased_text.end(), lowercased_text.begin(), towlower);
	int return_int = ElfIndividual::number_converter->convertNumberWord(lowercased_text);

	// Check the returned integer (note that convertNumberWord() returns zero if input is not a number word)
	if (return_int > 0) {
		// Return the cleaned integer as a string
		return boost::lexical_cast<std::wstring>(return_int);
	} else if (original_text == L"zero" || original_text == L"0") {
		return L"0";
	} else {
		std::stringstream error;
		error << "ElfIndividual::get_normalized_int_text(std::wstring): Not an integer '" << original_text << "'";
		throw std::runtime_error(error.str().c_str());
	}
}

std::string ElfIndividual::toDebugString(int indent) const {
	std::ostringstream ostr;
	dump(ostr, indent);
	return ostr.str();
}
