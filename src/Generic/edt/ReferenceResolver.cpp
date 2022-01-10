// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/HeapChecker.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/edt/ReferenceResolver.h"
#include "Generic/edt/OutsideDescLinker.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/edt/CorefUtilities.h"
#include "Generic/edt/discmodel/DTCorefLinker.h"
#include "Generic/edt/discmodel/DTNameLinker.h"
#include "Generic/edt/LinkGuess.h"
#include "Generic/edt/PreLinker.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Zone.h"
#include "Generic/theories/Zoning.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/reader/DefaultDocumentReader.h"

#include "Generic/common/SessionLogger.h"

DebugStream &ReferenceResolver::_debugOut = DebugStream::referenceResolverStream;
//DebugStream &ReferenceResolver::_pronDebugOut = PronounLinker::getDebugStream();

//#ifdef ENGLISH_LANGUAGE
//	#define DO_2P_SPEAKER_MODE true
//#elif defined ARABIC_LANGUAGE
//	#define DO_2P_SPEAKER_MODE true
//#else 
//	#define DO_2P_SPEAKER_MODE false
//#endif

ReferenceResolver::ReferenceResolver(int beam_width)
	: _lexDataCache(beam_width), _searcher(beam_width), _nameLinker(0),
	  _descLinker(0), _prevSet(0), _docTheory(0), _infoMap(0), // JJO 09 Aug 2011 - initialized _descLinkerTwo to null
	  _pronounLinker(PronounLinker::build())
{
	_use_correct_answers = ParamReader::isParamTrue("use_correct_answers");
	//initialize the AbbrevTable
	AbbrevTable::initialize();
	//initialize NameLinker, DescLinker, PronounLinker
	_beam_width = beam_width;
	_debugOut.init(Symbol(L"reference_resolver_debug_file"), false);
	_useDescLinker = true;
	// may be off, rule, or stat
	std::string linkMode = ParamReader::getRequiredParam("desc_link_mode");
	if (linkMode == "off")
		_useDescLinker = false;
	else if (linkMode == "rule")
		_descLinker = RuleDescLinker::build();
	else if (linkMode == "stat")
		_descLinker = _new StatDescLinker();
	else if (linkMode == "outside")
		_descLinker = _new OutsideDescLinker();
	else if (linkMode == "DT") {
		_descLinker = _new DTCorefLinker(); // JJO 09 Aug 2011 - added argument true (do word inclusion)
		//_descLinkerTwo = _new DTCorefLinker(); // JJO 09 Aug 2011 - word inclusion
	}
	else throw UnexpectedInputException(
		"ReferenceResolver::ReferenceResolver()",
		"Parameter 'desc_link_mode' must be set to 'off', 'rule, 'stat' or 'DT' - 'rule' is generally recommended");

	linkMode = ParamReader::getRequiredParam("name_link_mode");
	if (linkMode == "rule")
		_nameLinker = RuleNameLinker::build();
	else if (linkMode == "stat")
		_nameLinker = _new StatNameLinker();
	else if (linkMode == "DT") {
		_infoMap = _new DocumentMentionInformationMapper();
		_nameLinker = _new DTNameLinker(_infoMap);
	}
	else throw UnexpectedInputException(
		"ReferenceResolver::ReferenceResolver()",
		"Parameter 'name_link_mode' must be set to 'rule', 'stat' or 'DT' - 'stat' or 'DT' is recommended for balanced recall/precision");

	// SO: default == true
	PreLinker::setSpecialCaseLinkingSwitch(true);
	if (ParamReader::isParamTrue("turn_off_special_case_linking")) {
		PreLinker::setSpecialCaseLinkingSwitch(false);
	}

	// SO: default == false
	PreLinker::setEntitySubtypeFilteringSwitch(false);
	if (ParamReader::isParamTrue("do_coref_entity_subtype_filtering")) {
		PreLinker::setEntitySubtypeFilteringSwitch(true);
	}

	_create_partitive_entities = ParamReader::getRequiredTrueFalseParam("create_partitive_entities");

	// If true (or unspecified), link first and second person singular/plural pronouns
	_link_first_and_second_person_pronouns = ParamReader::getOptionalTrueFalseParamWithDefaultVal("link_first_and_second_person_pronouns", true);

	_use_itea_linking = ParamReader::isParamTrue("use_itea_linking");
}

ReferenceResolver::~ReferenceResolver() {
	delete _prevSet;
	delete _nameLinker;
	delete _descLinker;
	//delete _descLinkerTwo; // JJO 09 Aug 2011 - word inclusion
}

void ReferenceResolver::cleanup() {
	delete _prevSet;
	_prevSet = 0;
}


void ReferenceResolver::setBeamWidth(int beam_width) {
	_lexDataCache.setCapacity(beam_width);
	_searcher.setMaxLeaves(beam_width);
	_beam_width = beam_width;
}

void ReferenceResolver::cleanUpAfterDocument() {
	_pronounLinker->resetPreviousParses();
	_nameLinker->cleanUpAfterDocument();
	AbbrevTable::cleanUpAfterDocument();
	_lexDataCache.cleanup();
	if(_infoMap!=NULL)
		_infoMap->cleanUpAfterDocument();
}

void ReferenceResolver::resetForNewDocument(DocTheory *docTheory) {
	_nSentences = docTheory->getNSentences();
	if (_useDescLinker) {
		_descLinker->resetForNewDocument(docTheory->getDocument()->getName());
		_descLinker->resetForNewDocument(docTheory);
		/*
		// +++ JJO 09 Aug 2011 +++
		// Word inclusion
		_descLinkerTwo->resetForNewDocument(docTheory->getDocument()->getName());
		_descLinkerTwo->resetForNewDocument(docTheory);
		// --- JJO 09 Aug 2011 ---
		*/
	}
	_pronounLinker->resetForNewDocument(docTheory);
	_nameLinker->resetForNewDocument(docTheory->getDocument()->getName());
	_nameLinker->resetForNewDocument(docTheory);
	_docTheory = docTheory;
}

