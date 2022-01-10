// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "KbaStreamCorpus/KbaStreamCorpusItemWriter.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/RelMention.h"
#include "Generic/theories/RelMentionSet.h"
#include "KbaStreamCorpus/thrift-cpp-gen/streamcorpus_types.h"
#include "Generic/common/version.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/SessionLogger.h"
#include <boost/foreach.hpp>

namespace {
	const std::string SERIF_TAGGER_ID("serif");

	bool isAncestor(const SynNode* n1, const SynNode *n2, bool allow_same=false) {
		if (!allow_same) n2 = n2->getParent();
		while (n2) {
			if (n2==n1) return true;
			n2 = n2->getParent();
		}
		return false;
	}

	struct MentionCmp : public std::binary_function<const Mention*, const Mention*,bool>
	{
		inline bool operator()(const Mention* a, const Mention* b) {
			return (a->getHead()->getStartToken() < b->getHead()->getStartToken());
		}
	};

	void logMentionOverlap(const Mention *mention1, const Mention *mention2, const Mention *loser,
						   int tokno, int sent_no=-1, bool is_error=false)
	{
		std::ostringstream err;
		err << "Mentions overlap" << std::endl;
		err << "M1: ";
		mention1->dump(err);
		err << "\n  Extent: [" << mention1->node->getStartToken() 
			<< ":" << mention1->node->getEndToken() << "] " 
			<< mention1->node->toFlatString()
			<< "\n  Head: [" << mention1->getHead()->getStartToken()
			<< ":" << mention1->getHead()->getEndToken() << "] "
			<< mention1->getHead()->toFlatString() << "\n";
		err << "M2: ";
		mention2->dump(err);
		err << "\n  Extent: [" << mention2->node->getStartToken() 
			<< ":" << mention2->node->getEndToken() << "] " 
			<< mention2->node->toFlatString()
			<< "\n  Head: [" << mention2->getHead()->getStartToken()
			<< ":" << mention2->getHead()->getEndToken() << "] "
			<< mention2->getHead()->toFlatString() << "\n";
		err << "tokno: "<< tokno << std::endl;
		if (sent_no != -1) err << "sent_no: "<< sent_no << std::endl;
		err << "Discarding mention ";
		loser->dump(err);
		if (is_error)
			SessionLogger::err("kba-mention-overlap") << err.str();
		else
			SessionLogger::warn("kba-mention-overlap") << err.str();
	}


}

KbaStreamCorpusItemWriter::KbaStreamCorpusItemWriter()
	: _docTheory(0), _item(0),
	  _num_mentions(0), _num_entities(0), _num_relations(0),
	  _num_mentions_discarded_from_overlap(0),
	  _num_relations_discarded_because_mention_not_found(0),
	  _include_serifxml(false), _include_results(false)
{
	_include_serifxml = ParamReader::isParamTrue("kba_write_serifxml_to_chunk");
	_include_results = (ParamReader::isParamTrue("kba_write_results_to_chunk") ||
						!_include_serifxml);
}

KbaStreamCorpusItemWriter::~KbaStreamCorpusItemWriter() {
	if (_num_mentions > 0) {
		SessionLogger::info("kba-stream-corpus")
			<< "KBA StreamCorpus Item Writer:" 
			<< "\n   Mentions: " << _num_mentions 
			<< "\n   Entities: " << _num_entities 
			<< "\n   Relations: " << _num_relations 
			//<< "\n Mentions discarded because their heads overlap: " 
			//<< _num_mentions_discarded_from_overlap 
			//<< "\n Relations discarded because mention not found: " 
			//<< _num_relations_discarded_because_mention_not_found
			<< std::endl;
	}
}

void KbaStreamCorpusItemWriter::addDocTheoryToItem(const DocTheory* docTheory, 
												   streamcorpus::StreamItem *item) 
{
	// Reset all variables for this new item.
	_docTheory = docTheory;
	_item = item;
	_tokenMap.clear();
	_mentionId.clear();
	_mentionSent.clear();
	_equivId.clear();
	_offsetContentForm = _itemReader.getSourceName(*item);

	// Add all data.
	addTagging();
	if (_include_results) {
		SessionLogger::info("kba-stream-corpus") 
			<< "Writing results for " << docTheory->getDocument()->getName();
		addSentences();
		addMentions();
		addParses();
		addEntities();
		addRelations();
	}
}

