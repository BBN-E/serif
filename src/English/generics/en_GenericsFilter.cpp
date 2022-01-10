// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UTF8InputStream.h"
#include "English/generics/en_GenericsFilter.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/RelationSet.h"
#include "Generic/theories/Relation.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "English/parse/en_STags.h"
#include "English/common/en_WordConstants.h"
#include "Generic/common/SymbolHash.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SessionLogger.h"

#include <wchar.h>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
using namespace std;

EnglishGenericsFilter::EnglishGenericsFilter()
	: _debug(L"generics_filter_debug"), _remove_singleton_descriptors(false)
{
	std::string param_file = ParamReader::getParam("generics_word_list");
	if (param_file.empty()) {
		_usingCommonSpecifics = false;
	}
	else {
		// load the commonly specific words into the hash
		_loadCommonSpecifics(param_file.c_str());
		_usingCommonSpecifics = true;
	}

	_remove_singleton_descriptors = ParamReader::isParamTrue("remove_singleton_descriptors");
	FILTER_GENERICS = ParamReader::getRequiredTrueFalseParam("filter_generics");
}

EnglishGenericsFilter::~EnglishGenericsFilter() {
	if (_usingCommonSpecifics)
		delete _commonSpecifics;
}

void EnglishGenericsFilter::_loadCommonSpecifics(const char* file)
{
	// NOTE: the original way this was read, the entire line became a symbol. Do we still
	// want to do that??
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(file));
	UTF8InputStream& stream(*stream_scoped_ptr);
	if (stream.fail()) {
		string err = "Problem opening ";
		err.append(file);
		throw UnexpectedInputException("EnglishGenericsFilter::_loadCommonSpecifics()",
			(char *)err.c_str());
	}
	_commonSpecifics = _new SymbolHash(4000);

	// This is the ideal way to do it, but for now the list is very junky
	// so we have to filter and lower case things
	UTF8Token tok;
/*	while (!stream.eof()) {
		stream >> tok;
		_debug << "Adding " << tok.symValue().to_debug_string() << "\n";
		_commonSpecifics->add(tok.symValue());
	}
*/
	std::wstring line;
	while (!stream.eof()) {
		stream.getLine(line);
		// only keep lines without spaces
		if (wcspbrk(line.c_str(), L" ") == 0) {
			std::transform(line.begin(), line.end(), line.begin(), towlower);
			Symbol lineSym(line.c_str());
			_commonSpecifics->add(lineSym);
			_debug << "Adding " << lineSym.to_debug_string() << "\n";
		}
		else {
			if (_debug.isActive()) {
				Symbol badSym(line.c_str());
				_debug << "Rejecting " << badSym.to_debug_string() << "\n";
			}
		}
	}

	stream.close();
}

// if the entity is generic, set its flag.
void EnglishGenericsFilter::filterGenerics(DocTheory* docTheory)
{
	if (!FILTER_GENERICS)
		return;

	const EntitySet* ents = docTheory->getEntitySet();
	const RelationSet* rels = docTheory->getRelationSet();

	if (ents == 0) {
		SessionLogger::warn("null_entity_set") << "Null entity set";
		return;
	}
	// initialize sentence-level characteristics arrays
	_is_sent_hypothetical = _new bool[docTheory->getNSentences()];
	_is_sent_name_bearing = _new bool[docTheory->getNSentences()];
	// fill sentence-level characteristics arrays
	_fillHypothetical(_is_sent_hypothetical, docTheory);
	_fillNameBearing(_is_sent_name_bearing, docTheory);

	// check each entity for genericness
	int i;
	for (i=0; i < ents->getNEntities(); i++) {
		Entity* ent = ents->getEntity(i);
		if (_isGeneric(ent, ents, rels))
			ent->setGeneric();
	}
	// done with sentence-level characteristics arrays
	delete [] _is_sent_hypothetical;
	delete [] _is_sent_name_bearing;

}

