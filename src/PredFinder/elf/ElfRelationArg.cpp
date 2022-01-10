/**
 * Parallel implementation of ElfRelationArg object based on Python
 * implementation in ELF.py. Uses Xerces-C++ for XML writing.
 *
 * @file ElfRelationArg.cpp
 * @author nward@bbn.com
 * @date 2010.06.16
 **/
#pragma warning(disable:4996)

#include "Generic/common/leak_detection.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/common/ParamReader.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/theories/Value.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"
#include "Generic/values/TemporalNormalizer.h"
#include "Generic/common/NationalityRecognizer.h"
#include "Generic/common/WordConstants.h"
#include "LearnIt/MainUtilities.h"
#include "PredFinder/common/ElfMultiDoc.h"
#include "PredFinder/common/SXMLUtil.h"
#include "LearnIt/SlotFiller.h"
#include "PredFinder/macros/ReadingMacroSet.h"
#include "ElfName.h"
#include "ElfDescriptor.h"
#include "ElfType.h"
#include "ElfIndividualFactory.h"
#include "ElfRelationArg.h"
#include "boost/make_shared.hpp"
#include "boost/lexical_cast.hpp"
#pragma warning(push, 0)
#include "boost/regex.hpp"
#pragma warning(pop)
#include "boost/foreach.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/convenience.hpp"
#include <stdexcept>

using boost::dynamic_pointer_cast;

XERCES_CPP_NAMESPACE_USE

bool ElfRelationArg::_show_id = true;

/**
 * Dump output at specified level of indentation.
 *
 * @param out The stream to which the output should be written.
 * @param indent The indentation level (one level = one space; default = 0)
 * at which the output should be written.
 **/
void ElfRelationArg::dump(std::ostream &out, int indent /*= 0 */) const {
	std::wstring spaces(indent, L' ');
	ElfType_ptr type = get_type();
	out << spaces << "ElfRelationArg: role: <" << _role << ">; type: <" << type->get_value();
	if (!type->get_string().empty()) {
		out << ", " << type->get_string();
	}
	out << ">" << endl;
	// TODO: Control the information printed out with a parameter (debug_level) rather than comment it out.
	//out << spaces << "value: " << _value << endl;
	//if (_individual) {
	//	out << spaces << "individual: " << _individual->get_name_or_desc() << endl;
	//}
	//out << spaces << "start: " << _start << endl;
	//out << spaces << "end: " << _end << endl;
	//out << spaces << "text: " << _text << endl;
}

/**
 * Dump arg to a single string.
 *
 * @param get_individual Determines whether individual should be written.
 * @return (Wide) string containing output.
 **/
std::wstring ElfRelationArg::to_dbg_string(bool get_individual) const {
	std::wostringstream strstream;
	strstream  << "role: " << _role << L"\n";
	ElfType_ptr type = get_type();
	strstream  << "type: " << type->get_value()<<"\n";
		//L"\ntype_string: "<< type->get_string()<< L"\n";
	if(get_individual && _individual.get() != NULL){
		if(_individual->has_entity_id()){
			strstream<<"individual_txt: "<<_individual->get_name_or_desc()->get_string()<<L"\n";
		}
		else
			strstream<<"individual: NON-ENTITY";
		strstream<<"individual_id: "<<_individual->get_best_uri()<<L"\n";		
	}
	return strstream.str();
}

/**
 * Compare this ElfRelationArg to a reference ElfRelationArg.
 *
 * @param other Reference ElfRelationArg.
 * @return -1, 0, 1 depending on whether this element is less than, equal to, or greater than the reference element.
 **/