void ReferenceResolver::resetForNewSentence(DocTheory *docTheory, int sentence_num) {
	//set _prevSet and add last parse

	if (sentence_num == 0) {
		delete _prevSet; // SRS
		_prevSet = _new LexEntitySet(_nSentences);
	}
	else {
		//get previous lexical data from the cache.
		// this lexical data may be missing newly added entities
        SentenceTheory *lastTheory = docTheory->getSentenceTheory(sentence_num-1);
		EntitySet *lastEntitySet = lastTheory->getEntitySet();
		LexEntitySet::LexData *data = _lexDataCache.retrieveData(lastTheory->getEntitySet());
		while(data->lexEntities.length() < lastEntitySet->getNEntities())
			data->lexEntities.add(NULL);

		if(data == NULL) {
			throw InternalInconsistencyException("ReferenceResolver::resetForNewSentence()", "Lexical data not found.");
		}
		delete _prevSet; // SRS
		_prevSet = _new LexEntitySet(*lastEntitySet, *data);
		delete data;
		data = NULL;
		_pronounLinker->addPreviousParse(lastTheory->getPrimaryParse());
	}

	if (_useDescLinker) {
		_descLinker->resetForNewSentence();
		//_descLinkerTwo->resetForNewSentence(); // JJO 09 Aug 2011 - word inclusion
	}
}

void ReferenceResolver::resetSearch(const MentionSet *mentionSet) {
	LexEntitySet *newRoot = _new LexEntitySet(*_prevSet);
	newRoot->loadMentionSet(mentionSet);
	_searcher.resetSearch(newRoot);
}

int ReferenceResolver::getEntityTheories(EntitySet *results[],
										 const int max_results,
										 const Parse *parse,
										 const MentionSet *mentionSet,
										 const PropositionSet *propSet)
{
	_debugOut << "\n===========================BEGINNING OF SENTENCE==================";

	int nMentions = mentionSet->getNMentions();

	if (max_results != _beam_width)
		setBeamWidth(max_results);

	// set up pre-linked mention array
	std::map<int, const Mention*> preLinks;
	PreLinker::preLinkAppositives(preLinks, mentionSet);
	PreLinker::preLinkTitleAppositives(preLinks, mentionSet);
	PreLinker::preLinkCopulas(preLinks, mentionSet, propSet);
	PreLinker::preLinkSpecialCases(preLinks, mentionSet, propSet);

	//split mentions into three arrays: names, descriptors, pronouns
	//next, perform each of these three linking operations in turn
	
	resetSearch(mentionSet);
	//now split the mentions
	GrowableArray <MentionUID> names(nMentions);
	GrowableArray <MentionUID> descriptors(nMentions);
	GrowableArray <MentionUID> pronouns(nMentions);

	for (int j = 0; j < nMentions; j++) {
		Mention *thisMention = mentionSet->getMention(j);
		if(_infoMap!=NULL)
			_infoMap->addMentionInformation(thisMention);
		if (preLinks[j] == 0) {
			// categorize non-pre-linked mention
			if (thisMention->mentionType == Mention::NAME) {
				names.add(thisMention->getUID());
			} else if (_useDescLinker && thisMention->mentionType == Mention::DESC && thisMention->getEntityType().isRecognized()) {
				descriptors.add(thisMention->getUID());

			// For English, isLinkingPronoun returns true he/him/his/she/her/it/its/they/them/their/here/there and who/whom/which/whose/that/where
			} else if (thisMention->mentionType == Mention::PRON && WordConstants::isLinkingPronoun(thisMention->getNode()->getHeadWord())) {
				pronouns.add(thisMention->getUID());
			}
		}
	}


	//name linking
	_searcher.performSearch(names, mentionSet, *_nameLinker);

	//descriptor linking
	if (_useDescLinker) {
		linkAllLinkablePrelinks(preLinks);
		_searcher.performSearch(descriptors, mentionSet, *_descLinker);

		/*
		// +++ JJO 09 Aug 2011 +++
		// Word inclusion
		if (_descLinkerTwo != 0) {
			linkAllLinkablePrelinks(preLinks);		
			_searcher.performSearch(descriptors, mentionSet, *_descLinkerTwo);
		}
		// --- JJO 09 Aug 2011 ---
		*/
	}

	//pronoun linking
	linkAllLinkablePrelinks(preLinks);
	_searcher.performSearch(pronouns, mentionSet, *_pronounLinker);

	//retrieve theories and return
	//
	LexEntitySet **leaves = _new LexEntitySet *[max_results];
	int nLeaves = _searcher.removeLeaves(leaves, max_results);
	//the outside world only wants pure EntitySets
	//but we want to hold onto the lexical data
	//so: create pure EntitySet copies from the LexEntitySets,
	//	  extract the lexical data from the LexEntitySets,
	//	  THROW OUT the old LexEntitySet's, CACHE the LexData's,
	//    RETURN the EntitySet's
	for (int k = 0; k < nLeaves; k++) {
		// on these final theories we must carry out the pre-linking
		linkPreLinks(leaves[k], preLinks, true);

		// handle first- and second-person pronouns
		add1p2pPronouns(leaves[k],mentionSet, preLinks);

		// and don't forget about partitives, which all get their own entities
		if (_create_partitive_entities)
			addPartitives(leaves[k], mentionSet);


		results[k] = _new EntitySet(*leaves[k]);

		_lexDataCache.loadPair(results[k], leaves[k]->getLexData());
		//now the cache owns the LexData, so make sure it doesn't get deleted with the LexEntitySet
		leaves[k]->setLexData(NULL);
		delete leaves[k];
		leaves[k] = NULL;
	}
	delete[] leaves;

	return nLeaves;
}

void ReferenceResolver::linkAllLinkablePrelinks(ReferenceResolver::MentionMap &preLinks) {

	int n_leaves = _searcher.getNLeaves();
	
	for (int i = 0; i < n_leaves; i++) {
		LexEntitySet *lexEntitySet = _searcher.getLeaf(i);
		linkPreLinks(lexEntitySet, preLinks, false);
	}
}
void ReferenceResolver::linkAllLinkablePrelinks(ReferenceResolver::MentionMap &preLinks, const MentionSet* prevMentSet) {
	int n_leaves = _searcher.getNLeaves();
	
	for (int i = 0; i < n_leaves; i++) {
		LexEntitySet *lexEntitySet = _searcher.getLeaf(i);
		linkPreLinks(lexEntitySet,prevMentSet, preLinks, false);
	}
}