// top level stuff is based on all the mentions of the entity.
// after clearing this, we'll pass to specifics about the singleton entity
bool EnglishGenericsFilter::_isGeneric(Entity* ent, const EntitySet* ents,
								const RelationSet* rels)
{
	// FALSE unless singleton descriptor or descriptor and pronouns

	// TRUE if the only mentions are pronouns

	int n_names = 0;
	int n_descs = 0;
	Mention* descMention = 0;
	int i;
	for (i = 0; i < ent->mentions.length(); i++) {
		Mention* ment = ents->getMention(ent->mentions[i]);
		switch (ment->mentionType) {
			case Mention::PRON:
				break;
			case Mention::NAME:
			case Mention::NEST:
				n_names++;
				break;
			case Mention::DESC:
				n_descs++;
				descMention = ment;
				break;
			default:
				// anything else signifies non-generic
				return _debugReturn(false, "node is not a desc or pron", ment->node);
		}
	}

	if (n_names > 0)
		return _debugReturn(false, "entity has names");

	if (n_descs > 1)
		return _debugReturn(false, "entity has more than one descriptor");

	if (_remove_singleton_descriptors && n_descs == 1)
		return _debugReturn(true, "entity has only one descriptor");

	if (n_descs == 0)
		return _debugReturn(true, "saw only pronouns");

	// not generic if there is a non-citizen relation
	int id = ent->getID();
	for (int j = 0; j < rels->getNRelations(); j++) {
		Relation *rel = rels->getRelation(j);
		if (rel->getType() == RelationConstants::ROLE_CITIZEN_OF)
			continue;
		if (rel->getLeftEntityID() == id ||
			rel->getRightEntityID() == id)
			return _debugReturn(false, "involved in non-citizen relation");
	}

	// look specifically at the mention to determine genericness
	return _isGenericMention(descMention, ents);
}