int ElfRelationArg::compare(const ElfRelationArg & other) const {
	int role_diff = _role.compare(other._role);
	if (role_diff == 0) {
		int text_diff = _text.compare(other._text);
		if (text_diff == 0) {
			int start_diff = _start < other._start ? -1 : (_start > other._start ? 1 : 0);
			if (start_diff == 0) {
				int end_diff = _end < other._end ? -1 : (_end > other._end ? 1 : 0);
				if (end_diff == 0) {
					ElfIndividual_less_than ei_obj;
					int indiv_diff = ei_obj(_individual, other._individual) ? -1 : (ei_obj(other._individual, _individual) ? 1 : 0);
					return indiv_diff;
				} else {
					return end_diff;
				}
			} else {
				return start_diff;
			}
		} else {
			return text_diff;
		}
	} else {
		return role_diff;
	}
}
bool ElfRelationArg::isSameAndContains(const  ElfRelationArg_ptr other, const DocTheory* docTheory) const{
	if(_role != other->_role)
		return false;
	if(_individual->get_best_uri() != other->_individual->get_best_uri())
		return false;
	if(_individual->has_mention_uid() && other->_individual->has_mention_uid()){
		const Mention* thisMention = docTheory->getEntitySet()->getMention(_individual->get_mention_uid()); 
		const Mention* otherMention = docTheory->getEntitySet()->getMention(other->_individual->get_mention_uid()); 
		if(thisMention->getUID() == otherMention->getUID()){
			//std::wcout<<"Same Mention: "<<thisMention->getNode()->toTextString()<<" COVERS: "<<"\t"<<otherMention->getNode()->toTextString()<<std::endl;
			return true;
		}
		if(thisMention->getSentenceNumber() != otherMention->getSentenceNumber())
			return false;
		const Entity* thisEntity = docTheory->getEntityByMention(thisMention);
		const Entity* otherEntity = docTheory->getEntityByMention(otherMention);
		if (!thisEntity || !otherEntity) {
			return false;
		}
		if(thisEntity->getID() != otherEntity->getID())
			return false;
		if(thisMention->getMentionType() == Mention::APPO){
			if(otherMention->getMentionType() == Mention::NAME)
				if(thisMention->getNode()->isAncestorOf(otherMention->getNode())){
					return true;
				}
		}
		else if(thisMention->getMentionType() == Mention::NAME){
			if(thisMention->getNode()->isAncestorOf(otherMention->getNode())){
					return true;
			}
		}
		else if(thisMention->getMentionType() == Mention::DESC 
			&& otherMention->getMentionType() != Mention::NAME){
				if(thisMention->getNode()->isAncestorOf(otherMention->getNode())){
					return true;
				}
		}
		return false;
	}
	if(_individual->has_value_mention_id() && other->_individual->has_value_mention_id()){
		if(_individual->has_value_mention_id() == other->_individual->has_value_mention_id())
			return true;
		return false;
	}
	if(_individual->has_date_range() && other->_individual->has_date_range()){
		if((_individual->get_date_range().first == other->_individual->get_date_range().first) &&
			(_individual->get_date_range().second == other->_individual->get_date_range().second))
		{
			return true;
		}
		return false;
	}
	return false;
}

/**
 * Checks whether the individual associated with this argument has the
 * desired type.

 * @param type_string The search type URI.
 * @return True if the individual associated with this argument has the
 * same type as type_string, false otherwise.
 **/
bool ElfRelationArg::individual_has_type(const wstring& type_string) {
	if (_individual.get() != NULL) {
		return _individual->has_type(type_string);
	}
	return false;
}

/**
 * Hash implementation for ElfRelationArg shared_ptrs
 * that will be used by boost::hash to generate unique
 * IDs but also by various keyed containers. Parallel to
 * the __hash__ implementation in ELF.py.
 *
 * @param arg The pointer referencing the argument
 * to be hashed.
 * @return An integer hash of the role, type, offsets,
 * and value/id.
 **/