void ReferenceResolver::linkPreLinks(LexEntitySet *lexEntitySet, const MentionSet* prevMentSet,
									 ReferenceResolver::MentionMap &preLinks,
									 bool final)
{
	const MentionSet *mentionSet = prevMentSet;
	int n_mentions = mentionSet->getNMentions();

	/** Important Note: There are tricky things going on here with mentions and entity sets.
		It's important to remember that there is forking of entity sets during the linking process.
		This means that the mention pointed to by preLinks[i], which has ID XXX, may not be the same
		as the mention stored in the mentionSet with ID XXX. Specifically, they may have
		different entity types. So, you should really never do anything other than get the id 
		from preLinks[i]. We also get the mention type, which I assume is constant. But we should
		definitely not get the entity type, which is likely to have changed (for pronouns).
		**/

	for (int i = 0; i < n_mentions; i++) {
		if (preLinks[i] &&
			/* linking in things of type NONE is bad -- should be controlled for
			elsewhere, but let's at least try here -- EMB */
			mentionSet->getMention(i)->getMentionType() != Mention::NONE &&
			/* only link if the two mentions are not both names -- linking
			names is dangerous, and should only be attempted by the
			namelinker -- SRS */
			(!bothAreNames(mentionSet->getMention(i), preLinks[i]) || _use_itea_linking))
		{
			Mention *mention = mentionSet->getMention(i);

			/** Important Note: There are tricky things going on here with mentions and entity sets.
				It's important to remember that there is forking of entity sets during the linking process.
				This means that the mention pointed to by preLinks[i], which has ID XXX, may not be the same
				as the mention stored in the mentionSet with ID XXX. Specifically, they may have
				different entity types. So, let's make sure we use the new one going forward in this loop...
			**/
			Mention *preLinkMention = lexEntitySet->getMention(preLinks[i]->getUID());
			Entity *entity = lexEntitySet->getEntityByMention(preLinkMention->getUID());
			Entity *intendedEntity = preLinkMention->hasIntendedType() ? 0 :
					lexEntitySet->getIntendedEntityByMention(preLinkMention->getUID());

			
			if (entity) {

				// types match - add mention to entity
				if (mention->getEntityType() == entity->getType()) {
					lexEntitySet->add(mention->getUID(), entity->getID());
				}
				// mention type matches intended entity type - add mention to
				// intended entity
				else if (intendedEntity != 0 &&
						 mention->getEntityType() == intendedEntity->getType())
				{
					lexEntitySet->add(mention->getUID(), intendedEntity->getID());
				}
				// intended type of mention matches entity type - add mention
				// to entity
				else if (mention->hasIntendedType() &&
						 mention->getIntendedType() == entity->getType())
				{
					lexEntitySet->add(mention->getUID(), entity->getID());
				}
				// intended type of mention matches type of intended entity
				// -- add mention to intended entity
				else if (mention->hasIntendedType() && intendedEntity != 0 &&
						 mention->getIntendedType() == intendedEntity->getType())
				{
					lexEntitySet->add(mention->getUID(),
									  intendedEntity->getID());
				}
				// no matches - coerce mention type to that of entity
				else {
					mention->setEntityType(preLinkMention->getEntityType());					
					mention->setEntitySubtype(preLinkMention->getEntitySubtype());
					lexEntitySet->add(mention->getUID(), entity->getID());
				}

				// intended mention has not been assigned to an entity -- so
				// try to find one
				if (final &&
					mention->hasIntendedType() &&
					lexEntitySet->getIntendedEntityByMention(
											mention->getUID()) == 0)
				{
					if (mention->getIntendedType() == entity->getType()) {
						lexEntitySet->add(mention->getUID(), entity->getID());
					}
					else if (intendedEntity != 0 &&
							 mention->getIntendedType() ==
												intendedEntity->getType())
					{
						lexEntitySet->add(mention->getUID(),
										  intendedEntity->getID());
					}
					else {
						lexEntitySet->addNew(mention->getUID(),
											 mention->getIntendedType());
					}
				}

				// now make sure we don't try to link this again
				preLinks[i] = 0;
			}

			if (final) {
				// check to make sure both mention and intended mention have been added to
				// an entity. If not, create a new one.

				if (lexEntitySet->getEntityByMention(mention->getUID()) == 0) {
					lexEntitySet->addNew(mention->getUID(), mention->getEntityType());
				}

				if (mention->hasIntendedType() &&
					lexEntitySet->getIntendedEntityByMention(mention->getUID()) == 0)
				{
					lexEntitySet->addNew(mention->getUID(), mention->getIntendedType());
				}
			}
		}
	}
}

bool ReferenceResolver::bothAreNames(const Mention *ment1, const Mention *ment2) {
	return 
		ment1->getMentionType() == Mention::NAME &&
		ment2->getMentionType() == Mention::NAME;
}

void ReferenceResolver::linkPreLinks(LexEntitySet *lexEntitySet,
									 ReferenceResolver::MentionMap &preLinks,
									 bool final)
{
	const MentionSet *mentionSet = lexEntitySet->getLastMentionSet();
	linkPreLinks(lexEntitySet, mentionSet, preLinks, final);

}