void KbaStreamCorpusItemWriter::addTagging() {
	// Add the tagging
	streamcorpus::Tagging &tagging = _item->body.taggings[SERIF_TAGGER_ID];
	_item->body.__isset.taggings = true;
	// Set the attributes
	tagging.__set_tagger_id(SERIF_TAGGER_ID);
	tagging.__set_tagger_version(SerifVersion::getVersionString());
	tagging.__set_tagger_config(UnicodeUtil::toUTF8StdString(SerifVersion::getSerifLanguage().toString()));
	if (_include_serifxml) {
		SerifXML::XMLSerializedDocTheory serialized(_docTheory);
		std::ostringstream out;
		serialized.save(out);
		tagging.__set_raw_tagging(out.str());
		tagging.__isset.raw_tagging = true;
		SessionLogger::info("kba-stream-corpus") 
			<< "Writing serifxml for " << _docTheory->getDocument()->getName()
			<< " (" << tagging.raw_tagging.size() << " bytes)";
	}
	// to do: generation time!
	// to do: more info for config??
}

void KbaStreamCorpusItemWriter::addSentences() {
	_item->body.__isset.sentences = true;
	std::vector<streamcorpus::Sentence> &sentences = _item->body.sentences[SERIF_TAGGER_ID];
	sentences.resize(_docTheory->getNSentences());

	int doc_token_num_offset = 0;
	for (int sent_no=0; sent_no<_docTheory->getNSentences(); ++sent_no) {
		const SentenceTheory* sentTheory = _docTheory->getSentenceTheory(sent_no);
		serifToScSentence(sentTheory, doc_token_num_offset, sentences[sent_no]);
		doc_token_num_offset += sentTheory->getTokenSequence()->getNTokens();
	}
}

void KbaStreamCorpusItemWriter::serifToScSentence(const SentenceTheory *sentTheory, int doc_token_num_offset, streamcorpus::Sentence &scSent) {
	const TokenSequence *tokSeq = sentTheory->getTokenSequence();
	if (!tokSeq) return;
	scSent.__isset.tokens = true;
	std::vector<streamcorpus::Token> &scTokens = scSent.tokens;
	scTokens.resize(tokSeq->getNTokens());
	for (int sent_pos=0; sent_pos<tokSeq->getNTokens(); ++sent_pos) {
		const Token *tok = tokSeq->getToken(sent_pos);
		streamcorpus::Token &scToken = scTokens[sent_pos];
		scToken.__isset.offsets = true;
		streamcorpus::Offset &scOffset = scToken.offsets[streamcorpus::OffsetType::CHARS];
		scToken.__set_token_num(doc_token_num_offset+sent_pos);
		scToken.__set_token(UnicodeUtil::toUTF8StdString(tok->getSymbol().to_string()));
		scToken.__set_sentence_pos(sent_pos);
		scOffset.__set_type(streamcorpus::OffsetType::CHARS);
		scOffset.__set_first(tok->getStartCharOffset().value());
		scOffset.__set_length(tok->getEndCharOffset().value() - (int32_t)scOffset.first + 1);
		scOffset.__set_content_form(_offsetContentForm);
		_tokenMap[tok] = &scToken;
	}
}