size_t hash_value(ElfRelationArg_ptr const& arg) {
	// Hash in the role, type and start and end offsets
	boost::hash<std::wstring> string_hasher;
	boost::hash<int> int_hasher;
	size_t arg_hash = string_hasher(arg->get_role());
	ElfType_ptr type = arg->get_type();
	if (type.get() != NULL) {
		arg_hash ^= string_hasher(type->get_value());
	}
	arg_hash ^= int_hasher(arg->get_start().value());
	arg_hash ^= int_hasher(arg->get_end().value());

	// Hash in the best URI or value
	ElfIndividual_ptr individual = arg->get_individual();
	if (individual.get() != NULL) {
		if (individual->has_value()) {
			arg_hash ^= string_hasher(individual->get_value());
		} else {
			arg_hash ^= string_hasher(individual->get_best_uri());
		}
	}

	// Done
	return arg_hash;
}

/**
 * Construct an argument from a slot of a particular match.
 *
 * @param doc_theory The containing document.
 * @param constraints A SlotConstraints obtained from a
 * particular Pattern Target, in a boost::shared_ptr.
 * @param slot The actual SlotFiller obtained from a
 * particular MatchInfo::PatternMatch, in a boost::shared_ptr.
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
ElfRelationArg::ElfRelationArg(const DocTheory* doc_theory, const SlotConstraints_ptr constraints, const SlotFiller_ptr slot) {
	// Check for bad slot or document
	std::stringstream error_msg;
	static const std::string method_name("ElfRelationArg::ElfRelationArg(DocTheory*, SlotConstraints_ptr, SlotFiller_ptr): ");
	std::string details;
	if (slot.get() == NULL) {
		details = "Slot null";
	} else if (doc_theory == NULL) {
		details = "Document theory null";
	} else if (slot->getType() == SlotConstraints::UNFILLED_OPTIONAL) {
		// Special slot type found
		//   This check needs to happen first, since the SlotFiller
		//   might be incomplete (most likely, a NULL SentenceTheory)
		// Skip this optional arg
		details = "Slot optional";
	} else if (slot->getType() == SlotConstraints::UNMET_CONSTRAINTS) {
		// Slot constraints not met, so we don't consider this a match for our purposes
		details = "Slot constraints unmet";
	}
	if (!details.empty()) {
		ostringstream out;
		out << method_name << details;
		if (slot.get() != NULL && slot->getOriginalText() != L"") {
			out << "; text:\n" << slot->getOriginalText();
		}
		throw std::runtime_error(out.str().c_str());
	}

	// Get the argument properties from the slot constraints and filler
	_role = constraints->getELFRole();
	_start = slot->getStartOffset();
	_end = slot->getEndOffset();
	_text = slot->getOriginalText();

	// Create a type from the slot constraints associated with this argument's offsets
	std::wstring type_string = constraints->getELFOntologyType();

	// Check if we're dealing with a mention or a value slot
	if (slot->getType() == SlotConstraints::MENTION) {
		// Create an ElfIndividual from this mention match
		_individual = boost::make_shared<ElfIndividual>(doc_theory, type_string, slot->getMention());
	} else if (slot->getType() == SlotConstraints::VALUE) {
		// Check if we have an underlying ValueMention
		const ValueMention* value_mention = slot->getValueMention();
		if (value_mention) {
			// Create the value from the mention
			_individual = boost::make_shared<ElfIndividual>(doc_theory, type_string, value_mention);
		} else {
			// Just use the filtered slot text as the value, even though it's redundant with itself
			_individual = boost::make_shared<ElfIndividual>(type_string, _text, _text, _start, _end);
		}
	}
}

/**
 * Construct an argument from a ReturnPatternFeature.
 *
 * @param feature The actual ReturnPatternFeature_ptr.
 * @param doc_theory The containing document.
 *
 * @author nward@bbn.com
 * @date 2010.06.21
 **/