bool EnglishGenericsFilter::_isGenericMention(Mention* ment, const EntitySet* ents)
{
	// the syntactic structure test series:
	// these all involve something to do with the syn node
	const SynNode* node = ment->node;

	// it's generic if the descriptor isn't an np (and it's weird)
	if (!_isNPish(node->getTag()))
		return _debugReturn(true, "node isn't np-ish", node);
	const SynNode* coreNPA = _getCoreNPA(node);

	// not generic if coreNPA is nppos and npa is "definite"
	// e.g. "The country's leader"
	if (coreNPA->getTag() == EnglishSTags::NPPOS && _isDefinite(coreNPA))
		return _debugReturn(false, "nppos and definite core npa", node, coreNPA);

	// the next 4 tests look at the premods of the core npa.
	if (coreNPA->getHeadIndex() > 0) {
		int i;
		// this flag needed for a later test
		bool isFirstDT = false;
		for (i=0; i < coreNPA->getHeadIndex(); i++) {
			const SynNode* pre = coreNPA->getChild(i);
			// set flag for later test
			if (i == 0 && pre->getTag() == EnglishSTags::DT) {
				isFirstDT = true;
				continue;
			}
			Symbol tag = pre->getTag();
			// non generic if a premod is:
			// 1) NPPOS: "people's nation"
			// 2) NPPRO: TODO: what's an nppro?
			// 3) CD: "five factories"
			// 4) QP: "a percentage of the population"
			if (tag == EnglishSTags::NPPOS ||
				tag == EnglishSTags::NPPRO ||
				tag == EnglishSTags::CD ||
				tag == EnglishSTags::QP)
				return _debugReturn(false, "core npa premod is NPPOS/NPPRO/CD/QP", node, pre);
			// non generic if a premod is a numeric JJ
			// e.g. "5.3 cities"
			if (tag == EnglishSTags::JJ && _isNumeric(pre->getHeadWord()))
				return _debugReturn(false, "core npa premod is numeric jj", node, pre);

			// non generic if the first premod was a dt (or nppos, or nppro, but those cases
			// were caught above) and there is a premod with a name. e.g. "the Iraqi president"
			if (isFirstDT && pre->hasMention()) {
				Mention* preMent = ents->getMentionSet(ment->getSentenceNumber())->getMentionByNode(pre);
				if (preMent == 0)
					throw InternalInconsistencyException("EnglishGenericsFilter::_isGenericMention()",
														 "couldn't find premod mention in mention set");
				if (preMent->mentionType == Mention::NAME)
					return _debugReturn(false, "core npa with dt and name in premod");
			}

		}

		// non generic if the first premod of core was a dt (the flag set before)
		// and the whole node is an np, and the postmods of the node are a pp
		// with a name. e.g. "The president of Iraq"
		if (isFirstDT && node->getTag() == EnglishSTags::NP) {
			for (i = node->getHeadIndex()+1; i < node->getNChildren(); i++) {
				const SynNode* post = node->getChild(i);
				if (post->getTag() == EnglishSTags::PP) {
					int j;
					for (j=0; j < post->getNChildren(); j++) {
						const SynNode* ppNode = post->getChild(j);
						if (ppNode->hasMention()) {
							Mention* ppMent = ents->getMentionSet(ment->getSentenceNumber())->getMentionByNode(ppNode);
							if (ppMent == 0)
								throw InternalInconsistencyException("EnglishGenericsFilter::_isGenericMention()",
																	 "couldn't find postmod mention in mention set");

							if (ppMent->mentionType == Mention::NAME)
								return _debugReturn(false, "np with dt in core npa, and name in postmod pp", node, ppNode);
						}
					}
				}
			}
		}
	}

	// the more broadly-reaching tests. Hypothetical and name bearing were already determined
	// for subjunctive we'll take the time to traverse the tree for each node, for now.
	// many of these tests want to know about definiteness;
	bool isDef =  _isDefinite(node);
	bool isSubj = _isInSubjunctive(node);
	// it is generic if node is indefinite and in a subjunctive node
	if (!isDef && isSubj)
		return _debugReturn(true, "indefinite node in subjunctive chain", node);

	// it is generic if node is indefinite and in a hypothetical sentence
	// e.g. "if a governer is elected" but not "if the governor is elected"
	bool isHypothetical = _is_sent_hypothetical[ment->getSentenceNumber()];
	if (!isDef && isHypothetical)
		return _debugReturn(true, "indefinite node in hypothetical sentence", node);

	// FALSE if node is partitive (wait, that's easy and already done?)
	bool hasNames = _is_sent_name_bearing[ment->getSentenceNumber()];
	// it is generic if node is indefinite and this node's sentence has no names
	if (!isDef && !hasNames)
		return _debugReturn(true, "indefinite node in sentence with no name", node);

	// this next test isn't actually all that far-reaching
	// it is specific if node is the object of a PP that is the object of something specific
	// e.g. "the son of a king"
	// the method returns the contagious node, for debugging purposes
	const SynNode* contNode = _getContagious(node, ment->getSentenceNumber(), ents);
	if (contNode != 0)
		return _debugReturn(false, "node is contagious to a specific mention", node, contNode);

	// specific if closest verbs of core are past tense
	// the method returns the verb node, for debugging purposes
	const SynNode* predNode = _getPredicate(coreNPA, ment->getSentenceNumber(), ents);
	if (predNode != 0) {
		if (predNode->getTag() == EnglishSTags::VBD ||
			(predNode->getParent() != 0 && predNode->getParent()->getHead()->getTag() == EnglishSTags::VBD))
			return _debugReturn(false, "closest predicate is past tense", node, predNode);
	}

	// generic if head is plural and no premod or postmod (bare plurals)
	if ((coreNPA->getTag() == EnglishSTags::NNS ||
		coreNPA->getTag() == EnglishSTags::NNPS) && coreNPA->getNChildren() < 2)
		return _debugReturn(true, "node is a bare plural", node, coreNPA);

	// if we've gotten this far:
	// TRUE if subjunctive, hypothetical, or no names in the sentence
	if (isSubj)
		return _debugReturn(true, "at end of tests and node is underneath subjunctive", node);
	if (isHypothetical)
		return _debugReturn(true, "at end of tests and node's sentence is hypothetical", node);
	if (!hasNames)
		return _debugReturn(true, "at end of tests and node's sentence has no names", node);

	if (_usingCommonSpecifics) {
		// FALSE if the head word is in a list of words more often specific than generic
		Symbol headWord = node->getHeadWord();
		if (_commonSpecifics->lookup(headWord))
			return _debugReturn(false, "head word is in a list of words more often than not specific", node);
	}

	// the next test is irrelevant
	// TRUE if head word is any/no body/one

	// TRUE otherwise
	return _debugReturn(true, "not otherwise classified", node);
}

// find the NPA head of an NP, or just return yourself
const SynNode* EnglishGenericsFilter::_getCoreNPA(const SynNode* node) const
{
	if (node->getTag() == EnglishSTags::NP &&
		node->getHead()->getTag() == EnglishSTags::NPA)
		return node->getHead();
	return node;
}