void KbaStreamCorpusItemWriter::addMentions() {
	for (int sent_no=0; sent_no<_docTheory->getNSentences(); ++sent_no) {
		const SentenceTheory* sentTheory = _docTheory->getSentenceTheory(sent_no);
		const TokenSequence *tokSeq = sentTheory->getTokenSequence();
		if (!tokSeq) return;

		// Get a list of all mentions in this sentence.
		const MentionSet* mentionSet = sentTheory->getMentionSet();
		if (!mentionSet) return;
		std::list<const Mention*> mentionList;
		for (int ment_no=0; ment_no<mentionSet->getNMentions(); ++ment_no) {
			const Mention *mention = mentionSet->getMention(ment_no);
			if (mention->mentionType==Mention::NAME ||
				mention->mentionType==Mention::PRON ||
				mention->mentionType==Mention::DESC)
				mentionList.push_back(mention);
		}

		// Check for mentions that overlap.  Really we should just
		// have to do this once, but occasionally it looks like we
		// need to do it multiple times to catch all overlaps.  Todo:
		// look into why this is the case, and how it can be fixed.
		for (int i=0; i<10; ++i) 
			if (!fixMentionOverlaps(mentionList, tokSeq->getNTokens()))
				break;

		mentionList.sort(MentionCmp());

		// Add each mention's information to the tokens that make it up.
		BOOST_FOREACH(const Mention* mention, mentionList) {
			const SynNode *head = mention->getHead();

			// Do a sanity check: this new mention should not overlap
			// with anything we've already tagged, but if it does,
			// then issue a warning and move on.
			bool unexpected_overlap = false;
			for (int tokno=head->getStartToken(); tokno<=head->getEndToken(); ++tokno) {
				streamcorpus::Token* scToken = _tokenMap[tokSeq->getToken(tokno)];
				// This is intended to never happen -- in particular,
				// the method fixMentionOverlaps() is supposed to
				// remove all mentions that overlap.  But it appears
				// that it's not doing its job entirely, so some get
				// through.  If they do, then just throw them away.
				if (scToken->mention_id != -1) {
					unexpected_overlap = true;
					BOOST_FOREACH(const Mention* m2, mentionList) {
						std::map<const MentionUID, int>::iterator it = _mentionId.find(m2->getUID());
						if (it != _mentionId.end() && it->second == scToken->mention_id) {
							logMentionOverlap(mention, m2, mention, tokno, sent_no, true);
						}
					}
				}
			}
			if (unexpected_overlap) {
				++_num_mentions_discarded_from_overlap;
				continue;
			}

			// Tag this mention.
			++_num_mentions;
			int sc_mention_id = static_cast<int>(_mentionId.size());
			_mentionId[mention->getUID()] = sc_mention_id;
			_mentionSent[mention->getUID()] = sent_no;
			for (int tokno=head->getStartToken(); tokno<=head->getEndToken(); ++tokno) {
				streamcorpus::Token* scToken = _tokenMap[tokSeq->getToken(tokno)];
				scToken->__set_mention_id(sc_mention_id);
				EntityType etype = mention->getEntityType();
				if (etype.isDetermined())
					scToken->__set_entity_type(serifToScEntityType(etype.getName()));
				scToken->__set_mention_type(serifToScMentionType(mention->mentionType));
			}
		}
	}
}

bool KbaStreamCorpusItemWriter::fixMentionOverlaps(std::list<const Mention*> &mentionList,
												   int num_tokens) 
{
	bool fixed_any = false;
	typedef std::list<const Mention*>::iterator ListIter;
	ListIter endIter = mentionList.end();
	std::vector<ListIter> tokToMention(num_tokens, endIter);

	ListIter m=mentionList.begin();
	while (m!=mentionList.end()) {
		const Mention* mention = *m;
		const SynNode *head = mention->getHead();
		bool erased_m = false;
		for (int tokno=head->getStartToken(); tokno<=head->getEndToken(); ++tokno) {
			// Check if this mention overlaps with another one; and if
			// so, then choose one of them to discard.
			ListIter m2 = tokToMention[tokno];
			if (m2 != endIter) {
				const Mention *mention2 = *m2;
				ListIter loser = m;
				if ((mention->mentionType != Mention::NAME && mention2->mentionType == Mention::NAME))
					loser = m;
				else if ((mention->mentionType == Mention::NAME && mention2->mentionType != Mention::NAME))
					loser = m2;
				else if (isAncestor(head, mention2->getHead()))
					loser = m;
				else if (isAncestor(mention2->getHead(), head))
					loser = m2;
				else if (isAncestor(mention->node, mention2->node))
					loser = m;
				else if (isAncestor(mention2->node, mention->node))
					loser = m;
				else
					loser = m;
				++_num_mentions_discarded_from_overlap;
				fixed_any = true;
				logMentionOverlap(mention, mention2, *loser, tokno);
				const SynNode *loserHead = (*loser)->getHead();
				for (int t=loserHead->getStartToken(); t<=loserHead->getEndToken(); ++t)
					tokToMention[t] = endIter;
				if (loser == m) {
					// Delete this mention (and update iterator to
					// point to the next one in the list)
					m = mentionList.erase(m);
					erased_m = true;
					break;
				} else {
					mentionList.erase(m2);
				}
			} 
			// Update the token->mention map.
			tokToMention[tokno] = m;
		}
		// Move to the next mention.
		if (!erased_m)
			++m;
	}
	return fixed_any;
}