ElfRelationArg::ElfRelationArg(const DocTheory* doc_theory, ReturnPatternFeature_ptr feature) : _text(L""), _start(-1), _end(-1) {
	// Check for bad feature or document
	static const std::string method_name("ElfRelationArg::ElfRelationArg(DocTheory*, ReturnPatternFeature_ptr)");
	std::string exception_text(method_name + ": ");
	if (feature == NULL) {
		exception_text += "Feature null";
		throw std::runtime_error(exception_text.c_str());
	}
	if (doc_theory == NULL) {
		exception_text += "Document theory null";
		throw std::runtime_error(exception_text.c_str());
	}

	// Get the role from the return feature
	if (feature->hasReturnValue(L"role"))
		_role = feature->getReturnValue(L"role");
	else {
		ostringstream ostr;
		if(feature->getPattern())
			feature->getPattern()->dump(ostr, /*indent=*/0);
		if (feature->getPatternReturn()) {
			feature->getPatternReturn()->dump(ostr, /*indent=*/4);
		}
		exception_text += "No role return specified in manual pattern.\n";
		exception_text += ostr.str();
		throw std::runtime_error(exception_text.c_str());
	}

	// Get a value string from the sentence
	SentenceTheory* sent_theory = doc_theory->getSentenceTheory(feature->getSentenceNumber());
	if (sent_theory) {
		TokenSequence* token_sequence = sent_theory->getTokenSequence();
		if (token_sequence) {
			// Get the offsets and text for the feature (presumably after setCoverage() has been called)
			_start = token_sequence->getToken(feature->getStartToken())->getStartEDTOffset();
			_end = token_sequence->getToken(feature->getEndToken())->getEndEDTOffset();

			// Get the evidence string directly from the document using the feature's offsets
			LocatedString* text = MainUtilities::substringFromEdtOffsets(doc_theory->getDocument()->getOriginalText(), _start, _end);
			_text = std::wstring(text->toString());
			delete text;

			// Check if this is a value or individual
			if (feature->hasReturnValue(L"value")) {
				// Check if we're using the document value or a specified one
				std::wstring return_value = feature->getReturnValue(L"value");
				std::wstring return_type = feature->getReturnValue(L"type");
				if (return_value == L"true" && return_type != L"xsd:boolean") {
					// Check if we have an underlying ValueMention
					const ValueMentionReturnPFeature_ptr vmf = dynamic_pointer_cast<ValueMentionReturnPFeature>(feature);
					const DateSpecReturnPFeature_ptr dsf = dynamic_pointer_cast<DateSpecReturnPFeature>(feature);	
					if (vmf) {
						// Use the value mention text and offsets directly, with filtering if necessary
						_individual = boost::make_shared<ElfIndividual>(doc_theory, return_type, vmf->getValueMention());
						_text = _individual->get_name_or_desc()->get_string();
						_individual->get_name_or_desc()->get_offsets(_start, _end);
					} 
					else if(dsf){ 
						_individual = boost::make_shared<ElfIndividual>(doc_theory, feature->getReturnValue(L"type"), dsf->getDateSpecString(), 
							dsf->getValueMention1(), dsf->getValueMention2());
						_text = _individual->get_name_or_desc()->get_string();
						_individual->get_name_or_desc()->get_offsets(_start, _end);
					}
					else {
						// Check if this is a special <arg> that extracts the document date
						if (_role == L"eru:docDate") {
							std::wstring doc_date = L"";
							std::wstring doc_additional_time = L"";
							ElfMultiDoc::temporal_normalizer->normalizeDocumentDate(doc_theory, doc_date, doc_additional_time);
							if (doc_date != L"" && doc_date != L"XXXX-XX-XX") {
								// We found a good document date, so we don't need any provenance
								_individual = boost::make_shared<ElfIndividual>(return_type, doc_date);
								_text = L"";
								_start = EDTOffset();
								_end = EDTOffset();
							} else {
								exception_text += "docDate role specified, "
									"but could not get normalized document date";
								throw std::runtime_error(exception_text.c_str());
							}
						} else if (return_type == L"nfl:NFLTeam") {
							// Perform team mapping, this must be some kind of value-based regex match to work around Serif metonymy misses
							XDocIdType xdoc_id;
							std::wstring bound_uri;
							std::wstring best_uri = ElfMultiDoc::get_mapped_arg(_text, return_type, doc_theory, NULL, bound_uri, xdoc_id);

							// Generate a globally-unique ID from the offsets
							std::wstring docid = std::wstring(doc_theory->getDocument()->getName().to_string());
							std::wstring generated_uri = ElfIndividual::generate_uri_from_offsets(docid, _start, _end);

							// Construct the team individual manually
							ElfString_ptr name = boost::make_shared<ElfName>(_text, 1.0, _text, _start, _end);
							ElfType_ptr type = boost::make_shared<ElfType>(return_type, _text, _start, _end);
							_individual = boost::make_shared<ElfIndividual>(generated_uri, name, type);
							_individual->set_bound_uri(bound_uri);
						} else {
							// Filter and use the value specified by the feature text
							_individual = boost::make_shared<ElfIndividual>(return_type, _text, _text, _start, _end);
						}
					}
				} else {
					// Use the xsd:boolean value specified in the pattern's return constraint
					_individual = boost::make_shared<ElfIndividual>(return_type, return_value, _text, _start, _end);
				}
			} else {
				// Check if we have an underlying ValueMention, even though we want to treat this as an individual
				const ValueMentionReturnPFeature_ptr vmf = dynamic_pointer_cast<ValueMentionReturnPFeature>(feature);
				const DateSpecReturnPFeature_ptr dsf = dynamic_pointer_cast<DateSpecReturnPFeature>(feature);
				if (vmf) {
					// Use the value mention text and offsets directly, with filtering if necessary
					_individual = boost::make_shared<ElfIndividual>(doc_theory, feature->getReturnValue(L"type"), vmf->getValueMention());
					_text = _individual->get_name_or_desc()->get_string();
					_individual->get_name_or_desc()->get_offsets(_start, _end);
				} 
				else if(dsf){ 
					_individual = boost::make_shared<ElfIndividual>(doc_theory, feature->getReturnValue(L"type"), dsf->getDateSpecString(), 
						dsf->getValueMention1(), dsf->getValueMention2());
					_text = dsf->getDateSpecString();
					_individual->get_name_or_desc()->get_offsets(_start, _end);
				}
				else {
					// Default to using an individual
					_individual = boost::make_shared<ElfIndividual>(doc_theory, feature);

					// Get the argument-local individual's type text and offsets and use them for the arg as well
					_individual->get_type()->get_offsets(_start, _end);
					_text = _individual->get_type()->get_string();
				}
			}
		}
	}
}

