// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef REFERENCE_RESOLVER_H
#define REFERENCE_RESOLVER_H

#include "Generic/common/GrowableArray.h"
#include "Generic/common/DebugStream.h"
#include "Generic/edt/LinkerTreeSearch.h"
#include "Generic/edt/StatNameLinker.h"
#include "Generic/edt/RuleNameLinker.h"
#include "Generic/edt/RuleDescLinker.h"
#include "Generic/edt/StatDescLinker.h"
#include "Generic/edt/PronounLinker.h"
#include "Generic/edt/LexEntitySet.h"
#include "Generic/edt/LexDataCache.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
class PartOfSpeechSequence;
class DocumentMentionInformationMapper;

class ReferenceResolver {
public:
	ReferenceResolver(int beam_width = 0);
	~ReferenceResolver();
	void cleanup();
	void setBeamWidth(int beam_width);
	void cleanUpAfterDocument();
	void resetForNewDocument(DocTheory *docTheory);
	void resetForNewSentence(DocTheory *docTheory, int sentence_num);
	void resetWithPrevEntitySet(EntitySet *lastEntitySet, Parse* prevParse = 0);
	void addPartOfSpeechTheory(const PartOfSpeechSequence* pos);
	void resetSearch(const MentionSet *mentionSet);
	// This does the work. It fills in the array of pointers to EntitySets
	// where specified by results arg, and returns its size. The maximum
	// number of desired EntitySets is given by max_num_theories. It returns
	// 0 if something goes wrong. The client is responsible both for 
	// allocating and deleting the array of EntitySet pointers; the client is also
	// responsible for deleting the EntitySets themselves.
	int getEntityTheories(EntitySet *results[],
						  const int max_results,
						  const Parse *parse,
						  const MentionSet *mentionSet,
						  const PropositionSet *propSet);

	int addNamesToEntitySet(EntitySet *results[],
						  const int max_results,
						  const Parse *parse,
						  const MentionSet *mentionSet,
						  const PropositionSet *propSet);
	void postLinkSpecialNameCases(EntitySet* results[], int nsets);
	int addDescToEntitySet(EntitySet *results[],
						  const int max_results,
						  const Parse *parse,
						  const MentionSet *mentionSet,
						  const PropositionSet *propSet);
	int readdSingletonDescToEntitySet(EntitySet *results[],
						  const int max_results,
						  const Parse *parse,
						  const MentionSet *mentionSet,
						  const PropositionSet *propSet,
						  int overgen, double me_threshold);
	int addPronToEntitySet(EntitySet *results[],
						  const int max_results,
						  const Parse *parse,
						  const MentionSet *mentionSet,
						  const PropositionSet *propSet);

	//void linkPronouns(GrowableArray <Mention *> pronouns, EntitySet *results[], int nResults);

	static void setDo2PSpeakerMode(bool mode);
	static bool getDo2PSpeakerMode();

private:
	LexDataCache _lexDataCache;
	LexEntitySet *_prevSet;
	LinkerTreeSearch _searcher;
	DocTheory *_docTheory;

	// this pointer will be manipulated as appropriate 
	// -- either set to a RuleNameLinker or a StatNameLinker object
	MentionLinker* _nameLinker;
	// for high precision, don't use the desc linker.
	// determine this by use-desc-linker parameter
	bool _useDescLinker;
	MentionLinker* _descLinker;

	//MentionLinker *_descLinkerTwo; // JJO 09 Aug 2011 - word inclusion

	PronounLinker* _pronounLinker;

	bool _create_partitive_entities;

	// If true (or unspecified), link first and second person singular/plural pronouns
	bool _link_first_and_second_person_pronouns;

	bool _use_itea_linking;


	int _beam_width;
	int _nSentences;
	static DebugStream &_debugOut;
	//static DebugStream &_pronDebugOut;
	DocumentMentionInformationMapper *_infoMap;

	bool _use_correct_answers;

	typedef std::map<int, const Mention*> MentionMap;
	void linkAllLinkablePrelinks(MentionMap &preLinks);
	void linkAllLinkablePrelinks(MentionMap &preLinks, const MentionSet* prevMentSet);
	/** Carry out linking as determined by PreLinker.
	  * If final is true, then create new entities for mentions marked
	  * as pre-linked, but which cannot be linked. */
	void linkPreLinks(LexEntitySet *lexEntitySet,
					  MentionMap &preLinks, bool final);
	void linkPreLinks(LexEntitySet *lexEntitySet, const MentionSet * prevMentSet, MentionMap &preLinks, bool final);
	void add1p2pPronouns(LexEntitySet *lexEntitySet, const MentionSet * mentionSet, MentionMap &preLinks);
	void add1p2pPronounsToSpeakers(LexEntitySet *lexEntitySet, const MentionSet *mentionSet, MentionMap &preLinks);

	void addPartitives(LexEntitySet *lexEntitySet, const MentionSet *mentionSet);
	bool bothAreNames(const Mention *ment1, const Mention *ment2);
};

#endif