void ReferenceResolver::add1p2pPronouns(LexEntitySet *lexEntitySet, const MentionSet* mentionSet, ReferenceResolver::MentionMap &preLinks) {

	// Exit immediately if we are not doing 1st/2nd person coref
	if (!_link_first_and_second_person_pronouns) { return; }

	bool speaker_mode = false;
	if (CorefUtilities::hasSpeakerSourceType(_docTheory)) {
		add1p2pPronounsToSpeakers(lexEntitySet, mentionSet, preLinks);
		speaker_mode = true;
	}
	//if you are doing global linking, you cant get the last mention set
	//const MentionSet *mentionSet = lexEntitySet->getLastMentionSet();
	int n_mentions = mentionSet->getNMentions();

	//Ideally, this parameter could be made more flexible than just "first entity" in the future.
	if (ParamReader::isParamTrue("link_1p_pronouns_to_first_entity") && lexEntitySet->getNEntities() > 0){
		Entity *first = lexEntitySet->getEntity(0);
		EntityType first_type = first->getType();

		for (int j = 0; j < n_mentions; j++) {
			Mention *mention = mentionSet->getMention(j);
			Symbol headword = mention->getNode()->getHeadWord();
			if (mention->getMentionType() == Mention::PRON && WordConstants::is1pPronoun(headword) &&
				mention->getEntityType() == first_type)
			{
				lexEntitySet->add(mention->getUID(), 0);
			}
		}
	}

	// find IDs of first-person & second-person pronoun entities from previous sentences
	int id_1p_sing = -1;
	int id_1p_per_plural = -1;
	int id_1p_org = -1;
	int id_1p_gpe = -1;
	int id_2p = -1;
	for (int i = 0; i < lexEntitySet->getNEntities(); i++) {
		Entity *entity = lexEntitySet->getEntity(i);
		if(entity->getNMentions() <=0)
			continue;
		Mention *mention = lexEntitySet->getMention(entity->getMention(0));

		if (mention->getMentionType() == Mention::PRON) {

			Symbol headword = mention->getNode()->getHeadWord();
			if (id_1p_sing == -1 && WordConstants::isSingular1pPronoun(headword) &&
				entity->getType().matchesPER())
			{
				id_1p_sing = entity->getID();
			}
			else if (id_1p_per_plural == -1 && WordConstants::is1pPronoun(headword) &&
				entity->getType().matchesPER())
			{
				id_1p_per_plural = entity->getID();
			}
			else if (id_1p_org == -1 && WordConstants::is1pPronoun(headword) &&
				entity->getType().matchesORG())
			{
				id_1p_org = entity->getID();
			}
			else if (id_1p_gpe == -1 && WordConstants::is1pPronoun(headword) &&
				entity->getType().matchesGPE())
			{
				id_1p_gpe = entity->getID();
			}
			else if (id_2p == -1 && WordConstants::is2pPronoun(headword) &&
				entity->getType().matchesPER())
			{
				id_2p = entity->getID();
			}
			if (id_1p_sing != -1 && id_2p != -1 && id_1p_per_plural != 1 &&
				id_1p_org != 1 && id_1p_gpe != 1)
				break;
		}
	}

	// plurals are always safe; if not in speaker mode, you can look at singulars too
	int id_1p_per = id_1p_per_plural;
	if (!speaker_mode && id_1p_per == -1) {
		id_1p_per = id_1p_sing;
	} 

	for (int j = 0; j < n_mentions; j++) {
		Mention *mention = mentionSet->getMention(j);
		Symbol headword = mention->getNode()->getHeadWord();

		if (_use_correct_answers) {
			if (mention->getMentionType() == Mention::PRON && 
				!mention->getEntityType().isDetermined() &&
				(WordConstants::is2pPronoun(headword) || WordConstants::is1pPronoun(headword)))
				{
					lexEntitySet->getMention(mention->getUID())->setEntityType(EntityType::getPERType());
					mention->setEntityType(EntityType::getPERType());
				}
		}

		// we look only at pronouns, of course, but we also have to make sure
		// it's of the right entity type, or we'd end up with a non-PER mention in a PER
		// entity (or whatever), which would be bad.
		if (mention->getMentionType() == Mention::PRON &&
			preLinks[mention->getIndex()] == 0)
		{

			// skip it if it's already linked -- that means we don't have to
			// test what mode we're in below
			if (lexEntitySet->getEntityByMention(mention->getUID()) != 0)
				continue;
		
			if (WordConstants::is1pPronoun(headword) &&
				mention->getEntityType().matchesPER()) 
			{
				if (id_1p_per == -1) {
					lexEntitySet->addNew(mention->getUID(),
										 mention->getEntityType());
					id_1p_per = lexEntitySet->getEntityByMention(
										mention->getUID())->getID();
				}
				else {
					lexEntitySet->add(mention->getUID(), id_1p_per);
				}
			} else if (WordConstants::is1pPronoun(headword) &&
				mention->getEntityType().matchesORG()) 
			{
				if (id_1p_org == -1) {
					lexEntitySet->addNew(mention->getUID(),
										 mention->getEntityType());
					id_1p_org = lexEntitySet->getEntityByMention(
										mention->getUID())->getID();
				}
				else {
					lexEntitySet->add(mention->getUID(), id_1p_org);
				}
			} else if (WordConstants::is1pPronoun(headword) &&
				mention->getEntityType().matchesGPE()) 
			{
				if (id_1p_gpe == -1) {
					lexEntitySet->addNew(mention->getUID(),
										 mention->getEntityType());
					id_1p_gpe = lexEntitySet->getEntityByMention(
										mention->getUID())->getID();
				}
				else {
					lexEntitySet->add(mention->getUID(), id_1p_gpe);
				}
			} else if (WordConstants::is2pPronoun(headword) &&
				mention->getEntityType().matchesPER() && 
				lexEntitySet->getEntityByMention(mention->getUID()) == 0) // make sure it hasn't been already
																	      // added, for example by speaker mode
			{
				if (id_2p == -1) {
					lexEntitySet->addNew(mention->getUID(),
										 mention->getEntityType());
					id_2p = lexEntitySet->getEntityByMention(
										mention->getUID())->getID();
				}
				else {
					lexEntitySet->add(mention->getUID(), id_2p);
				}
			}
		}
	}
}