namespace {
	std::map<Mention::Type, streamcorpus::MentionType::type> makeMentionTypeMap() {
		std::map<Mention::Type, streamcorpus::MentionType::type> mentionTypeMap;
		mentionTypeMap[Mention::PRON] = streamcorpus::MentionType::PRO;
		mentionTypeMap[Mention::NAME] = streamcorpus::MentionType::NAME;
		mentionTypeMap[Mention::DESC] = streamcorpus::MentionType::NOM;
		// Note: no entries for NONE, PART, APPO, LIST, INFL.
		return mentionTypeMap;
	}

	Symbol::HashMap<streamcorpus::EntityType::type> makeEntityTypeMap() {
		Symbol::HashMap<streamcorpus::EntityType::type> entityTypeMap;
		entityTypeMap[Symbol(L"PER")] = streamcorpus::EntityType::PER;
		entityTypeMap[Symbol(L"ORG")] = streamcorpus::EntityType::ORG;
		entityTypeMap[Symbol(L"LOC")] = streamcorpus::EntityType::LOC;
		entityTypeMap[Symbol(L"GPE")] = streamcorpus::EntityType::GPE;
		entityTypeMap[Symbol(L"FAC")] = streamcorpus::EntityType::FAC;
		entityTypeMap[Symbol(L"VEH")] = streamcorpus::EntityType::VEH;
		entityTypeMap[Symbol(L"WEA")] = streamcorpus::EntityType::WEA;
		entityTypeMap[Symbol(L"OTH")] = streamcorpus::EntityType::MISC;
		return entityTypeMap;
	}

