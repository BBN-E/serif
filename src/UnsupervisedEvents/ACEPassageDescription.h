#ifndef _ACE_PASSAGE_DESCRIPTION_H_
#define _ACE_PASSAGE_DESCRIPTION_H_
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include "Generic/common/bsp_declare.h"
#include "GraphicalModels/pr/Constraint.h"

BSP_DECLARE(ACEPassageDescription)
class DocTheory;
class Entity;
class ACEEvent;
class ProblemDefinition;

class ACEPassageDescription {
public:
	ACEPassageDescription(const ACEEvent* event, const std::wstring& docid, 
			const DocTheory* dt, unsigned int start_sentence, 
			unsigned int end_sentence) : _docid(docid), _event(event),
	_start_sentence(start_sentence), _end_sentence(end_sentence),
	_next_mention_idx(0)
	{
		populateDumpingData(dt);
	}

	const std::wstring& docid() const { return _docid;}
	unsigned int startSentence() const { return _start_sentence; }
	unsigned int endSentence() const { return _end_sentence; }

	void toHTML(std::wostream& out, const ProblemDefinition& problem) const;
	void addEntity(const DocTheory* dt, const Entity* e,
		unsigned int entity_idx);

	std::wstring key() const;

	void freeze();

	typedef std::pair<unsigned int, unsigned int> MentionLength;

	std::vector<int> entityIDs;
private:
	typedef std::vector<std::wstring> SentTokens;
	typedef std::vector<MentionLength> TokenAnnotation;
	typedef std::vector<TokenAnnotation> SentenceAnnotation;

	std::wstring _docid;
	unsigned int _start_sentence;
	unsigned int _end_sentence;
	// raw pointer to prevent circular shared_ptr loop
	// passage descriptions should not be allowed to outlive their
	// events
	const ACEEvent* _event;

	void populateDumpingData(const DocTheory* dt);
	void mentionSuffix(std::wostream& out, unsigned int entity) const;
	void mentionTooltip(std::wostream& out, unsigned int entity,
			const ProblemDefinition& problem) const;

	std::map<int, int> _mentionToEntity;
	std::map<int, int> _mentionLengths; 
	std::vector<SentenceAnnotation> _mentionStarts;
	std::vector<SentenceAnnotation> _mentionEnds;
	std::vector<SentTokens> _tokens;
	unsigned int _next_mention_idx;
};

#endif