void ReferenceResolver::add1p2pPronounsToSpeakers(LexEntitySet *lexEntitySet, const MentionSet *mentionSet,
										ReferenceResolver::MentionMap &preLinks)
{
	//if you are doing global linking, you cant get the last mention set
	//const MentionSet *mentionSet = lexEntitySet->getLastMentionSet();
	int n_mentions = mentionSet->getNMentions();

	int last_speaker_id = -1;
	int last_receiver_id = -1;
	int second_to_last_speaker_id = -1;
	bool found_speaker = false;
	bool found_receiver = false;
	bool found_second_to_last_speaker = false;

	for (int j = 0; j < n_mentions; j++) {
		Mention *mention = mentionSet->getMention(j);
		Symbol headword = mention->getNode()->getHeadWord();

		if (_use_correct_answers) {
			if (mention->getMentionType() == Mention::PRON && 
				!mention->getEntityType().isDetermined() &&
				(WordConstants::is2pPronoun(headword) || WordConstants::is1pPronoun(headword)))
				{
					lexEntitySet->getMention(mention->getUID())->setEntityType(EntityType::getPERType());
					mention->setEntityType(EntityType::getPERType());
				}
		}
        
		// we look only at pronouns, of course, but we also have to make sure
		// it's of type PER, or we'd end up with a non-PER mention in a PER
		// entity, which would be bad.
		if (mention->getMentionType() == Mention::PRON &&
			mention->getEntityType().matchesPER() &&
			preLinks[mention->getIndex()] == 0 &&
			lexEntitySet->getEntityByMention(mention->getUID()) == 0)
		{
			if (lexEntitySet->getEntityByMention(mention->getUID()) != 0)
				continue;

			if (WordConstants::isSingular1pPronoun(mention->getNode()->getHeadWord())) {
				if (last_speaker_id != -1) {
					lexEntitySet->add(mention->getUID(), last_speaker_id);
					continue;
				}
			} else if (getDo2PSpeakerMode() &&
				WordConstants::is2pPronoun(mention->getNode()->getHeadWord())) 
			{
				if (last_receiver_id != -1) {
					lexEntitySet->add(mention->getUID(), last_receiver_id);
					continue;
				}
				if (second_to_last_speaker_id != -1) {
					lexEntitySet->add(mention->getUID(), second_to_last_speaker_id);
					continue;
				}
			} else continue;
			//std::cout << _docTheory->getDocument()->getZoning() << "\n";

			if(_docTheory->getDocument()->getZoning()){
			
				//for (int j = 0; j < n_mentions; j++) {
					if (found_speaker && (found_receiver || found_second_to_last_speaker)) //what should this be?
						break;
					//Mention *ment = mentionSet->getMention(j);
					//we want to find if the mention matches any author/orig_author
					int s_token=mention->getNode()->getStartToken();
					int e_token=mention->getNode()->getEndToken();
					TokenSequence* tokenSeq=_docTheory->getSentenceTheory(mentionSet->getSentenceNumber())->getTokenSequence();
					EDTOffset EDTstart=tokenSeq->getToken(s_token)->getStartEDTOffset();
					EDTOffset EDTend=tokenSeq->getToken(e_token)->getEndEDTOffset();

					boost::optional<Zone*> closestZone=_docTheory->getDocument()->getZoning()->findZone(EDTstart);
					
					std::wstring author_value_wstring=L"";
					if(closestZone){
						LSPtr author=closestZone.get()->getAuthor();
						if(author){
							author_value_wstring=author.get()->toWString();
						}
					}
		
					//now need to loop over all the sentences before the corrent one in this zone
					for (int s = mentionSet->getSentenceNumber() - 1; s >= 0; s--) {
					//for(int s=0;s<_docTheory->getNSentences();s++){
						if (found_speaker && (found_receiver || found_second_to_last_speaker)) //what should this be?
							break;
						//std::cout << _docTheory->getSentence(s)->getString()->toString()<<"\n";

						EDTOffset currect_sentence_start=_docTheory->getSentence(s)->getStartEDTOffset();
						EDTOffset currect_sentence_end=_docTheory->getSentence(s)->getEndEDTOffset();

						boost::optional<Zone*> currect_sentence_zone=_docTheory->getDocument()->getZoning()->findZone(currect_sentence_start);

						if(currect_sentence_zone && closestZone && currect_sentence_zone.get()==closestZone.get()){ //this sentence is in the zone
							const MentionSet *potentialMS= _docTheory->getSentenceTheory(s)->getMentionSet();
							for(int m=0;m<potentialMS->getNMentions();m++){
								if (found_speaker && (found_receiver || found_second_to_last_speaker)) //what should this be?
									break;
								//std::cout << potentialMS->getMention(m)->toCasedTextString()<<"\n";
								if(potentialMS->getMention(m)->toCasedTextString().compare(author_value_wstring)==0){
										Entity *authorEnt = lexEntitySet->getEntityByMention(potentialMS->getMention(m)->getUID());
										if (potentialMS->getMention(m)->getEntityType().matchesPER() && authorEnt != 0) {
											if (!found_speaker){
												last_speaker_id = authorEnt->getID();
												
											}
											else {
												second_to_last_speaker_id = authorEnt->getID();
												
											}
										}
										if (!found_speaker)
											found_speaker = true;
										else
											found_second_to_last_speaker = true;
									} 
				
							}

					
						}
					}

					//deal with receiver now.
					/* rules:
						for each pronoun, look for the children zones: 
						if there is only one children zone, then the author/orig_author is the receiver id
						if there are more children zones, then get the offset of the pronoun and see which children zone is closest
						if there is no children
					*/
					
					author_value_wstring=L"";
					if(closestZone){
						std::vector<Zone*> childrenZones=closestZone.get()->getChildren();
						if(childrenZones.size()==1){
							LSPtr author=childrenZones[0]->getAuthor();
							if(author){
								author_value_wstring=author.get()->toWString();
							}
						
						}
						else if(childrenZones.size()>1){
							
							Zone* min_zone;
							for(unsigned i=0;i<childrenZones.size();i++){
								EDTOffset zone_end=childrenZones[i]->getEndEDTOffset();
								if(i==0 && EDTend<zone_end){
									min_zone=childrenZones[i];
									break;
								}
								else if(i+1<childrenZones.size()){
									EDTOffset next_zone_end=childrenZones[i+1]->getEndEDTOffset();
									if(EDTend>zone_end && EDTend< next_zone_end ){
										min_zone=childrenZones[i];
										break;
									}
								}else if(EDTend>zone_end){
									min_zone=childrenZones[i];
									break;
								}
								
							}

						
							LSPtr author=min_zone->getAuthor();
							if(author){
								author_value_wstring=author.get()->toWString();
							}

						
						}
						
						if(childrenZones.size()!=0)
						{
						//now need to loop over all the sentences in this zone
							for (int s = mentionSet->getSentenceNumber() - 1; s >= 0; s--) {
							//for(int s=0;s<_docTheory->getNSentences();s++){
								if (found_speaker && (found_receiver || found_second_to_last_speaker)) //what should this be?
									break;
							//	std::cout << _docTheory->getSentence(s)->getString()->toString()<<"\n";

								EDTOffset currect_sentence_start=_docTheory->getSentence(s)->getStartEDTOffset();
								EDTOffset currect_sentence_end=_docTheory->getSentence(s)->getEndEDTOffset();

								boost::optional<Zone*> currect_sentence_zone=_docTheory->getDocument()->getZoning()->findZone(currect_sentence_start);
						
								
								if(currect_sentence_zone && currect_sentence_zone.get()==closestZone.get()){ //this sentence is in the zone
									const MentionSet *potentialMS= _docTheory->getSentenceTheory(s)->getMentionSet();
									for(int m=0;m<potentialMS->getNMentions();m++){
										if (found_speaker && (found_receiver || found_second_to_last_speaker)) //what should this be?
											break;
									//	std::cout << potentialMS->getMention(m)->toCasedTextString()<<"\n";
										if(potentialMS->getMention(m)->toCasedTextString().compare(author_value_wstring)==0){
												Entity *authorEnt = lexEntitySet->getEntityByMention(potentialMS->getMention(m)->getUID());
												if (potentialMS->getMention(m)->getEntityType().matchesPER() && authorEnt != 0) {
													if (!found_receiver){
														last_receiver_id = authorEnt->getID();
														
													}
												
												}
			
												found_receiver = true;
											
										} 
						
									}

							
								}
							}
						}
					}

			}

		

				// we want to find the most recent speaker and either the receiver or the second-most-recent speaker
				for (int s = mentionSet->getSentenceNumber() - 1; s >= 0; s--) {
					if (found_speaker && (found_receiver || found_second_to_last_speaker))
						break;
					if (_docTheory->isSpeakerSentence(s)) {
						const MentionSet *speakerMS 
							= _docTheory->getSentenceTheory(s)->getMentionSet();
						if (speakerMS->getNMentions() >= 1) {
							const Mention *speakerMent = speakerMS->getMention(0);
							Entity *speakerEnt = lexEntitySet->getEntityByMention(speakerMent->getUID());
							if (speakerMent->getEntityType().matchesPER() && speakerEnt != 0) {
								if (!found_speaker)
									last_speaker_id = speakerEnt->getID();
								else second_to_last_speaker_id = speakerEnt->getID();
							} 
						}
						if (!found_speaker)
							found_speaker = true;
						else
							found_second_to_last_speaker = true;
					}
				}
			
				for (int s = mentionSet->getSentenceNumber() - 1; s >= 0; s--) {
					if (found_speaker && (found_receiver || found_second_to_last_speaker))
							break;
					if (_docTheory->isReceiverSentence(s)) {
						const MentionSet *receiverMS = 
							_docTheory->getSentenceTheory(s)->getMentionSet();
						if (receiverMS->getNMentions() >= 1) {
							const Mention *receiverMent = receiverMS->getMention(0);
							Entity *receiverEnt = lexEntitySet->getEntityByMention(receiverMent->getUID());
							if (receiverMent->getEntityType().matchesPER() && receiverEnt != 0) {
								if (!found_receiver)
									last_receiver_id = receiverEnt->getID();
							} 
						}
						found_receiver = true;
					}
				}


			if (WordConstants::isSingular1pPronoun(mention->getNode()->getHeadWord())) {
				if (last_speaker_id != -1) {
					lexEntitySet->add(mention->getUID(), last_speaker_id);
					continue;
				}
			} else if (getDo2PSpeakerMode() && 
				WordConstants::is2pPronoun(mention->getNode()->getHeadWord())) 
			{
				if (last_receiver_id != -1) {
					lexEntitySet->add(mention->getUID(), last_receiver_id);
					continue;
				}
				if (second_to_last_speaker_id != -1) {
					lexEntitySet->add(mention->getUID(), second_to_last_speaker_id);
					continue;
				}
			}
		}			
	}
}