	Symbol::HashMap<streamcorpus::RelationType::type> makeRelationTypeMap() {
		Symbol::HashMap<streamcorpus::RelationType::type> relationTypeMap;
		relationTypeMap[Symbol(L"PHYS.Located")] = streamcorpus::RelationType::PHYS_Located;
		relationTypeMap[Symbol(L"PHYS.Near")] = streamcorpus::RelationType::PHYS_Near;
		relationTypeMap[Symbol(L"PART-WHOLE.Geographical")] = streamcorpus::RelationType::PARTWHOLE_Geographical;
		relationTypeMap[Symbol(L"PART-WHOLE.Subsidiary")] = streamcorpus::RelationType::PARTWHOLE_Subsidiary;
		relationTypeMap[Symbol(L"PART-WHOLE.Artifact")] = streamcorpus::RelationType::PARTWHOLE_Artifact;
		relationTypeMap[Symbol(L"PER-SOC.Business")] = streamcorpus::RelationType::PERSOC_Business;
		relationTypeMap[Symbol(L"PER-SOC.Family")] = streamcorpus::RelationType::PERSOC_Family;
		relationTypeMap[Symbol(L"PER-SOC.Lasting-Personal")] = streamcorpus::RelationType::PERSOC_LastingPersonal;
		relationTypeMap[Symbol(L"ORG-AFF.Employment")] = streamcorpus::RelationType::ORGAFF_Employment;
		relationTypeMap[Symbol(L"ORG-AFF.Ownership")] = streamcorpus::RelationType::ORGAFF_Ownership;
		relationTypeMap[Symbol(L"ORG-AFF.Founder")] = streamcorpus::RelationType::ORGAFF_Founder;
		relationTypeMap[Symbol(L"ORG-AFF.Student-Alum")] = streamcorpus::RelationType::ORGAFF_StudentAlum;
		relationTypeMap[Symbol(L"ORG-AFF.Sports-Affiliation")] = streamcorpus::RelationType::ORGAFF_SportsAffiliation;
		relationTypeMap[Symbol(L"ORG-AFF.Investor-Shareholder")] = streamcorpus::RelationType::ORGAFF_InvestorShareholder;
		relationTypeMap[Symbol(L"ORG-AFF.Membership")] = streamcorpus::RelationType::ORGAFF_Membership;
		relationTypeMap[Symbol(L"ART.User-Owner-Inventor-Manufacturer")] = streamcorpus::RelationType::ART_UserOwnerInventorManufacturer;
		relationTypeMap[Symbol(L"GEN-AFF.Citizen-Resident-Religion-Ethnicity")] = streamcorpus::RelationType::GENAFF_CitizenResidentReligionEthnicity;
		relationTypeMap[Symbol(L"GEN-AFF.Org-Location")] = streamcorpus::RelationType::GENAFF_OrgLocation;
		relationTypeMap[Symbol(L"Business.Declare-Bankruptcy")] = streamcorpus::RelationType::Business_DeclareBankruptcy;
		relationTypeMap[Symbol(L"Business.End-Org")] = streamcorpus::RelationType::Business_EndOrg;
		relationTypeMap[Symbol(L"Business.Merge-Org")] = streamcorpus::RelationType::Business_MergeOrg;
		relationTypeMap[Symbol(L"Business.Start-Org")] = streamcorpus::RelationType::Business_StartOrg;
		relationTypeMap[Symbol(L"Conflict.Attack")] = streamcorpus::RelationType::Conflict_Attack;
		relationTypeMap[Symbol(L"Conflict.Demonstrate")] = streamcorpus::RelationType::Conflict_Demonstrate;
		relationTypeMap[Symbol(L"Contact.Phone-Write")] = streamcorpus::RelationType::Contact_PhoneWrite;
		relationTypeMap[Symbol(L"Contact.Meet")] = streamcorpus::RelationType::Contact_Meet;
		relationTypeMap[Symbol(L"Justice.Acquit")] = streamcorpus::RelationType::Justice_Acquit;
		relationTypeMap[Symbol(L"Justice.Appeal")] = streamcorpus::RelationType::Justice_Appeal;
		relationTypeMap[Symbol(L"Justice.Arrest-Jail")] = streamcorpus::RelationType::Justice_ArrestJail;
		relationTypeMap[Symbol(L"Justice.Charge-Indict")] = streamcorpus::RelationType::Justice_ChargeIndict;
		relationTypeMap[Symbol(L"Justice.Convict")] = streamcorpus::RelationType::Justice_Convict;
		relationTypeMap[Symbol(L"Justice.Execute")] = streamcorpus::RelationType::Justice_Execute;
		relationTypeMap[Symbol(L"Justice.Extradite")] = streamcorpus::RelationType::Justice_Extradite;
		relationTypeMap[Symbol(L"Justice.Fine")] = streamcorpus::RelationType::Justice_Fine;
		relationTypeMap[Symbol(L"Justice.Pardon")] = streamcorpus::RelationType::Justice_Pardon;
		relationTypeMap[Symbol(L"Justice.Release-Parole")] = streamcorpus::RelationType::Justice_ReleaseParole;
		relationTypeMap[Symbol(L"Justice.Sentence")] = streamcorpus::RelationType::Justice_Sentence;
		relationTypeMap[Symbol(L"Justice.Sue")] = streamcorpus::RelationType::Justice_Sue;
		relationTypeMap[Symbol(L"Justice.Trial-Hearing")] = streamcorpus::RelationType::Justice_TrialHearing;
		relationTypeMap[Symbol(L"Life.Be-Born")] = streamcorpus::RelationType::Life_BeBorn;
		relationTypeMap[Symbol(L"Life.Die")] = streamcorpus::RelationType::Life_Die;
		relationTypeMap[Symbol(L"Life.Divorce")] = streamcorpus::RelationType::Life_Divorce;
		relationTypeMap[Symbol(L"Life.Injure")] = streamcorpus::RelationType::Life_Injure;
		relationTypeMap[Symbol(L"Life.Marry")] = streamcorpus::RelationType::Life_Marry;
		relationTypeMap[Symbol(L"Movement.Transport")] = streamcorpus::RelationType::Movement_Transport;
		relationTypeMap[Symbol(L"Personnel.Elect")] = streamcorpus::RelationType::Personnel_Elect;
		relationTypeMap[Symbol(L"Personnel.End-Position")] = streamcorpus::RelationType::Personnel_EndPosition;
		relationTypeMap[Symbol(L"Personnel.Nominate")] = streamcorpus::RelationType::Personnel_Nominate;
		relationTypeMap[Symbol(L"Personnel.Start-Position")] = streamcorpus::RelationType::Personnel_StartPosition;
		relationTypeMap[Symbol(L"Transaction.Transfer-Money")] = streamcorpus::RelationType::Transaction_TransferMoney;
		relationTypeMap[Symbol(L"Transaction.Transfer-Ownership")] = streamcorpus::RelationType::Transaction_TransferOwnership;
		return relationTypeMap;
	}
}