/**
 * Copy constructor.
 *
 * @param other ElfRelationArg to deep copy from.
 *
 * @author nward@bbn.com
 * @date 2010.08.18
 **/
ElfRelationArg::ElfRelationArg(ElfRelationArg_ptr other) {
	// Copy the other arg's role, value, text, and offsets
	_role = other->_role;
	_text = other->_text;
	_start = other->_start;
	_end = other->_end;

	// Deep copy the other arg's individual, if any
	if (other->_individual.get() != NULL) {
		// Copy as a value or an individual as appropriate
		if (other->_individual->has_value())
			_individual = boost::make_shared<ElfIndividual>(boost::static_pointer_cast<ElfIndividual>(other->_individual));
		else
			_individual = boost::make_shared<ElfIndividual>(other->_individual);
	}
}

/**
 * Constructor for creating an anonymous relation arg
 * from an individual; intended for use in search utility
 * methods. Assumes that the input individual has one type
 * that should become the arg's type (or none if unspecified).
 *
 * @param role The desired role of the new arg.
 * @param individual The individual being copied into the new arg.
 *
 * @author nward@bbn.com
 * @date 2010.08.18
 **/
ElfRelationArg::ElfRelationArg(const std::wstring & role, const ElfIndividual_ptr individual) {
	// Store the role and individual directly
	_role = role;
	_individual = boost::make_shared<ElfIndividual>(individual);

	// If we're creating an rdf:subject when generating R-ELF, use name provenance
	ElfString_ptr provenance;
	if (_role == L"rdf:subject")
		provenance = individual->get_name_or_desc();
	else if (_role != L"")
		provenance = individual->get_type();

	// Copy the name/type provenance from the individual
	if (provenance.get() != NULL) {
		// Get the associated strings and offsets
		EDTOffset start, end;
		provenance->get_offsets(start, end);
		std::wstring text_evidence = provenance->get_string();

		// Store the offsets and evidence for the type with this arg
		_start = start;
		_end = end;
		_text = text_evidence;
	}
}