void ReferenceResolver::addPartitives(LexEntitySet *lexEntitySet, const MentionSet *mentionSet) {
	//if you are doing global linking, you cant get the last mention set
	//const MentionSet *mentionSet = lexEntitySet->getLastMentionSet();
	int n_mentions = mentionSet->getNMentions();

	for (int i = 0; i < n_mentions; i++) {
		Mention *mention = mentionSet->getMention(i);
		if (mention->getMentionType() == Mention::PART &&
			lexEntitySet->getEntityByMention(mention->getUID()) == 0 &&
			mention->getEntityType().isRecognized())
		{
			lexEntitySet->addNew(mention->getUID(), mention->getEntityType());
		}
	}
}









//Methods for a super tenuous way of doing document level linking I make no guarantees....
//the general idea is link all names, then link all descriptors, then link all pronouns

int ReferenceResolver::addNamesToEntitySet(EntitySet *results[],
										 const int max_results,
										 const Parse *parse,
										 const MentionSet *mentionSet,
										 const PropositionSet *propSet)
{

	int nMentions = mentionSet->getNMentions();

	if (max_results != _beam_width)
		setBeamWidth(max_results);

	// set up pre-linked mention array
	std::map<int, const Mention*> preLinks;
	PreLinker::preLinkAppositives(preLinks, mentionSet);
	PreLinker::preLinkTitleAppositives(preLinks, mentionSet);
	PreLinker::preLinkCopulas(preLinks, mentionSet, propSet);
	PreLinker::preLinkSpecialCases(preLinks, mentionSet, propSet);

	//split mentions into three arrays: names, descriptors, pronouns
	//next, perform each of these three linking operations in turn

	resetSearch(mentionSet);
	//now split the mentions
	GrowableArray <MentionUID> names(nMentions);

	for (int j = 0; j < nMentions; j++) {
		Mention *thisMention = mentionSet->getMention(j);
		if(_infoMap!=NULL)
			_infoMap->addMentionInformation(thisMention);
	
		if (preLinks[j] == 0) {
			// categorize non-pre-linked mention
			if (thisMention->mentionType == Mention::NAME)
				names.add(thisMention->getUID());
		}

	}


	//name linking
	_searcher.performSearch(names, mentionSet, *_nameLinker);

	LexEntitySet **leaves = _new LexEntitySet *[max_results];
	int nLeaves = _searcher.removeLeaves(leaves, max_results);
	//the outside world only wants pure EntitySets
	//but we want to hold onto the lexical data
	//so: create pure EntitySet copies from the LexEntitySets,
	//	  extract the lexical data from the LexEntitySets,
	//	  THROW OUT the old LexEntitySet's, CACHE the LexData's,
	//    RETURN the EntitySet's
	for (int k = 0; k < nLeaves; k++) {
		results[k] = _new EntitySet(*leaves[k]);
		_lexDataCache.loadPair(results[k], leaves[k]->getLexData());
		//now the cache owns the LexData, so make sure it doesn't get deleted with the LexEntitySet
		leaves[k]->setLexData(NULL);
		delete leaves[k];
		leaves[k] = NULL;
	}
	delete[] leaves;

	return nLeaves;
}
void ReferenceResolver::postLinkSpecialNameCases(EntitySet* results[],  int nsets){
	
		//run post linker to link Washington to America (and other special cases i find
		int link_pairs[1000];
		int nlinked = PreLinker::postLinkSpecialCaseNames(results[0], link_pairs, 1000);
		for(int m = 0; m< nlinked; m++){
			if(link_pairs[m] != -1){
				//found a Washington -> America link!
				//this leaves Empty Entities, be careful!!!
				Entity* ent1 = results[0]->getEntity(m);
				Entity* ent2 = results[0]->getEntity(link_pairs[m]);
				MentionUID mentions_to_remove[5000];
				int nToRemove = 0;
				int n;
				for(n = 0; n <ent2->getNMentions(); n++){
					ent1->addMention(ent2->getMention(n));
					mentions_to_remove[nToRemove++] =ent2->getMention(n);
				}
				for(n = 0; n <nToRemove; n++){
					ent2->removeMention(mentions_to_remove[n]);
				}
			}
		}
		
}
int ReferenceResolver::addDescToEntitySet(EntitySet *results[],
						  const int max_results,
						  const Parse *parse,
						  const MentionSet *mentionSet,
						  const PropositionSet *propSet)
{
	int nMentions = mentionSet->getNMentions();

	if (max_results != _beam_width)
		setBeamWidth(max_results);

	// set up pre-linked mention array
	std::map<int, const Mention*> preLinks;
	PreLinker::preLinkAppositives(preLinks, mentionSet);
	PreLinker::preLinkTitleAppositives(preLinks, mentionSet);
	PreLinker::preLinkCopulas(preLinks, mentionSet, propSet);
	PreLinker::preLinkSpecialCases(preLinks, mentionSet, propSet);

	//split mentions into three arrays: names, descriptors, pronouns
	//next, perform each of these three linking operations in turn
	//resetSearch(mentionSet);
	//all mention sets will be added in names
	LexEntitySet *newRoot = _new LexEntitySet(*_prevSet);
	_searcher.resetSearch(newRoot);
	//now split the mentions
	GrowableArray <MentionUID> descriptors(nMentions);

	for (int j = 0; j < nMentions; j++) {
		Mention *thisMention = mentionSet->getMention(j);
		if(_infoMap!=NULL)
			_infoMap->addMentionInformation(thisMention);
		if (preLinks[j] == 0) {
			// categorize non-pre-linked mention
			if (_useDescLinker &&
					 thisMention->mentionType == Mention::DESC &&
					 thisMention->getEntityType().isRecognized())
			{
				descriptors.add(thisMention->getUID());
			}
			
		}
	}


	//descriptor linking
	if (_useDescLinker) {

		linkAllLinkablePrelinks(preLinks, mentionSet);
		_searcher.performSearch(descriptors, mentionSet, *_descLinker);
	}

	//retrieve theories and return
	//
	LexEntitySet **leaves = _new LexEntitySet *[max_results];
	int nLeaves = _searcher.removeLeaves(leaves, max_results);
	//the outside world only wants pure EntitySets
	//but we want to hold onto the lexical data
	//so: create pure EntitySet copies from the LexEntitySets,
	//	  extract the lexical data from the LexEntitySets,
	//	  THROW OUT the old LexEntitySet's, CACHE the LexData's,
	//    RETURN the EntitySet's
	for (int k = 0; k < nLeaves; k++) {
		linkPreLinks(leaves[k], mentionSet, preLinks, true);
		results[k] = _new EntitySet(*leaves[k]);
		_lexDataCache.loadPair(results[k], leaves[k]->getLexData());
		//now the cache owns the LexData, so make sure it doesn't get deleted with the LexEntitySet
		leaves[k]->setLexData(NULL);
		delete leaves[k];
		leaves[k] = NULL;
	}
	delete[] leaves;
	return nLeaves;
}






