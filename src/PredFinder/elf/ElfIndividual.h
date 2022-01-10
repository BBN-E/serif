/**
 * Parallel implementation of ElfIndividual object based on Python
 * implementation in ELF.py; extended significantly to interact with
 * Serif mentions and entities. Uses Xerces-C++ for XML writing.
 *
 * @file ElfIndividual.h
 * @author nward@bbn.com
 * @date 2010.06.23
 **/

#pragma once

#include "Generic/theories/DocTheory.h"
#include "Generic/common/Attribute.h"
//#include "distill-generic/features/SnippetReturnFeature.h"
#include "ElfString.h"
#include "ElfName.h"
#include "ElfType.h"
#include "PredFinder/common/ContainerTypes.h"
#include "PredFinder/common/ElfTemplate.h"

#include "xercesc/dom/DOM.hpp"
#include "boost/make_shared.hpp"
#pragma warning(push, 0)
#include "boost/regex.hpp"
#pragma warning(pop)
#include <map>

class ValueMention;
BSP_DECLARE(NumberConverter);
BSP_DECLARE(ElfIndividual);
BSP_DECLARE(ReturnPatternFeature);

/**
 * Convenience macro for selecting the only unique
 * ID out of a multiset, if there is one, or returning
 * a default value. That is, the multiset should contain
 * 1 or more instances of the same member value.
 *
 * @param SET The std::multiset being checked
 * @param DEFAULT The value to return if there is no match,
 * which should be of the same type as the SET.
 *
 * @author nward@bbn.com
 * @date 2011.06.10
 **/
#define GET_UNIQUE_ID(SET, DEFAULT) (SET.size() >= 1 && SET.count(*(SET.begin())) == SET.size()) ? *(SET.begin()) : DEFAULT;

/**
 * Contains one individual for a document. Used to convert
 * a Serif Entity or Event, or possibly a non-ACE individual,
 * to an <individual> XML element. Associated
 * with ElfRelationArg, but output at the ElfDocument level.
 *
 * Individuals associated with <arg>s have a single local <type>,
 * but document-level merged individuals collect all of those at
 * serialization. We store all possible IDs for an individual and
 * use the most general one when relevant.
 *
 * @author nward@bbn.com
 * @date 2010.06.23
 **/
class ElfIndividual {
protected:
	// Default initializer
	void initialize(void);

public:
	// Copy/merge constructors
	ElfIndividual(const ElfIndividual_ptr other);
	ElfIndividual(const DocTheory* doc_theory, const ElfIndividual_ptr other);
	ElfIndividual(const ElfIndividual_ptr other, const std::wstring & type_string, EDTOffset start, EDTOffset end);
	ElfIndividual(const ElfIndividualSet others);

	// Individual constructors
	ElfIndividual(const std::wstring & uri, const ElfString_ptr string = ElfName_ptr(), const ElfType_ptr type = ElfType_ptr());
	ElfIndividual(const DocTheory* doc_theory, ReturnPatternFeature_ptr feature);
	ElfIndividual(const DocTheory* doc_theory, const std::wstring & type_string, const Mention* mention);
	ElfIndividual(const xercesc::DOMElement* individual, const std::wstring & docid, const std::wstring & document_source, bool exclusive_end_offsets = false);

	// Value constructors
	ElfIndividual(const DocTheory* doc_theory, const std::wstring & type_string, const ValueMention* value_mention);
	ElfIndividual(const std::wstring & type_string, const std::wstring & value, const std::wstring & text = L"", EDTOffset start = EDTOffset(), EDTOffset end = EDTOffset());
	ElfIndividual(const DocTheory* doc_theory, const std::wstring & type_string, const std::wstring & value, const ValueMention* v1, const ValueMention* v2);

	std::wstring get_partner_uri(void) const { return _partner_uri; }
	bool has_partner_uri(void) const { return _partner_uri != L""; }

	std::wstring get_generated_uri(void) const { return _generated_uri; }
	bool has_generated_uri(void) const { return _generated_uri != L""; }
	void set_generated_uri(std::wstring generated_uri) { _generated_uri = generated_uri; }

