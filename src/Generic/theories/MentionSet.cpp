
// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/MentionSet.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/NodeInfo.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"

#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLStrings.h"

MentionSet::MentionSet(const Parse *parse, int sentence_number)
	: _parse(parse), _sent_no(sentence_number), _name_score(0), _desc_score(0)
{
	_n_mentions = countReferenceCandidates(parse->getRoot());

	if (_n_mentions > MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS) {
		_n_mentions = MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS;

		SessionLogger::warn("max_sentence_mentions") <<
			"MentionSet::MentionSet(): truncating mention list because number\n"
			"mentions exceeded per-sentence limit (REAL_MAX_SENTENCE_MENTIONS)\n";
	}

	_mentions = _new Mention*[_n_mentions];
	for (int i=0; i<_n_mentions; ++i)
		_mentions[i] = 0;
	initializeMentionArray(_mentions, parse->getRoot(), 0);

	setParseNodeMentionIndices();
}

MentionSet::MentionSet(const MentionSet &other, int sent_offset, const Parse *parse)
	: _n_mentions(other._n_mentions),
	  _name_score(other._name_score), _desc_score(other._desc_score),
	  _sent_no(other._sent_no), _parse(other._parse)
{
	_mentions = _new Mention*[_n_mentions];

	for (int i = 0; i < _n_mentions; i++)
		_mentions[i] = _new Mention(this, *(other._mentions[i]));

	if (parse != NULL) {
		_sent_no += sent_offset;
		_parse = parse;
		for (int i = 0; i < _n_mentions; i++) {
			_mentions[i]->setUID(MentionUID(_mentions[i]->getUID().sentno() + sent_offset, _mentions[i]->getUID().index()));
			_mentions[i]->node = parse->getSynNode(_mentions[i]->node->getID());
		}
	}
}

MentionSet::~MentionSet() {
	for (int i=0; i<_n_mentions; ++i)
		delete _mentions[i];
	delete[] _mentions;
}


Mention *MentionSet::getMention(MentionUID ID) const {
	return getMention(Mention::getIndexFromUID(ID));
}
Mention *MentionSet::getMention(int index) const {
	if (index < _n_mentions)
		return _mentions[index];
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"MentionSet::getMention()", _n_mentions, index);
	}
}

void MentionSet::changeEntityType(MentionUID ID, EntityType type) {
	size_t i = Mention::getIndexFromUID(ID);
	if (i < (unsigned) _n_mentions) {
		_mentions[i]->setEntityType(type);
	} else throw InternalInconsistencyException::arrayIndexException(
			"MentionSet::getMention()", _n_mentions, static_cast<int>(i));

}

void MentionSet::changeMentionType(MentionUID ID, Mention::Type type) {
	size_t i = Mention::getIndexFromUID(ID);
	if (i < (unsigned) _n_mentions) {
		_mentions[i]->mentionType = type;
	} else throw InternalInconsistencyException::arrayIndexException(
			"MentionSet::getMention()", _n_mentions, static_cast<int>(i));

}


Mention *MentionSet::getMentionByNode(const SynNode *node) const {
	if (node->hasMention())
		return getMention(node->getMentionIndex());
	else
		return 0;
}

int MentionSet::countPopulatedMentions() const {
	int result = 0;
	for (int i = 0; i < _n_mentions; i++) {
		if (_mentions[_n_mentions]->isPopulated())
			result++;
	}
	return result;
}

int MentionSet::countEDTMentions() const {
	int result = 0;
	for (int i = 0; i < _n_mentions; i++) {
		if (_mentions[_n_mentions]->isPopulated() &&
			_mentions[_n_mentions]->isOfRecognizedType())
		{
			result++;
		}
	}
	return result;
}


int MentionSet::countReferenceCandidates(const SynNode *node) {
	if (node->isTerminal())
		return 0;

	int result = 0;

	if (NodeInfo::isReferenceCandidate(node))
		result++;

	for (int i = 0; i < node->getNChildren(); i++)
		result += countReferenceCandidates(node->getChild(i));

	if (result < MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS)
		return result;
	else
		return MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS;
}

int MentionSet::initializeMentionArray(Mention **mentions,
									   const SynNode *node,
									   int index)
{
	if (node->isTerminal())
		return index;

	if (NodeInfo::isReferenceCandidate(node) &&
		index < MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS)
	{
		delete mentions[index];
		mentions[index] = _new Mention(this, index, node);
		index++;
	}

	for (int i = 0; i < node->getNChildren(); i++) {
		index = initializeMentionArray(mentions, node->getChild(i),
									   index);
	}

	return index;
}

void MentionSet::setParseNodeMentionIndices() {
	for (int i = 0; i < _n_mentions; i++){
		if(const_cast<SynNode *>(_mentions[i]->node)->hasMention()){
			SessionLogger::dbg("mult_ment_0")
				<<"MentionSet::setParseNodeMentionIndices(): multiple mentions reference a single node\n"
				<<"The following mention will not be reachable with node->getMentionIndex()\n"
				<<_mentions[i]->getUID()<<" "
				<<_mentions[i]->getEntityType().getName().to_string()<<" "
				<<_mentions[i]->getMentionType()<<" "
				//<<_mentions[i]->startChar<<" "
				//<<_mentions[i]->endChar
				<<"\n";
		}
			
		const_cast<SynNode *>(_mentions[i]->node)->setMentionIndex(i);
	}
}