int ReferenceResolver::readdSingletonDescToEntitySet(EntitySet *results[],
						  const int max_results,
						  const Parse *parse,
						  const MentionSet *mentionSet,
						  const PropositionSet *propSet, 
						  int overgen, double me_threshold)
{
	int nMentions = mentionSet->getNMentions();

	if (max_results != _beam_width)
		setBeamWidth(max_results);

	LexEntitySet *newRoot = _new LexEntitySet(*_prevSet);
	_searcher.resetSearch(newRoot);
	//now split the mentions
	GrowableArray <MentionUID> descriptors(nMentions);
	//must remove the mentions from entities first!
	for (int j = 0; j < nMentions; j++) {
		Mention *thisMention = mentionSet->getMention(j);
		if (_useDescLinker &&
			thisMention->mentionType == Mention::DESC &&
			thisMention->getEntityType().isRecognized())
		{
			Entity* thisEnt = _prevSet->getEntityByMention(thisMention->getUID(), thisMention->getEntityType());
			if((thisEnt != 0) && (thisEnt->getNMentions() == 1)){
				Entity* newrootent = newRoot->getEntityByMention(thisMention->getUID(), thisMention->getEntityType());
				newrootent->removeMention(thisMention->getUID());
				thisEnt->removeMention(thisMention->getUID());
				descriptors.add(thisMention->getUID());
			}
			
		}
	}


	//descriptor linking
	if (_useDescLinker) {
		double origthreshold = ((DTCorefLinker*)_descLinker)->getOvergen();
		double origme_threshold = ((DTCorefLinker*)_descLinker)->getMaxentThreshold();
		((DTCorefLinker*)_descLinker)->setMaxentThreshold(me_threshold);
		((DTCorefLinker*)_descLinker)->setOvergen(overgen);
		_searcher.performSearch(descriptors, mentionSet, *_descLinker);
		((DTCorefLinker*)_descLinker)->setMaxentThreshold(origme_threshold);
		((DTCorefLinker*)_descLinker)->setOvergen(origthreshold);
	}

	//retrieve theories and return
	//
	LexEntitySet **leaves = _new LexEntitySet *[max_results];
	int nLeaves = _searcher.removeLeaves(leaves, max_results);
	//the outside world only wants pure EntitySets
	//but we want to hold onto the lexical data
	//so: create pure EntitySet copies from the LexEntitySets,
	//	  extract the lexical data from the LexEntitySets,
	//	  THROW OUT the old LexEntitySet's, CACHE the LexData's,
	//    RETURN the EntitySet's
	for (int k = 0; k < nLeaves; k++) {
		results[k] = _new EntitySet(*leaves[k]);

		_lexDataCache.loadPair(results[k], leaves[k]->getLexData());
		//now the cache owns the LexData, so make sure it doesn't get deleted with the LexEntitySet
		leaves[k]->setLexData(NULL);
		delete leaves[k];
		leaves[k] = NULL;
	}
	delete[] leaves;
	return nLeaves;
}




