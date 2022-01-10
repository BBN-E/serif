/**
 * Parallel implementation of ElfRelation object based on Python
 * implementation in ELF.py. Uses Xerces-C++ for XML writing.
 *
 * @file ElfRelation.h
 * @author nward@bbn.com
 * @date 2010.06.16
 **/

#pragma once

#include "LearnIt/MatchInfo.h"
#include "ElfIndividual.h" // for ElfIndividualMap
#include "ElfRelationArg.h" // for ElfRelationArgMap
#include "PredFinder/common/ElfTemplate.h"
#include "Generic/common/bsp_declare.h"

#include "xercesc/dom/DOM.hpp"

#include <string>
#include <vector>
#include "boost/make_shared.hpp"

// Forward declaration to use shared pointer for class in class
class DocTheory;
class SentenceTheory;
BSP_DECLARE(ElfRelation);
BSP_DECLARE(PatternSet);
BSP_DECLARE(PatternFeatureSet);
BSP_DECLARE(Pattern);

/**
 * Contains one relation, which may have any number of arguments.
 * Used to convert a MatchInfo::PatternMatch to a <relation> XML
 * element.
 *
 * @author nward@bbn.com
 * @date 2010.05.14
 **/
class ElfRelation {
public:
	ElfRelation(const DocTheory* doc_theory, const SentenceTheory* sent_theory,
		const Target_ptr target, const MatchInfo::PatternMatch& match, 
		double confidence, bool learnit2 = false);
	ElfRelation(const DocTheory* doc_theory, const PatternSet_ptr pattern, const PatternFeatureSet_ptr feature_set);
	ElfRelation(const ElfRelation_ptr other);
	ElfRelation(const DocTheory* doc_theory, const ElfRelation_ptr other);
	ElfRelation(const xercesc::DOMElement* relation, const ElfIndividualSet & individuals, bool exclusive_end_offsets = false);
	ElfRelation(const std::wstring & name, const std::vector<ElfRelationArg_ptr> & arguments, 
		const std::wstring & text, EDTOffset start, EDTOffset end, double confidence, int score_group);

	std::vector<ElfRelationArg_ptr> get_args(void) const;
	ElfRelationArg_ptr get_arg(const std::wstring & role) const;
	std::vector<ElfRelationArg_ptr>  get_args_with_role(const std::wstring & role) const;

	/**
	 * Inlined accessor to the relation's args.
	 *
	 * @return The map in _args.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.29
	 **/
	ElfRelationArgMap get_arg_map(void) const { return _arg_map; }

	/**
	 * Inlined accessor to the relation's name.
	 *
	 * @return The value of _name.
	 *
	 * @author nward@bbn.com
	 * @date 2010.08.23
	 **/
	std::wstring get_name(void) const { return _name; }

	/**
	 * Inlined mutator for the relation's name.
	 *
	 * @param name The new value of _name.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.13
	 **/
	void set_name(const std::wstring & name) { _name = name; }

	/**
	 * Inlined accessor to the relation's optional source.
	 *
	 * @return The value of _source.
	 *
	 * @author nward@bbn.com
	 * @date 2010.09.01
	 **/
	std::wstring get_source(void) const { return _source; }

	/**
	 * Inlined mutator for the relation's optional source.
	 *
	 * @param source The new value for _source.
	 *
	 * @author nward@bbn.com
	 * @date 2010.09.01
	 **/
	void set_source(const std::wstring & source) { _source = source; }

	/**
	 * Inlined mutator for the relation's optional source.
	 *
	 * @param source The new value to extend _source.
	 *
	 * @author nward@bbn.com
	 * @date 2010.11.05
	 **/
	void add_source(const std::wstring & source) { _source == L"" ? _source = source : _source = _source + L";" + source; }

	/**
	 * Inlined accessor to the relation's optional text.
	 *
	 * @return The value of _text.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.18
	 **/
	std::wstring get_text(void) const { return _text; }

	/**
	 * Inlined convenience method to get the number of different
	 * roles with arguments specified for a relation. It's possible
	 * for a relation to have multiple arguments with the same
	 * role, though, so this isn't necessarily the same as 
	 * get_args().size() .
	 *
	 * @return the size of _args
	 *
	 * @author rgabbard@bbn.com
	 * @date 2010.12.17
	 **/
	size_t arity(void) const { return _arg_map.size(); }

	/**
	 * Inlined accessor to the relation's start offset, if any.
	 *
	 * @return The value of _start, which might be -1.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.18
	 **/
	EDTOffset get_start(void) const { return _start; }