streamcorpus::EntityType::type KbaStreamCorpusItemWriter::serifToScEntityType(Symbol etype) {
	static Symbol::HashMap<streamcorpus::EntityType::type> entityTypeMap = makeEntityTypeMap();
	return entityTypeMap[etype];
}

streamcorpus::MentionType::type KbaStreamCorpusItemWriter::serifToScMentionType(Mention::Type mtype) {
	static std::map<Mention::Type, streamcorpus::MentionType::type> mentionTypeMap = makeMentionTypeMap();
	return mentionTypeMap[mtype];
}

streamcorpus::RelationType::type KbaStreamCorpusItemWriter::serifToScRelationType(Symbol rtype) {
	static Symbol::HashMap<streamcorpus::RelationType::type> relationTypeMap = makeRelationTypeMap();
	return relationTypeMap[rtype];
}

void KbaStreamCorpusItemWriter::addParses() {
	for (int sent_no=0; sent_no<_docTheory->getNSentences(); ++sent_no) {
		const SentenceTheory* sentTheory = _docTheory->getSentenceTheory(sent_no);
		const Parse *parse = sentTheory->getPrimaryParse();
		if (!parse) return;
		const TokenSequence *tokSeq = parse->getTokenSequence();
		if (!tokSeq) return;
		if (tokSeq->getNTokens() > 0) {
			addDepParse(tokSeq, parse->getRoot());
		}
	}
}

void KbaStreamCorpusItemWriter::addDepParse(const TokenSequence *tokSeq,
											const SynNode *node, int parent_id, 
											std::string path) 
{
	if (node->isPreterminal()) {
		const Token* tok = tokSeq->getToken(node->getStartToken());
		streamcorpus::Token* scToken = _tokenMap[tok];
		scToken->__set_pos(UnicodeUtil::toUTF8StdString(node->getTag().to_string()));
		if (node->getNChildren()==1)
			scToken->__set_lemma(UnicodeUtil::toUTF8StdString(node->getChild(0)->getTag().to_string()));
		if (parent_id != -1) {
			scToken->__set_parent_id(parent_id);
		}
		scToken->__set_dependency_path("/"+path+"\\");
	} else {
		const SynNode *head = node->getHead();
		if (head==0) {
			std::cerr << "Warning: headless syn node?" << std::endl;
			head = node->getChild(0);
		}
		int headSentPos = node->getHeadPreterm()->getStartToken();
		std::string pathToHeadTerminal = getPathToHeadTerminal(node);
		std::string pathFromHeadTerminal = getPathFromHeadTerminal(node, path);
		for (int c=0; c<node->getNChildren(); ++c) {
			const SynNode *child = node->getChild(c);
			if (child == head)
				addDepParse(tokSeq, child, parent_id, pathFromHeadTerminal);
			else
				addDepParse(tokSeq, child, headSentPos, pathToHeadTerminal);
		}
	}
}