// look for a definite article in the first premod of the node
bool EnglishGenericsFilter::_isDefinite(const SynNode* node) const
{
	const SynNode* pre = node->getChild(0);
	// if no premod, not definite
	if (pre == 0 || pre == node->getHead())
		return false;
	if (pre->getTag() == EnglishSTags::PRPS)
		return true;
	if (pre->getTag() == EnglishSTags::DT) {
		Symbol preWord = pre->getHeadWord();
		if (preWord == EnglishWordConstants::THE ||
			preWord == EnglishWordConstants::THAT ||
			preWord == EnglishWordConstants::THIS ||
			preWord == EnglishWordConstants::THESE ||
			preWord == EnglishWordConstants::THOSE)
			return true;
	}
	return false;
}
bool EnglishGenericsFilter::_isNPish(Symbol sym) const
{
	return (
		sym == EnglishSTags::NP ||
		sym == EnglishSTags::NPA ||
		sym == EnglishSTags::NPP ||
		sym == EnglishSTags::NPPOS ||
		sym == EnglishSTags::NPPRO
		);
}
bool EnglishGenericsFilter::_isVish(Symbol sym) const
{
	return (
		sym == EnglishSTags::VBD ||
		sym == EnglishSTags::VBG ||
		sym == EnglishSTags::VBN ||
		sym == EnglishSTags::VBP ||
		sym == EnglishSTags::VBZ
		);
}

bool EnglishGenericsFilter::_isSish(Symbol sym) const
{
	return (
		sym == EnglishSTags::S ||
		sym == EnglishSTags::SINV ||
		sym == EnglishSTags::SQ
		);
}


// the node must be child of a pp, and that pp must be the postmod of a node whose head (or head path)
// is a mention of something specific
const SynNode* EnglishGenericsFilter::_getContagious(const SynNode* node, int sentNum, const EntitySet* ents)
{
	const SynNode* parent = node->getParent();
	if (parent == 0 || parent->getTag() != EnglishSTags::PP)
		return 0;
	const SynNode* grandparent = parent->getParent();
	if (grandparent == 0)
		return 0;
	// the pp shouldn't be the head of its parent
	const SynNode* part = grandparent->getHead();
	if (part == parent)
		return 0;
	const MentionSet* ments = ents->getMentionSet(sentNum);
	// find a mention in the parent chain of the part
	Mention* partMent = 0;
	while (part != 0 && _isNPish(part->getTag())) {
		partMent = ments->getMentionByNode(part);
		if (partMent == 0) {
			return 0;
		}
		if (partMent->isPopulated())
			break;
		part = part->getParent();
	}
	if (part == 0 || partMent == 0 || !partMent->isPopulated())
		return 0;
	// NOTE: assume only edt entities count.
	if (!partMent->isOfRecognizedType())
		return 0;

	// if the mention has relevant children, I'd prefer to use them
	while (partMent->getChild() != 0 && partMent->getChild()->mentionType != Mention::NONE)
		partMent = partMent->getChild();
	part = partMent->node;
	// look at the entity of this mention.
	Entity* ent = ents->getEntityByMention(ments->getMentionByNode(part)->getUID());
	// something weird if we can't get the entity
	if (ent == 0) {
		return 0; // this happens in bnews-66 and npaper sets
#if 0
		string err = "unable to find entity given node:\n";
		err.append(part->toDebugString(0).c_str());
		err.append(" when mention is of type ");
		err.append(Mention::getTypeString(partMent->mentionType));
		err.append(", ");
		err.append(partMent->getEntityType().to_debug_string());
		err.append("\n");
		throw InternalInconsistencyException("EnglishGenericsFilter::_getContagious", (char*)(err.c_str()));
#endif
	}
	// if the contagious mention is specific, so are we
	if (!ent->isGeneric())
		return part;
	return 0;
}

// find closest verbs to this node
const SynNode* EnglishGenericsFilter::_getPredicate(const SynNode* node, int sentNum, const EntitySet* ents) const
{
	return _getPredicateDriver(node, false, sentNum, ents);

}