	/**
	 * Inlined accessor to the relations's end offset, if any.
	 *
	 * @return The value of _end, which might be -1.
	 *
	 * @author nward@bbn.com
	 * @date 2010.10.18
	 **/
	EDTOffset get_end(void) const { return _end; }

	std::vector<ElfIndividual_ptr> get_referenced_individuals(void) const;

	ElfRelationArg_ptr get_matching_arg(const ElfRelationArg_ptr search_arg) const;

	double get_confidence() const { return _confidence; }
	int get_score_group() const { return _score_group; }

	bool offsetless_equals(const ElfRelation_ptr other);
	int compare(const ElfRelation & other) const;
	bool less_than(const ElfRelation & other) const {
		return (compare(other) < 0);}

	xercesc::DOMElement* to_xml(xercesc::DOMDocument* doc, const std::wstring & docid) const;

	void dump(std::ostream &out, int indent = 0) const;
	std::string toDebugString(int indent) const; // calls dump()

	ElfRelationArg_ptr insert_argument(const ElfRelationArg_ptr arg);
	void remove_argument(const ElfRelationArg_ptr arg);

	static const double DEFAULT_CONFIDENCE;

private:
	int lexicographicallyCompareArgMapsByKey(const ElfRelation & other) const;
	/** 
	 *  Functions for "TBD" events (complex events whose event and 
	 *  argument types depend on the whole package).
     **/
	void fix_tbd_arguments(const DocTheory *doc_theory, 
		ReturnPatternFeature_ptr tbdEventFeature, 
		std::vector<ReturnPatternFeature_ptr>& tbdAgentFeatures,
		std::vector<ReturnPatternFeature_ptr>& tbdPatientFeatures);

	/**
	 * A (possibly namespace-prefixed) name for the relation
	 * which could be an ontology predicate or an internal
	 * pattern name.
	 **/
	std::wstring _name;

	/**
	 * Starting Serif character offset into the source document.
	 * The document starting at this offset should contain the
	 * arg _text. Optional, but must be paired with _end.
	 * Typically the start of the maximum span of the arguments.
	 **/
	EDTOffset _start;

	/**
	 * Ending Serif character offset into the source document.
	 * The document ending at this offset should contain the
	 * arg _text. Optional, but must be paired with _start.
	 * Typically the end of the maximum span of the arguments.
	 **/
	EDTOffset _end;

	/**
	 * Document substring between _start and _end. Optional.
	 **/
	std::wstring _text;

	/**
	 * Confidence value for this relation, probably the confidence
	 * value for the pattern that produced it. Optional.
	 **/
	double _confidence;

	/**
	 * Pattern that generated this relation, intended for debugging
	 * and provenance tracking. Optional.
	 **/
	std::wstring _source;

	/**
	 * Score group value for this relation, probably from
	 * the pattern that produced it. Optional.
	 **/
	int _score_group;

	/**
	 * All of the arguments of this relation, stored by role name.
	 * See definition of ElfRelationArgMap for more information.
	 **/
	ElfRelationArgMap _arg_map;
};

/**
 * Defines a simple string to ElfRelation dictionary
 * of lists, intended for use in an ElfIndividualClusterMember
 * for collecting the relations relevant to a particular
 * individual. Keys should be the name of the relation. Actual declaration is in ContainerTypes.h.
 *
 * @author nward@bbn.com
 * @date 2010.08.24
 **/
//typedef std::map<std::wstring, std::vector<ElfRelation_ptr>> ElfRelationMap;

size_t hash_value(ElfRelation_ptr const& relation);

/**
 * Comparison callable so we can be sure of getting a stable sort.
 *
 * @author afrankel@bbn.com
 * @date 2011.04.21
 **/
struct ElfRelation_less_than {
	bool operator()(const ElfRelation_ptr s1, const ElfRelation_ptr s2) const
	{
		if (s1 == NULL) {
			if (s2 != NULL) {
				return true;
			} else {
				return false;
			}
		} else if (s2 == NULL) {
			return false;
		} else {
			return (s1->less_than(*s2));
		}
	}
};

struct ElfRelation_name_less_than {
	bool operator()(const ElfRelation_ptr s1, const ElfRelation_ptr s2) const
	{
		if (s1 == NULL) {
			if (s2 != NULL) {
				return true;
			} else {
				return false;
			}
		} else if (s2 == NULL) {
			return false;
		} else {
			return (s1->get_name() < s2->get_name());
		}
	}
};


// Technically, all sets are sorted, but here we specify the sorting criterion.
typedef std::set< ElfRelation_ptr, ElfRelation_less_than >ElfRelationSortedSet;

typedef std::multiset< ElfRelation_ptr, ElfRelation_name_less_than >ElfRelationMultisetSortedByName;