/**
 * Constructor for creating a relation arg containing an ElfIndividual.
 * For convenience, the arg's ElfType, the ElfIndividual, and the arg
 * itself all get the same text and offsets. Generally easier than
 * creating a value first and then passing it in.
 *
 * @param role The desired role of the new arg.
 * @param type_string The string URI of the type of the new value arg.
 * @param value The actual value string (possibly a URI).
 * @param text An optional text evidence for the new arg.
 * @param start An optional start offset for the text evidence.
 * @param end An optional end offset for the text evidence.
 *
 * @author nward@bbn.com
 * @date 2011.05.09
 **/
ElfRelationArg::ElfRelationArg(const std::wstring & role, const std::wstring & type_string, const std::wstring & value, const std::wstring & text, EDTOffset start, EDTOffset end) {
	// Store the role and offsets directly
	_role = role;
	_text = text;
	_start = start;
	_end = end;

	// Create the new value
	_individual = boost::make_shared<ElfIndividual>(type_string, value, text, start, end);
}

/**
 * Reads this ElfRelationArg from an XML <arg> element.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param arg The DOMElement containing a <arg>.
 * @param individuals The read <individual>s from this <arg>'s
 * containing document, used for id lookup.
 *
 * @author nward@bbn.com
 * @date 2010.08.31
 **/
