#include "Generic/common/leak_detection.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <boost/foreach.hpp>
#include "Generic/theories/EntitySet.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/SynNode.h"
#include "ACEPassageDescription.h"
#include "ACEEvent.h"
#include "ACEEventDumper.h"
#include "Sparkline.h"
#include "ProblemDefinition.h"

using std::make_pair;
using std::wostream;
using std::endl;
using std::vector;

void ACEPassageDescription::populateDumpingData(const DocTheory* dt) {
	assert(dt);
	for (unsigned int sent=_start_sentence; sent<=_end_sentence; ++sent) {
		SentTokens sentTokens;
		assert(sent < (unsigned int)dt->getNSentences());
		const TokenSequence* ts = dt->getSentenceTheory(sent)->getTokenSequence();
		for (int tok = 0; tok < ts->getNTokens(); ++tok) {
			sentTokens.push_back(ts->getToken(tok)->getSymbol().to_string());
		}
		_tokens.push_back(sentTokens);
		_mentionStarts.push_back(SentenceAnnotation(sentTokens.size(), TokenAnnotation()));
		_mentionEnds.push_back(SentenceAnnotation(sentTokens.size(), TokenAnnotation()));
	}
}

struct by_second {
	bool operator()(const ACEPassageDescription::MentionLength& a, 
			const ACEPassageDescription::MentionLength& b)
	{
		return a.second > b.second;
	}
};

void ACEPassageDescription::freeze() {
	BOOST_FOREACH(SentenceAnnotation& sentAnn, _mentionStarts) {
		BOOST_FOREACH(TokenAnnotation& tokAnn, sentAnn) {
			std::sort(tokAnn.begin(), tokAnn.end(), by_second());
		}
	}

	BOOST_FOREACH(SentenceAnnotation& sentAnn, _mentionEnds) {
		BOOST_FOREACH(TokenAnnotation& tokAnn, sentAnn) {
			std::sort(tokAnn.begin(), tokAnn.end(), by_second());
		}
	}
}

unsigned int mentionLength(const Mention* m) {
	const SynNode* s = m->getNode();
	return s->getEndToken() - s->getStartToken();
}

void ACEPassageDescription::addEntity(const DocTheory* dt, const Entity* e,
		unsigned int entity_idx) 
{
	entityIDs.push_back(e->getID());
	for (int i=0; i<e->getNMentions(); ++i) {
		MentionUID m_id = e->getMention(i);
		unsigned int sn = Mention::getSentenceNumberFromUID(m_id);
		if (sn >= _start_sentence && sn <=_end_sentence) {
			const Mention* m = 
				dt->getSentenceTheory(sn)->getMentionSet()->getMention(m_id);
			assert(m);
			const SynNode* s = m->getNode();
			assert(s);
			int start_tok = s->getStartToken();
			int end_tok = s->getEndToken();
			unsigned int idx = _next_mention_idx++;

			_mentionToEntity.insert(make_pair(idx,entity_idx));
			_mentionLengths.insert(make_pair(idx,end_tok-start_tok));

			unsigned int relative_sn = sn - _start_sentence;
			assert (relative_sn >=0);
			assert(relative_sn < _mentionStarts.size());
			assert(relative_sn < _mentionEnds.size());
			assert(start_tok >= 0);
			assert((size_t)start_tok < _mentionStarts[relative_sn].size());
			assert((size_t)end_tok < _mentionEnds[relative_sn].size());
					
			_mentionStarts[relative_sn][start_tok].push_back(
					make_pair(idx, mentionLength(m)));
			_mentionEnds[relative_sn][end_tok].push_back(
					make_pair(idx, mentionLength(m)));
		}
	}
}

void ACEPassageDescription::toHTML(wostream& out, 
		const ProblemDefinition& problem) const
{
	out << L"\t\t<div class = 'passage'>" << endl;
	out << L"\t\t\t<h3>" << _docid << L" (" << _start_sentence
		<< L", " << _end_sentence << L") </h3>" << endl;

	_event->instanceDebugInfo(problem, out);

	out << L"<div class = 'passageText'>"<< endl;
	for (size_t sent = 0; sent < _mentionStarts.size(); ++sent) {
		const SentTokens& toks = _tokens[sent];
		const SentenceAnnotation& starts = _mentionStarts[sent];
		const SentenceAnnotation& ends = _mentionEnds[sent];

		for (size_t i=0; i<toks.size(); ++i) {
			for (size_t j=0; j<starts[i].size(); ++j) {
				unsigned int mention_idx = starts[i][j].first;
				unsigned int entity = _mentionToEntity.find(mention_idx)->second;
				int gold = _event->gold(entity);
				if (gold!=-1) {
					out << L"<b><font color='" << ACEEventDumper::color(gold)
						<< "'>";
				}
				out << L"<span class='mention' title='";
				mentionTooltip(out, entity, problem);
				out << "'>[";
			}
			out << toks[i];
		//	for (int j=0; j<ends[i].size(); ++j) {
			for (int j=ends[i].size()-1; j>=0; --j) {
				unsigned int mention_idx = ends[i][j].first;
				unsigned int entity = _mentionToEntity.find(mention_idx)->second;
				out << L"]";
				mentionSuffix(out, entity);
				out << L"</span>";
				int gold = _event->gold(entity);
				if (gold!=-1) {
					out << L"</font></b>";
				}
			}

			out << L" ";
		}
	}
	out << L"</div></div>";
}

void ACEPassageDescription::mentionSuffix(wostream& out, 
		unsigned int entity) const 
{
	vector<wstring> colorMap;

	assert (entity < _event->nEntities());
	const vector<double>& marginals = _event->probsOfEntity(entity);

	if (marginals.size() > ACEEventDumper::nColors()) {
		throw UnexpectedInputException("ACEPassageDescription::mentionSuffix",
				"More argument types than available colors");
	}
	for (size_t i=0; i<marginals.size(); ++i) {
		colorMap.push_back(ACEEventDumper::color(i));
	}

	out << L"<sub>" << entity << L"</sub>";
	out << SparkBar(marginals, 0.0, 1.0, 20, 3, 1, colorMap, true);
}

void ACEPassageDescription::mentionTooltip(wostream& out,
		unsigned int entity, const ProblemDefinition& problem) const 
{
	_event->debugInfo(entity, problem, out);
}

std::wstring ACEPassageDescription::key() const {
	std::wstringstream s;
	s << docid() << L"-" << startSentence() << L"-" << endSentence();
	return s.str();
}

