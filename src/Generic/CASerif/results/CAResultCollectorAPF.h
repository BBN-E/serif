// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CA_APF_RESULT_COLLECTOR_H
#define CA_APF_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/theories/Mention.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"
#include "Generic/CASerif/correctanswers/CorrectMention.h"

class DocTheory;
class Entity;
class EntitySet;
class RelationSet;
class Relation;
class RelMention;
class SynNode;
class TokenSequence;
class UTF8OutputStream;


class CAResultCollectorAPF : public ResultCollector {
public:
	CAResultCollectorAPF() 
		: _docTheory(0), _entitySet(0), _relationSet(0), _tokenSequence(0) {}

	~CAResultCollectorAPF()
	{	finalize(); }

	virtual void loadDocTheory(DocTheory *docTheory);

	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t *document_filename);

	virtual void produceOutput(std::wstring *results) { ResultCollector::produceOutput(results); }

private:

	void finalize() {
		delete [] _tokenSequence;
		_tokenSequence = 0;
	}

	/***** utility apf methods *****/

	// some constants that apf uses to describe mentions
	const wchar_t* _convertMentionTypeToAPF(Mention* ment);

	// heads the document (write to stream) representing doc_source and 
	// called doc_name
	void _printAPFDocumentHeader(UTF8OutputStream& stream,
								 const wchar_t* doc_name=L"DUMMY_NAME",
								 const wchar_t* doc_source=L"DUMMY_SOURCE");

	// begin info for ent, writing to stream, with generic flag and doc name as indicated
	void _printAPFEntityHeader(UTF8OutputStream& stream, 
							   Entity* ent, 
							   bool isGeneric, 
							   const wchar_t* doc_name=L"DUMMY_NAME");
	// begin info for attributes section. write to stream
	void _printAPFAttributeHeader(UTF8OutputStream& stream);

	// info for men, a mention of ent, writing to stream
	void _printAPFMention(UTF8OutputStream& stream,
						  Mention* ment,
						  Entity* ent);

	void _printCorrectMention(UTF8OutputStream& stream,
							 CorrectMention* ment,
							 Entity* ent);

	// info for a name mention, as printed in attributes
	void _printAPFNameAttribute(UTF8OutputStream& stream,
								Mention* ment);

	// prints text span and character offset
	void _printAPFMentionExtent(UTF8OutputStream& stream,
								const SynNode* node,
								int sentNum);

	void _printCorrectMentionExtent(UTF8OutputStream& stream,
									CorrectMention* cm,
									int sentNum);

	void _printCorrectMentionHeadExtent(UTF8OutputStream& stream,
									CorrectMention* cm,
									int sentNum);

	void _printCorrectMentionText(UTF8OutputStream& stream, 
		CorrectMention *cm,
		int sentNum);

	void _printCorrectMentionHeadText(UTF8OutputStream& stream, 
		CorrectMention *cm,
		int sentNum) ;


	// RELATIONS
	void _printAPFRelationHeader(UTF8OutputStream& stream, 
							   Relation* rel, 
							   const wchar_t* doc_name=L"DUMMY_NAME");
	void _printAPFRelMention(UTF8OutputStream& stream, 
							   RelMention* ment,
							   Relation* rel);
	void _printCorrectRelMention(UTF8OutputStream& stream,
								 RelMention* ment,
								 Relation* rel);

	void _printAPFNodeText(UTF8OutputStream& stream, 
		const SynNode* node,
		int sentNum);

	void _printAPFNodeText(UTF8OutputStream& stream, 
		int sentNum,
		int startTok,
		int endTok,
		EDTOffset start,
		EDTOffset end);


	// cleanup - print footers
	void _printAPFAttributeFooter(UTF8OutputStream& stream);
	void _printAPFEntityFooter(UTF8OutputStream& stream);
	void _printAPFRelationFooter(UTF8OutputStream& stream);
	void _printAPFDocumentFooter(UTF8OutputStream& stream);

	/***** utility mention information methods *****/

	// is this the top mention of this entity (i.e. not a child of something structured)
	bool _isTopMention(Mention* ment, EntityType type);

	// is this mention syntactically inside a name mention that is not a GPE?
	bool _isNestedInsideOfName(Mention* ment);
	bool _isNestedDriver(const SynNode* node, int sentNum);

	// is this mention the second part of an appositive?
	// that is, the latter mention of two
	bool _isSecondPartOfAppositive(Mention* ment);

	// is the parent of this mention a GPE with a person role?
	bool _isGPEModifierOfPersonGPE(Mention* ment);

	// is this a name, and is its parent's head not it, and is its
	// parent a different entity? (wordy, I know)
	bool _isNameNotInHeadOfParent(Mention* ment, EntityType type);

	// the data we need
	// may as well save the whole doc theory
	DocTheory *_docTheory;
	const EntitySet *_entitySet;
	const RelationSet *_relationSet;	
	CorrectDocument *_correctDocument;

	// indexed by sentence
	const TokenSequence **_tokenSequence;

};

#endif