// do the work for the above
const SynNode* EnglishGenericsFilter::_getPredicateDriver(const SynNode* node, bool goingDown, int sentNum, const EntitySet* ents) const
{
	if (node == 0)
		return 0;
	// we might be an aux. find the real verb in postmode (now descending)
	if (_isVish(node->getTag())) {
		int i;
		for(i = node->getHeadIndex()+1; i < node->getNChildren(); i++) {
			const SynNode* post = node->getChild(i);
			if (post->getTag() == EnglishSTags::VP)
				return _getPredicateDriver(post, true, sentNum, ents);
		}
		return node;
	}
	// look down vp & S chain
	if (node->getTag() == EnglishSTags::VP || _isSish(node->getTag()))
		return _getPredicateDriver(node->getHead(), true, sentNum, ents);

	// bad path to descend
	if (goingDown || node->getParent() == 0)
		return 0;
	const SynNode* parent = node->getParent();
	// pp object of a partitive np  doesn't really care about the verb
	if (node->getTag() == EnglishSTags::PP && _isNPish(parent->getTag())) {
		// checking partitive parent (info is in the mention)
		if (parent->hasMention() && ents->getMentionSet(sentNum)->getMentionByNode(parent)->mentionType == Mention::PART)
			return 0;
	}
	return _getPredicateDriver(parent, goingDown, sentNum, ents);

}


// look for numbers, an optional leading negative, and up to one decimal
bool EnglishGenericsFilter::_isNumeric(Symbol sym) const
{
	const wchar_t* symStr = sym.to_string();
	int i = 0;
	if (symStr[0] == L'-')
		i++;
	bool seenDec = false;
	while(&symStr[i] != 0) {
		if (symStr[i] == L'.') {
			if (seenDec)
				return false;
			seenDec = true;
			i++;
			continue;
		}
		if (!iswdigit(symStr[i]))
			return false;
		i++;
	}
	return true;
}


bool EnglishGenericsFilter::_debugReturn(bool decision, const char* reason, const SynNode* decNode, const SynNode* reasonNode)
{
	// don't even bother all of this if there's no output being generated
	if (!_debug.isActive())
		return decision;
	_debug << "isGeneric(): deciding ";
	if (decision)
		_debug << "TRUE";
	else
		_debug << "FALSE";
	_debug << " on : \n";
	if (decNode != 0) {
		if (_debug.isActive())
			_debug << decNode->toDebugString(2).c_str() << "\n";
	}
	else
		_debug << "(no node)\n";
	// the reason
	_debug << "===" << reason << "===\n";
	if (reasonNode != 0) {
		if (_debug.isActive()) 
			_debug << "\tbased on\n" << reasonNode->toDebugString(2).c_str() << "\n";
	}
	_debug << "\n";
	return decision;
}

// check parse in each sentence for an IN with "if" as the tag
void EnglishGenericsFilter::_fillHypothetical(bool* arr, DocTheory* thry)
{
	int i;
	for (i=0; i < thry->getNSentences(); i++) {
		const SynNode* parse = thry->getSentenceTheory(i)->getPrimaryParse()->getRoot();
		arr[i] = _fillHypotheticalSearcher(parse);
	}
}
bool EnglishGenericsFilter::_fillHypotheticalSearcher(const SynNode* node)
{
	if (!node->isPreterminal()) {
		int i;
		for (i=0; i < node->getNChildren(); i++) {
			if (_fillHypotheticalSearcher(node->getChild(i)))
				return true;
		}
	}
	if (node->getTag() == EnglishSTags::IN && node->getHeadWord() == EnglishWordConstants::IF)
		return true;
	return false;
}
// check mentions in each sentence for at least one name
void EnglishGenericsFilter::_fillNameBearing(bool* arr, DocTheory* thry)
{
	int i;
	for (i=0; i < thry->getNSentences(); i++) {
		const MentionSet* ments = thry->getSentenceTheory(i)->getMentionSet();
		int j;
		bool sawName = false;
		for (j=0; j < ments->getNMentions(); j++) {
			Mention* ment = ments->getMention(j);
			if (ment->mentionType == Mention::NAME) {
				sawName = true;
				break;
			}
		}
		arr[i] = sawName;
	}
}

// traverse nodes for subjunctive
// a subjunctive node is a vp with a md head, or a node with that in its parent
bool EnglishGenericsFilter::_isInSubjunctive(const SynNode* node)
{
	if (node == 0)
		return false;
	if (node->getTag() == EnglishSTags::VP && node->getHead()->getTag() == EnglishSTags::MD)
		return true;
	return _isInSubjunctive(node->getParent());
}