std::string KbaStreamCorpusItemWriter::getPathFromHeadTerminal(const SynNode *node, const std::string& path) {
	std::ostringstream s;
	s << node->getTag().to_debug_string();
	if (!path.empty()) s << "/" << path;
	return s.str();
}

std::string KbaStreamCorpusItemWriter::getPathToHeadTerminal(const SynNode *node) {
	if (node == 0) {
		return "??"; // this could only happen if we have a headless syn node.
	} else if (node->isPreterminal()) {
		return "";
	} else {
		std::ostringstream s;
		s << node->getTag().to_debug_string();
		std::string rest = getPathToHeadTerminal(node->getHead());
		if (!rest.empty()) s << "\\" << rest;
		return s.str();
	}
}


void KbaStreamCorpusItemWriter::addEntities() {
	const EntitySet* entitySet = _docTheory->getEntitySet();
	if (!entitySet) return;
	for (int e=0; e<entitySet->getNEntities(); ++e) {
		const Entity *entity = entitySet->getEntity(e);
		EntityType etype = entity->getType();

		// Add an equiv_id for this entity.
		int equiv_id = _equivId.size();
		_equivId[entity->getID()] = equiv_id;

		// Mark each mention in the entity.
		bool added_a_mention = false;
		for (int m=0; m<entity->getNMentions(); ++m) {
			const Mention *mention = entitySet->getMention(entity->getMention(m));
			if (_mentionId.find(mention->getUID()) != _mentionId.end()) {

				// Find the token sequence for this mention.
				int sent_no = _mentionSent[mention->getUID()];
				const SentenceTheory* sentTheory = _docTheory->getSentenceTheory(sent_no);
				const Parse *parse = sentTheory->getPrimaryParse();
				const TokenSequence *tokSeq = parse->getTokenSequence();

				// Mark each token in the mention's head.
				const SynNode *head = mention->getHead();
				for (int tokno=head->getStartToken(); tokno<=head->getEndToken(); ++tokno) {
					streamcorpus::Token* scToken = _tokenMap[tokSeq->getToken(tokno)];
					scToken->__set_equiv_id(equiv_id);
					if (etype.isDetermined())
						scToken->__set_entity_type(serifToScEntityType(etype.getName()));
				}
				added_a_mention = true;
			}
		}
		if (added_a_mention)
			++_num_entities;

	}
}

void KbaStreamCorpusItemWriter::addRelations() {
	_item->body.__isset.relations = true;
	std::vector<streamcorpus::Relation> &scRelations = _item->body.relations[SERIF_TAGGER_ID];
	// Clear any old relation values:
	scRelations.clear();

	for (int sent_no=0; sent_no<_docTheory->getNSentences(); ++sent_no) {
		const SentenceTheory* sentTheory = _docTheory->getSentenceTheory(sent_no);
		if (!sentTheory) continue;
		const RelMentionSet* rmSet = sentTheory->getRelMentionSet();
		if (!rmSet) continue;
		for (int rm=0; rm<rmSet->getNRelMentions(); ++rm) {
			const RelMention *relMention = rmSet->getRelMention(rm);
			const Mention *lhs = relMention->getLeftMention();
			const Mention *rhs = relMention->getRightMention();
			if ((_mentionId.find(lhs->getUID()) == _mentionId.end()) ||
				(_mentionId.find(rhs->getUID()) == _mentionId.end())) {
				++_num_relations_discarded_because_mention_not_found;
				continue; // mention not found.
			}
			scRelations.push_back(streamcorpus::Relation());
			streamcorpus::Relation &scRel = scRelations.back();

			scRel.__set_mention_id_1((*_mentionId.find(lhs->getUID())).second);
			scRel.__set_mention_id_2((*_mentionId.find(rhs->getUID())).second);
			scRel.__set_sentence_id_1((*_mentionSent.find(lhs->getUID())).second);
			scRel.__set_sentence_id_2((*_mentionSent.find(rhs->getUID())).second);
			scRel.__set_relation_type(serifToScRelationType(relMention->getType()));
			++_num_relations;
		}
	}
}