	std::wstring get_value(void) const { return _value; }
	bool has_value(void) const { return _is_value; }
	void set_value(std::wstring value) { _is_value = true; _value = value; }

	int get_value_mention_id(void) const { return _value_mention_id; }
	bool has_value_mention_id(void) const { return _value_mention_id != -1; }
	
	std::pair<int, int> get_date_range(void) const {return _date_range;}
	bool has_date_range(void) const {return (_date_range.first != -1 || _date_range.second != -1);}

	MentionUID get_mention_uid(void) const { return _mention_uid; }
	bool has_mention_uid(void) const { return _mention_uid.isValid(); }

	MentionConfidenceAttribute get_mention_confidence(void) const { return _mention_confidence; }
	bool has_mention_confidence(void) const { return _mention_confidence != MentionConfidenceStatus::UNKNOWN_CONFIDENCE; }

	int get_entity_id(void) const { return _entity_id; }
	bool has_entity_id(void) const { return _entity_id != -1; }

	std::wstring get_coref_uri(void) const { return _coref_uri; }
	bool has_coref_uri(void) const { return _coref_uri != L""; }
	void set_coref_uri(std::wstring coref_uri) { _coref_uri = coref_uri; }

	int get_xdoc_cluster_id(void) const { return _xdoc_cluster_id; }
	bool has_xdoc_cluster_id(void) const { return _xdoc_cluster_id != -1; }

	int get_event_id(void) const { return _event_id; }
	bool has_event_id(void) const { return _event_id != -1; }

	std::wstring get_bound_uri(void) const { return _bound_uri; }
	bool has_bound_uri(void) const { return _bound_uri != L""; }
	void set_bound_uri(std::wstring bound_uri) { _bound_uri = bound_uri; }

	std::wstring get_best_uri(const std::wstring & docid = L"") const;

	bool has_equal_ids(const ElfIndividual_ptr other) const;

	/**
	 * Inlined accessor to get the individual's name ElfString.
	 *
	 * @return This individual's _name_or_desc.
	 *
	 * @author nward@bbn.com
	 * @date 2010.06.23
	 **/
	const ElfString_ptr get_name_or_desc(void) const { return _name_or_desc; }

	/**
	 * Inlined accessor to retrieve the only ElfType in
	 * an individual's type set, intended for use in an
	 * ElfRelationArg context.
	 *
	 * @return The first an only ElfType in _types; throws
	 * an exception if there is not one type.
	 *
	 * @author nward@bbn.com
	 * @date 2011.05.27
	 **/
	ElfType_ptr get_type(void) const { if (_types.size() == 1) { return *(_types.begin()); } else { throw UnrecoverableException("ElfIndividual::get_type()", "Individual doesn't have a single type."); } }

	/**
	 * Inlined mutator that completely replaces an
	 * individual's types with the one specified. Intended
	 * for use by various inference methods.
	 *
	 * @param type The new ElfType that will be the only item
	 * in this individual's _types ElfTypeSet.
	 *
	 * @author nward@bbn.com
	 * @date 2011.07.11
	 **/
	void set_type(ElfType_ptr type) { _types.clear(); _types.insert(type); }

	//void add_type(const ElfType_ptr new_type);
	bool has_type(const std::wstring & type_string) const;
	std::multiset<std::wstring> get_type_strings(void) const;

	static std::wstring generate_uri_from_offsets(const std::wstring &docid, EDTOffset start, EDTOffset end);

	void get_spanning_offsets(EDTOffset & start, EDTOffset & end) const;

	int compare(const ElfIndividual & other) const;
	bool less_than(const ElfIndividual & other) const {
		return (compare(other) < 0);}

	xercesc::DOMElement* to_xml(xercesc::DOMDocument* doc, const std::wstring & docid) const;

	static bool are_coreferent(const DocTheory* doc_theory, const ElfIndividual_ptr left_individual, 
		const ElfIndividual_ptr right_individual);
	void dump(std::ostream &out, int indent = 0) const;
	std::string toDebugString(int indent) const; // calls dump()

