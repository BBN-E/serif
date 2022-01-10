// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#ifndef MATCH_INFO_H
#define MATCH_INFO_H

#include "boost/shared_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "Generic/common/bsp_declare.h"
#include <sstream>
#include <map>
#include <set>
#include "Generic/patterns/multilingual/LanguageVariant.h"
#include "Generic/patterns/multilingual/AlignmentTable.h"
#include "Generic/patterns/multilingual/AlignedDocSet.h"
#include "LearnIt/MatchProvenance.h"

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)

#include "LearnIt/pb/instances.pb.h"

#pragma warning(pop)

class SentenceTheory;  // Defined in "Generic/theories/SentenceTheory.h"
class DocTheory;    
BSP_DECLARE(SlotFiller)      // Defined in "LearnIt/SlotFiller.h"
BSP_DECLARE(LearnItPattern)	   // Defined in "learint/LearnItPattern.h"
class Proposition;     // Defined in "Generic/theories/Proposition.h"
class Argument;        // Defined in "Generic/theories/Argument.h"
class RelEvMatch;
BSP_DECLARE(Target)
BSP_DECLARE(SlotPairConstraints)

/** A utility (all-static) class whose methods extract information about 
  * the places where seeds or patterns match in a document, and record that
  * information using XML strings.
  *
  * The SeedMatchFinder and InstanceFinder binaries use these methods to
  * record information about the seeds and patterns that they find. */
class MatchInfo: boost::noncopyable {
public:

	/** Return an XML string that records the SERIF analysis of a single
	  * sentence.  This analysis is recorded using the SERIF state
	  * saver (Generic/state/StateSaver.h) in a google protocol buffer. */
	static void sentenceTheoryInfo(learnit::SentenceTheory* sentTheoryOut,
		const DocTheory* doc, const SentenceTheory *sentTheory, int sent_index, bool include_state=true);

	static void setNameSpanForMention(const Mention* m, const DocTheory* doc, learnit::NameSpan* nameSpan);

	/** Return an XML string describing a given slot filler.  
	  * "doc_info" and "sentTheory" are the document and sentence that
	  * contain the slot filler.  "target" and "slot" specify what 
	  * concept or relation the slot filler belongs to; and which slot
	  * of that concept/relation it fills.  "slot_filler" is the slot
	  * filler itself. */
	static void slotFillerInfo(learnit::SlotFiller* outSlotFiller,
		const SentenceTheory *sentTheory, Target_ptr target, 
		SlotFiller_ptr slot_filler, unsigned int slot_num, std::wstring seedString=L"");
	static void slotFillerInfo(learnit::SlotFiller* outSlotFiller,
		const SentenceTheory *sentTheory, Target_ptr target, 
		SlotFiller_ptr slot_filler, unsigned int slot_num, std::wstring seedString,
		const std::vector<RelEvMatch>& relationMatches,
		const std::vector<RelEvMatch>& eventMatches);

	/** Return an XML string describing the set of propositions that connect
	  * one or more slot fillers. */
	static void propInfo(learnit::SeedInstanceMatch* out, 
		const SentenceTheory *sentTheory, Target_ptr target,
		std::map<int, SlotFiller_ptr> slotFillers, bool lexicalize = false);


	struct PatternMatch {
		std::vector<SlotFiller_ptr> slots;
		int start_token;
		int end_token;
		double score;
		std::wstring source;
		MatchProvenance_ptr provenance;

		PatternMatch(const std::vector<SlotFiller_ptr> &slots, int start_token, 
			int end_token, double score = 0.0, std::wstring source = L""):
		slots(slots), start_token(start_token), end_token(end_token), 
		score(score), source(source) {}
	};
	
	typedef std::vector<PatternMatch> PatternMatches;

	static PatternMatches findPatternInSentence(const DocTheory* dt, 
		const SentenceTheory *sent_theory, LearnItPattern_ptr pattern) ;
	static PatternMatches findPatternInSentence(const AlignedDocSet_ptr doc_set, 
		const SentenceTheory *sent_theory, LearnItPattern_ptr pattern) ;

private:
	// Data structure to store paths from propositions to slot fillers:
	struct PropPathLink {
		PropPathLink(const Proposition *prop, std::wstring role, bool end): prop(prop), role(role), end(end) {}
		std::wstring role; //The role the proposition plays for its parent
		const Proposition *prop; //The proposition object itself
		bool end; //Whether or not this is a leaf (important since now leaves can be lex)
	};
	typedef std::vector<PropPathLink> PropPath;
	typedef std::map<int, PropPath> SlotPathMap;
	static void propInfo(
		learnit::SeedInstanceMatch* out, 
		const SentenceTheory *sentTheory, 
		Proposition *prop, Target_ptr target,
		std::map<int, SlotFiller_ptr> slotFillers, bool lexicalize = false);

	static void findPropPaths(
		const SentenceTheory *sentTheory, const Proposition *prop, 
		SlotFiller_ptr slotFiller, const PropPath &pathToHere,
		std::vector<PropPath> &result);

	static bool _satisfiesSlotPairConstraints(
		const std::vector<SlotFiller_ptr>& slot_fillers,
		const std::vector<SlotPairConstraints_ptr>& constraints);
	
	/**
	 * Given a vector of all the found 'PropPath's try to add lexical arguments
	 * to each path.
	**/
	static void lexicalizeCombinations(const SentenceTheory *sentTheory, std::vector<SlotPathMap> &thePaths);
	
	/**
	 * Given a Proposition pointer, gets an appropriate string value for it
	**/
	static std::wstring getLexValue(const Proposition *prop);

	static void findPropPathsViaArg(
		const SentenceTheory *sentTheory, const Proposition *prop, 
		Argument *arg, SlotFiller_ptr slotFiller, const PropPath &pathToHere,
		std::vector<PropPath> &result);

	static int writePropInfoFromSlotPaths(const std::map<int, PropPath> &slotPaths, 
		                                   learnit::SeedInstanceMatch* out, size_t depth=0);
	static void getPropStartTag(const Proposition *prop, learnit::Prop *out);

	static void eventsToXML(learnit::SentenceTheory* sentTheoryOut, const DocTheory* Dt, int sent_index);
	static void namesToXML(learnit::SentenceTheory* sentTheoryOut, const DocTheory* dt, int sent_index);

	static void printRelEvRoles(const std::vector<RelEvMatch>& matches,
		const std::wstring& attributeName, unsigned int slot, learnit::SlotFiller* out);
};

class RelEvMatch {
public:
	RelEvMatch(const std::wstring& type, 
		const std::vector<std::wstring>& roles, const std::wstring& anchor=L"");
	std::wstring type;
	std::vector<std::wstring> roles;
	std::wstring anchor;
};

#endif