ElfRelationArg::ElfRelationArg(const DOMElement* arg, const ElfIndividualSet & individuals, bool exclusive_end_offsets) {
	// Read the required arg attributes
	_role = SXMLUtil::getAttributeAsStdWString(arg, "role");
	if (_role == L"")
		throw std::runtime_error("ElfRelationArg::ElfRelationArg(DOMElement*, ElfIndividualMap): "
			"No role attribute specified on <arg>");

	// Correct some bad UW arg roles
	if (_role == L"eru:personHasAgeInYears")
		_role = L"eru:ageOfPersonInYears";
	else if (boost::starts_with(_role, L"eru:t:")) {
		// Temporary hack to handle bad temporal type namespace from UW
		boost::replace_first(_role, L"eru:", L"");
	}

	// Read the mutually exclusive arg attributes
	std::wstring value = SXMLUtil::getAttributeAsStdWString(arg, "value");
	std::wstring id = SXMLUtil::getAttributeAsStdWString(arg, "id");
	if ((id != L"" && value != L"") || (id == L"" && value == L"")) {
		throw std::runtime_error("ElfRelationArg::ElfRelationArg(DOMElement*, ElfIndividualMap): "
			"Invalid id/value attributes on <arg>; must specify one but not both");
	}

	// Correct UW's use of value/ID
	if (value.length() > 0 && value.find(L"-GI-") != std::wstring::npos) {
		// Swap the URI into the ID instead of the value
		id = value;
		value = L"";
	}
	// Read the optional arg attributes
	std::wstring start = SXMLUtil::getAttributeAsStdWString(arg, "start");
	if (start != L"") {
		_start = EDTOffset(boost::lexical_cast<int>(start));
	} else {
		_start = EDTOffset();
	}
	std::wstring end = SXMLUtil::getAttributeAsStdWString(arg, "end");
	if (end != L"") {
		_end = EDTOffset(boost::lexical_cast<int>(end));
	} else {
		_end = EDTOffset();
	}

	// Adjust the offsets if necessary
	if (exclusive_end_offsets && _end.is_defined())
		--_end;

	// Get the element's text content, if any
	_text = SerifXML::transcodeToStdWString(arg->getTextContent());

	// Check the validity of the offsets and text
	if ((!_start.is_defined() && _end.is_defined()) || (_start.is_defined() && !_end.is_defined())) {
		throw std::runtime_error("ElfRelationArg::ElfRelationArg(DOMElement*, ElfIndividualMap): "
			"Invalid offset attributes on <arg>; specify both or neither");
	} else if ((_start.is_defined() && _end.is_defined() && _text == L"") || 
		(!_start.is_defined() && !_end.is_defined() && _text != L"")) {
		throw std::runtime_error("ElfRelationArg::ElfRelationArg(DOMElement*, ElfIndividualMap): "
			"<arg> must specify both offset attributes and text content or neither");
	}

	// Get the element's type attribute, if any
	std::wstring type_string = SXMLUtil::getAttributeAsStdWString(arg, "type");
	if (type_string == L"") {
		// UW didn't specify types on some literals, so infer them from the role
		if (_role == L"eru:count" || _role == L"eru:int")
			type_string = L"xsd:int";
		else if (_role == L"eru:date")
			type_string = L"xsd:date";
	} else {
		// Temporary hack to handle bad type namespace from UW
		boost::replace_first(type_string, L"eru:", ReadingMacroSet::domain_prefix + L":");
	}

	// Load the value, if any
	if (value != L"") {
		// Check the value to make sure we have an associated type
		if (!boost::regex_match(value, boost::basic_regex<wchar_t>(L"\\w+:\\S+")) && 
			!(value.length() >= 7 && value.substr(0, 7) == L"http://") && type_string == L"") {
			throw std::runtime_error("ElfRelationArg::ElfRelationArg(DOMElement*, ElfIndividualMap): "
				"Non-URI value specified on <arg> with no associated type attribute");
		}

		// Create a new value individual associated with this <arg>
		_individual = boost::make_shared<ElfIndividual>(type_string, value, _text, _start, _end);
	}

	// Look up the document individual by ID, if any
	if (id != L"") {
		// Find a matching individual by URI
		ElfIndividual_ptr matched_individual;
		bool all_done = false;
		BOOST_FOREACH(ElfIndividual_ptr individual, individuals) {
			if (all_done) continue;
			// We've already converted individual IDs to URIs
			std::wstring best_uri = individual->get_best_uri();
			if (best_uri == id || best_uri.substr(best_uri.find_first_of(L":") + 1) == id || best_uri.substr(best_uri.find_last_of(L"-") + 1) == id) {
				matched_individual = individual;
				all_done = true; // can't brk in a BOOST_FOREACH
			}
		}
		if (matched_individual.get() != NULL) {
			// Copy the appropriate provenance of the referenced individual
			_individual = boost::make_shared<ElfIndividual>(matched_individual, type_string, _start, _end);
		} else {
			// Bad individual reference
			throw std::runtime_error("ElfRelationArg::ElfRelationArg(DOMElement*, ElfIndividualMap): "
				"Individual specified on <arg> not found in <doc>");
		}
	}

	// Check for malformed UW dates that were treated as individuals
	if (id != L"" && type_string == L"xsd:string")
		// Turn the individual associated with this <arg> into a value
		_individual = boost::make_shared<ElfIndividual>(type_string, _text, _text, _start, _end);
}

const std::wstring& ElfRelationArg::get_text() const {
	return _text;
}

/**
 * Mutator to the argument's underlying individual.
 * If the new individual is non-null, clear out the _value too.
 * A copy of the input individual is always made.
 *
 * @param individual The new individual being associated with this arg.
 *
 * @author nward@bbn.com
 * @date 2010.08.23
 **/