	// can be useful to turn off IDs to simplify diffs for debugging
	static void set_show_id(bool show_id) {_show_id = show_id;} 

	static NumberConverter_ptr number_converter;

private:
	static std::wstring get_text_according_to_type(const std::wstring & type_string, const std::wstring & original_text);
	static std::wstring get_normalized_int_text(const std::wstring & original_text);

	static const boost::wregex individual_id_re;

protected:
	int lexicographicallyCompareTypeSets(const ElfIndividual & other) const;

protected:
	/**
	 * The globally unique URI for this individual as
	 * specified in input partner S-ELF before being merged
	 * with Serif mentions. Only used if there's no
	 * mapping to a more general URI.
	 **/
	std::wstring _partner_uri;

	/**
	 * A globally unique identifier for this individual,
	 * typically containing a source URI prefix, the
	 * document ID, and either an integer offset pair
	 * or a hash value based on the individual's source
	 * context.
	 *
	 * This URI is typically used when no others are
	 * available, or for generated individuals like
	 * NFLGame clusters or intermediate Date/Count objects.
	 **/
	std::wstring _generated_uri;

	/**
	 * A flag indicating whether we expect this ElfIndividual
	 * to contain a literal value or an individual with some URI.
	 **/
	bool _is_value;

	/**
	 * The literal's value string (which may contain a formatted
	 * data type). This should never be a URI.
	 **/
	std::wstring _value;

	/**
	 * The underlying Serif value mention UID, if found.
	 *
	 * Used to generate a URI for values that were found from
	 * a Serif ValueMention.
	 **/
	int _value_mention_id;

	/**
	 * The underlying Serif mention UID, if found/aligned.
	 *
	 * Used to generate a URI for some partner individuals
	 * that get mapped, and for some types that shouldn't
	 * rely on Serif's entity coreference, such as Position.
	 **/
	MentionUID _mention_uid;

	/**
	 * The underlying Serif mention confidence, if found/aligned.
	 **/
	MentionConfidenceAttribute _mention_confidence;

	/**
	 * The underlying Serif entity ID, if found/aligned.
	 *
	 * Used to generate a URI for individuals of most types,
	 * if they couldn't be mapped to a more general cross-doc
	 * or bound URI.
	 **/
	int _entity_id;

	/**
	 * A within-document coreference URI, generated by some
	 * non-Serif coreference process, such as NFLGame clustering
	 * or Blitz inference. Expected to contain the document ID,
	 * so it will be globally unique. Not used for most types.
	 **/
	std::wstring _coref_uri;

	/**
	 * A cross-document coreference cluster ID, if mapped
	 * by string-based lookup or Serif entity alignment.
	 *
	 * Used to generate a URI for individuals of most types,
	 * if they couldn't be mapped to a more general bound
	 * URI.
	 **/
	int _xdoc_cluster_id;

	/**
	 * The underlying Serif event ID, if found/aligned.
	 *
	 * Used to generate a URI for exact ACE event matches,
	 * which is fairly rare. Mutually exclusive with
	 * _mention_id, _entity_id, and _xdoc_cluster_id.
	 **/
	int _event_id;

	/**
	 * A bound URI, if mapped by string-based lookup or
	 * cross-doc entity alignment.
	 *
	 * Used for individuals of most types, unless there
	 * is a lookup type conflict (such as when a Position
	 * is based on a PER mention that maps to a bound URI).
	 **/
	std::wstring _bound_uri;

	/**
	 * The best name or descriptor, based on the underlying
	 * Serif entity or cross-doc entity's best name when
	 * available.
	 **/
	ElfString_ptr _name_or_desc;

	/**
	 * A set of types, whose uniqueness is determined by
	 * their type strings. Unmerged individuals associated
	 * with ElfRelationArgs should have only a single type.
	 **/
	ElfTypeSet _types;

	/**
	 * It can be useful to turn off IDs to simplify sorting for debugging purposes.
	 **/
	static bool _show_id;

	std::pair<int, int> _date_range;
};