void MentionSet::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "Mention Set (" << _n_mentions << " mentions; "
						   << "name score: " << _name_score << "; "
						   << "desc score: " << _desc_score << "): ";

	if (_n_mentions == 0) {
		out << newline << "  (no mentions)";
	}
	else {
		for (int i = 0; i < _n_mentions; i++)
			out << newline << "- " << *_mentions[i];
	}

	delete[] newline;
}


void MentionSet::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	for (int i = 0; i < _n_mentions; i++)
		_mentions[i]->updateObjectIDTable();

	_parse->updateObjectIDTable();
}

void MentionSet::saveState(StateSaver *stateSaver) const {
	stateSaver->beginList(L"MentionSet", this);

	stateSaver->saveReal(_name_score);
	stateSaver->saveReal(_desc_score);
	if (stateSaver->getVersion() >= std::make_pair(1,6)) {
		stateSaver->savePointer(_parse);
	} else {
		stateSaver->savePointer(_parse->getRoot());
	}
	stateSaver->saveInteger(_sent_no);

	stateSaver->saveInteger(_n_mentions);
	stateSaver->beginList(L"MentionSet::_mentions");
	for (int i = 0; i < _n_mentions; i++)
		_mentions[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

MentionSet::MentionSet(StateLoader *stateLoader) {
	int id = stateLoader->beginList(L"MentionSet");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_name_score = stateLoader->loadReal();
	_desc_score = stateLoader->loadReal();
	_parse = static_cast<Parse *>(stateLoader->loadPointer());
	_sent_no = stateLoader->loadInteger();

	_n_mentions = stateLoader->loadInteger();
	_mentions = _new Mention*[_n_mentions];
	stateLoader->beginList(L"MentionSet::_mentions");
	for (int i = 0; i < _n_mentions; i++) {
		_mentions[i] = _new Mention();
		_mentions[i]->loadState(stateLoader);
	}
	stateLoader->endList();

	stateLoader->endList();
}

void MentionSet::resolvePointers(StateLoader * stateLoader) {
	if (stateLoader->getVersion() >= std::make_pair(1,6)) {
		_parse = static_cast<Parse *>(stateLoader->getObjectPointerTable().getPointer(_parse));
	} else {
		SynNode *root = static_cast<SynNode *>(stateLoader->getObjectPointerTable().getPointer(_parse));
		_parse = stateLoader->getParseByRoot(root);
	}

	for (int i = 0; i < _n_mentions; i++)
		_mentions[i]->resolvePointers(stateLoader);
}

const wchar_t* MentionSet::XMLIdentifierPrefix() const {
	return L"mentionset";
}

void MentionSet::saveXML(SerifXML::XMLTheoryElement mentionsetElem, const Theory *context) const {
	using namespace SerifXML;
	if (context != 0)
		throw InternalInconsistencyException("MentionSet::saveXML", "Expected context to be NULL");
	mentionsetElem.setAttribute(X_name_score, _name_score);
	mentionsetElem.setAttribute(X_desc_score, _desc_score);
	mentionsetElem.saveTheoryPointer(X_parse_id, _parse);

	// Assign an id to each mention before we serialize any of them,
	// since there can be pointers between mentions.
	for (int i = 0; i < _n_mentions; i++)
		mentionsetElem.generateChildId(_mentions[i]);
	// Now that each mention has an id, serialize them all.
	for (int j = 0; j < _n_mentions; ++j) 
		mentionsetElem.saveChildTheory(X_Mention, _mentions[j]);

}

MentionSet::MentionSet(SerifXML::XMLTheoryElement mentionSetElem)
: _n_mentions(0), _mentions(0), _name_score(0), _desc_score(0)
{
	using namespace SerifXML;
	mentionSetElem.loadId(this);
	_name_score = mentionSetElem.getAttribute<float>(X_name_score, 0);
	_desc_score = mentionSetElem.getAttribute<float>(X_desc_score, 0);
	_parse = mentionSetElem.loadTheoryPointer<Parse>(X_parse_id);
	_sent_no = _parse->getTokenSequence()->getSentenceNumber();

	XMLTheoryElementList mentionElems = mentionSetElem.getChildElementsByTagName(X_Mention);
	_n_mentions = static_cast<int>(mentionElems.size());

	if (_n_mentions > MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS) {
		_n_mentions = MentionUID_tag::REAL_MAX_SENTENCE_MENTIONS;

		SessionLogger::warn("max_sentence_mentions") <<
			"MentionSet::MentionSet(): truncating mention list because number\n"
			"mentions exceeded per-sentence limit (REAL_MAX_SENTENCE_MENTIONS)\n";
	}

	_mentions = _new Mention*[_n_mentions];

	for (int i=0; i<_n_mentions; ++i)
		_mentions[i] = _new Mention(mentionElems[i], this, MentionUID(_sent_no, i));
	for (int i=0; i<_n_mentions; ++i)
		_mentions[i]->resolvePointers(mentionElems[i]);

	setParseNodeMentionIndices(); 
}