void ElfRelationArg::set_individual(ElfIndividual_ptr individual) {
	if (individual.get() != NULL) {
		// Copy as a value or an individual as appropriate
		if (individual->has_value())
			_individual = boost::make_shared<ElfIndividual>(boost::static_pointer_cast<ElfIndividual>(individual));
		else
			_individual = boost::make_shared<ElfIndividual>(individual);
	} else {
		// Store the null shared pointer (overwriting what was there)
		_individual = individual;
	}
}

/**
 * Equality operation for arguments. Compares role and id/value,
 * but ignores type and offsets.
 *
 * @param other ElfRelationArg to compare with.
 * @param check_roles Whether roles should be compared; default true. 
 *
 * @author nward@bbn.com
 * @date 2010.08.19
 **/
bool ElfRelationArg::offsetless_equals(const ElfRelationArg_ptr other, bool check_roles) {
	// Make sure the roles match if specified
	if (check_roles && _role != other->_role)
		return false;

	// Check if we're need to compare individuals
	if (_individual.get() != NULL && other->_individual.get() != NULL) {
		// Check if we're comparing values instead of individuals
		if (_individual->has_value() && other->_individual->has_value()) {
			// Check if values match
			if (_individual->get_value() == other->_individual->get_value()) {
				// Match
				return true;
			}
		} else if (!_individual->has_value() && !other->_individual->has_value()) {
			// Check if individual IDs match
			if (_individual->get_best_uri() == other->_individual->get_best_uri()) {
				// Match
				return true;
			}
		}
	}

	// No match
	return false;
}

/**
 * Converts this argument to an XML <arg> element using the
 * Xerces-C++ library.
 *
 * XMLPlatformUtils::Initialize() must be called before this method.
 *
 * @param doc An already-instantiated Xerces DOMDocument that
 * provides namespace context for the created element (since
 * Xerces doesn't support easy anonymous element import).
 * @return The constructed <arg> DOMElement
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
DOMElement* ElfRelationArg::to_xml(DOMDocument* doc, const std::wstring & docid, bool suppress_type) const {
	// Create a new argument element
	DOMElement* arg = SXMLUtil::createElement(doc, "arg");

	// Check what type of ELF we're producing
	std::wstring contents = SXMLUtil::getAttributeAsStdWString(doc->getDocumentElement(), "contents");

	// Copy the argument's properties in as attributes of the <arg>
	SXMLUtil::setAttributeFromStdWString(arg, "role", _role);
	if (_individual.get() != NULL) {
		// Assign value or id attribute as appropriate
		if (_individual->has_value()) {
			SXMLUtil::setAttributeFromStdWString(arg, "value", _individual->get_value());
		} else {
			if (contents == L"R-ELF") {
				SXMLUtil::setAttributeFromStdWString(arg, "value", _individual->get_best_uri(docid));
			} else {
				if (_show_id) {
					SXMLUtil::setAttributeFromStdWString(arg, "id", _individual->get_best_uri(docid));
				}
			}
		}

		if (_individual->has_mention_confidence()) {
			SXMLUtil::setAttributeFromStdWString(arg, "confidence", _individual->get_mention_confidence().toString());
		}

		// Assign type attribute based on individual, but not in some cases
		if (!suppress_type)
			SXMLUtil::setAttributeFromStdWString(arg, "type", _individual->get_type()->get_value());
	}
	if (_start.is_defined() && _end.is_defined()) {
		SXMLUtil::setAttributeFromEDTOffset(arg, "start", _start);
		SXMLUtil::setAttributeFromEDTOffset(arg, "end", _end);
	}

	// Optionally print the text evidence for the argument
	if (ParamReader::isParamTrue("elf_include_text_excerpts")) {
        SerifXML::xstring x_text = SerifXML::transcodeToXString(_text.c_str());
		DOMText* arg_cdata = doc->createTextNode(x_text.c_str());
		arg->appendChild(arg_cdata);
	}

	// Done
	return arg;
}