int ReferenceResolver::addPronToEntitySet(EntitySet *results[],
										 const int max_results,
										 const Parse *parse,
										 const MentionSet *mentionSet,
										 const PropositionSet *propSet)
{
	int nMentions = mentionSet->getNMentions();

	if (max_results != _beam_width)
		setBeamWidth(max_results);

	// set up pre-linked mention array
	std::map<int, const Mention*> preLinks;
	PreLinker::preLinkAppositives(preLinks, mentionSet);
	PreLinker::preLinkTitleAppositives(preLinks, mentionSet);
	PreLinker::preLinkCopulas(preLinks, mentionSet, propSet);
	PreLinker::preLinkSpecialCases(preLinks, mentionSet, propSet);

	//split mentions into three arrays: names, descriptors, pronouns
	//next, perform each of these three linking operations in turn

	//resetSearch(mentionSet);
	LexEntitySet *newRoot = _new LexEntitySet(*_prevSet);
	_searcher.resetSearch(newRoot);
	//now split the mentions
	GrowableArray <MentionUID> pronouns(nMentions);

	for (int j = 0; j < nMentions; j++) {
		Mention *thisMention = mentionSet->getMention(j);
		if(_infoMap!=NULL)
			_infoMap->addMentionInformation(thisMention);
		if (preLinks[j] == 0) {
			// categorize non-pre-linked mention
			if (thisMention->mentionType == Mention::PRON &&
				WordConstants::isLinkingPronoun(
						thisMention->getNode()->getHeadWord()))
				pronouns.add(thisMention->getUID());
		}
	}



	//pronoun linking
	//linkAllLinkablePrelinks(preLinks, mentionSet);
	_searcher.performSearch(pronouns, mentionSet, *_pronounLinker);

	//retrieve theories and return
	//
	LexEntitySet **leaves = _new LexEntitySet *[max_results];
	int nLeaves = _searcher.removeLeaves(leaves, max_results);
	//the outside world only wants pure EntitySets
	//but we want to hold onto the lexical data
	//so: create pure EntitySet copies from the LexEntitySets,
	//	  extract the lexical data from the LexEntitySets,
	//	  THROW OUT the old LexEntitySet's, CACHE the LexData's,
	//    RETURN the EntitySet's
	for (int k = 0; k < nLeaves; k++) {
		// on these final theories we must carry out the pre-linking
		//linkPreLinks(leaves[k], preLinks, true);
	//	linkPreLinks(leaves[k], mentionSet, preLinks, true);
		// handle first- and second-person pronouns
		add1p2pPronouns(leaves[k], mentionSet, preLinks);

		// and don't forget about partitives, which all get their own entities
		if (_create_partitive_entities)
			addPartitives(leaves[k], mentionSet);


		results[k] = _new EntitySet(*leaves[k]);

		_lexDataCache.loadPair(results[k], leaves[k]->getLexData());
		//now the cache owns the LexData, so make sure it doesn't get deleted with the LexEntitySet
		leaves[k]->setLexData(NULL);
		delete leaves[k];
		leaves[k] = NULL;
	}
	delete[] leaves;

	return nLeaves;
}
void ReferenceResolver::resetWithPrevEntitySet(EntitySet *lastEntitySet, Parse* prevParse) {
	//set _prevSet and add last parse

	if (lastEntitySet == 0) {
		delete _prevSet; // SRS
		_prevSet = _new LexEntitySet(_nSentences);
	}
	else {
		//get previous lexical data from the cache.
		// this lexical data may be missing newly added entities
        LexEntitySet::LexData *data = _lexDataCache.retrieveData(lastEntitySet);
		while(data->lexEntities.length() < lastEntitySet->getNEntities())
			data->lexEntities.add(NULL);

		if(data == NULL) {
			throw InternalInconsistencyException("ReferenceResolver::resetWithPrevEntitySet()", "Lexical data not found.");
		}
		delete _prevSet; // SRS
		_prevSet = _new LexEntitySet(*lastEntitySet, *data);
		delete data;
		data = NULL;
		if(prevParse != 0){
			_pronounLinker->addPreviousParse(prevParse);
		}
	}
	if (_useDescLinker)
		_descLinker->resetForNewSentence();
}
void ReferenceResolver::addPartOfSpeechTheory(const PartOfSpeechSequence *pos){
	_pronounLinker->addPartOfSpeechSequence(pos);
}


namespace {
	bool& _do2PSpeakerMode() {
		static bool _m = false;
		return _m;
	}
}

void ReferenceResolver::setDo2PSpeakerMode(bool mode) {
	_do2PSpeakerMode() = mode;
}

bool ReferenceResolver::getDo2PSpeakerMode() {
	return _do2PSpeakerMode();
}
