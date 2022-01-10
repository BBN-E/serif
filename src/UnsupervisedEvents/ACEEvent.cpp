#include "Generic/common/leak_detection.h"
#include <sstream>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/DocumentTable.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/PropTree/PropForestFactory.h"
#include "English/parse/en_LanguageSpecificFunctions.h"
#include "../GraphicalModels/Alphabet.h"
#include "../GraphicalModels/pr/Constraint.h"
#include "../GraphicalModels/distributions/MultinomialDistribution.h"
#include "ACEEvent.h"
#include "ACEEntityFactorGroup.h"
#include "ACEPassageDescription.h"
#include "ProblemDefinition.h"

using std::wstringstream;
using std::wostream;
using std::wstring;
using boost::make_shared;
using boost::shared_ptr;
using std::max_element;
using std::vector;
using GraphicalModel::Alphabet;
using GraphicalModel::SymmetricDirichlet;
using GraphicalModel::SymmetricDirichlet_ptr;

unsigned int ACEEvent::_next_index = 0;

std::vector<ACEEvent_ptr> ACEEvent::createFromDocument(const Symbol& docid, 
		const DocTheory* dt, const ProblemDefinition& problem, 
		bool keep_passage)
{
	if (problem.maxPassageSize() != 1) {
		throw UnexpectedInputException("ACEEvent::createFromDocument",
				"Currently only single sentence passages are allowed.");
	}

	std::vector<ACEEvent_ptr> ret;
	std::vector<SentenceRange> sentences;

	if (ParamReader::getOptionalTrueFalseParamWithDefaultVal("first_doc_event_only", false)) {
		try {
			sentences.push_back(getBestSentenceRange(dt, problem));
		} catch (NoEventFound&) { }
	} else {
		sentences = sentencesWithEvents(dt, problem.eventType());
	}

	BOOST_FOREACH(SentenceRange passage, sentences) {
		ret.push_back(createFromPassage(docid, dt, problem, passage));
	}
	return ret;
}

ACEEvent_ptr ACEEvent::createFromPassage(const Symbol& docid,
		const DocTheory* dt, const ProblemDefinition& problem,
		const SentenceRange& passage) 
{
	ACEEvent_ptr eventPassage = make_shared<ACEEvent>(_next_index++);
	// don't want to do this in constructor to avoid passing pointer to
	// not-yet-intitialized ACEEvent object
	eventPassage->_passage = make_shared<ACEPassageDescription>(eventPassage.get(),
			docid.to_string(), dt, passage.first, passage.second);

	DocPropForest_ptr forest = PropForestFactory::getDocumentForest(dt);
	const EntitySet* es = dt->getEntitySet();
	for (int ent = 0; ent < es->getNEntities(); ++ent) {
		const Entity* e = es->getEntity(ent);
		if (foundInSentenceRange(e, passage)) {
			eventPassage->addEntity(dt, *forest, passage, e, problem);
		}
	}

	eventPassage->calculateOrderByMention(dt, *eventPassage->_passage);
	eventPassage->_passage->freeze();
	return eventPassage;
}

void ACEEvent::calculateOrderByMention(const DocTheory* dt, 
		const ACEPassageDescription& passage)
{
	orderByMention.clear();
	for (size_t s = passage.startSentence(); s<=passage.endSentence(); ++s) {
		const SentenceTheory* st = dt->getSentenceTheory(s);
		const MentionSet* ms = st->getMentionSet();

		// by construction mentions are ordered by a pre-order traversal
		// of the parse tree

		for (int m_idx = 0; m_idx < ms->getNMentions(); ++m_idx) {
			const Mention* m = ms->getMention(m_idx);
			const Entity* e = m->getEntity(dt);
			if (e) {
				std::vector<int>::const_iterator probe = 
					std::find(passage.entityIDs.begin(),
							passage.entityIDs.end(), e->getID());

				if (probe == passage.entityIDs.end()) {
					throw UnrecoverableException("ACEEvent::calculateOrderByMention",
							"Cannot find index for entity");
				}

				size_t entity_idx = std::distance(passage.entityIDs.begin(), probe);

				if (std::find(orderByMention.begin(), orderByMention.end(),
							entity_idx) == orderByMention.end())
				{
					orderByMention.push_back(entity_idx);
				}
			}
		}
	}

	if (orderByMention.size() != nEntities()) {
		std::stringstream err;
		err << "There are " << nEntities() << " entities, but the list "
				<< "ordered by first mention has only " 
				<< orderByMention.size() << ". This should be impossible.";
		throw UnrecoverableException("ACEEvent::calculateOrderByMention", err.str());
	}
}

ACEEvent::ACEEvent(unsigned int idx) : _idx(idx)
{ 
}

int ACEEvent::gold(unsigned int entity_idx) const {
	return _entityVariables[entity_idx]->gold();
}

void ACEEvent::addEntity(const DocTheory* dt, const DocPropForest& forest, 
		SentenceRange passage, const Entity* e, const ProblemDefinition& problem)
{
	ACEEntityVariable_ptr entityVar = 
		make_shared<ACEEntityVariable>(problem.nClasses());
	entityVar->setFromGold(dt, e, problem, passage.first, passage.second);
	ACEEntityFactorGroup_ptr entityFactors = 
		make_shared<ACEEntityFactorGroup>(dt, forest, passage, e, entityVar);
	entityFactors->connectToVar(*entityVar);
	unsigned int idx = _entityVariables.size();
	_entityVariables.push_back(entityVar);
	_entityFactors.push_back(entityFactors);



	_passage->addEntity(dt, e, idx);
}

void ACEEvent::finishedLoading(const ProblemDefinition& problem)
{
	ACEEntityFactorGroup::finishedLoading(problem);
}

void ACEEvent::initializeRandomly() {
	BOOST_FOREACH(const ACEEntityVariable_ptr& v, _entityVariables) {
		std::vector<double> tmp(v->nValues());
		double x = 1.0/v->nValues();
		BOOST_FOREACH(double& val, tmp) {
			val = x;
		}
		v->setMarginals(tmp);
	}
}

void ACEEvent::observeImpl() {
	for (unsigned int i=0; i<_entityVariables.size(); ++i) {
		_entityFactors[i]->observe(*_entityVariables[i]);
	}
}

void ACEEvent::clearCounts() {
	ACEEntityFactorGroup::clearCounts();
}

// find the 5 sentence window with the highest concentration of
// attack triggers
SentenceRange ACEEvent::getBestSentenceRange(const DocTheory* dt,
		const ProblemDefinition& problem) 
{
	if (dt->getNSentences() < 1) {
		throw NoEventFound();
	}

	std::vector<unsigned int> attacks(dt->getNSentences());

	for (int sent = 0; sent < dt->getNSentences(); ++sent) {
		bool has_event = false;
		const SentenceTheory* st = dt->getSentenceTheory(sent);
		const EventMentionSet* ems = st->getEventMentionSet();
		for (int event = 0; event < ems->getNEventMentions(); ++event) {
			const EventMention* em = ems->getEventMention(event);
			has_event = has_event || em->getEventType() == problem.eventType();
		}
		attacks[sent] = has_event?1:0;
	}

	if ((unsigned int)dt->getNSentences() <= problem.maxPassageSize()) {
		bool keep = false;
		for (int sent =0; sent<dt->getNSentences(); ++sent) {
			if (attacks[sent]>0) {
				keep = true;
			}
		}
		if (keep) {
			return SentenceRange(0, dt->getNSentences() - 1);
		} else {
			throw NoEventFound();
		}
	} else {
		size_t sz = problem.maxPassageSize();
		std::vector<unsigned int> scores(dt->getNSentences());
		for (size_t sent =0; sent<attacks.size()-sz; ++sent) {
			for (size_t curSent = sent; curSent < attacks.size() && curSent < sent + sz; ++curSent) {
				scores[sent] += attacks[curSent];
			}
		}

		vector<unsigned int>::const_iterator it =
			max_element(scores.begin(), scores.end());

		if (*it > 0) {
			int base = it-scores.begin();
			return SentenceRange(base, base+sz-1);
		} else {
			throw NoEventFound();
		}
	}
}

std::vector<SentenceRange> ACEEvent::sentencesWithEvents(const DocTheory* dt,
		const Symbol& eventType)
{
	std::vector<SentenceRange> ret;
	for (int sent = 0; sent<dt->getNSentences(); ++sent) {
		const SentenceTheory* st = dt->getSentenceTheory(sent);
		const EventMentionSet* ems = st->getEventMentionSet();
		for (int event = 0; event < ems->getNEventMentions(); ++event) {
			const EventMention* em = ems->getEventMention(event);
			if (em->getEventType() == eventType) {
				ret.push_back(SentenceRange(sent, sent));
				break;
			}
		}
	}
	return ret;	
}


bool ACEEvent::foundInSentenceRange(const Entity* e, SentenceRange eventPassage) {
	for (int i =0; i<e->getNMentions(); ++i) {
		int sn = Mention::getSentenceNumberFromUID(e->getMention(i));
		if (sn  >= eventPassage.first && sn <=eventPassage.second)
		{
			return true;
		}
	}
	return false;
}

void ACEEvent::debugInfo(unsigned int idx, const ProblemDefinition& problem,
		std::wostream& out) const 
{
	assert (idx >= 0 && idx < _entityVariables.size());
	const vector<double>& marginals = probsOfEntity(idx);
	assert(problem.nClasses() == marginals.size());

	out << L"<table><thead><tr><th>RT</th>";
	for (size_t i=0; i<marginals.size(); ++i) {
		out << L"<th>"<<problem.className(i) << L"</th>";
	}
	out << L"</thead><tbody>";
	out << L"<tr><td><b>Mgnls</b></td>";
	for (size_t i =0; i<marginals.size(); ++i) {
		out << L"<td>" << std::setiosflags(std::ios::fixed) 
			<< std::setprecision(1) << 100.0 * marginals[i] << L"</td>";
	}
	out << L"</tr>";

	_entityFactors[idx]->debugInfo(problem, out);	
	out << L"</tbody></table>" << std::endl;
}

void ACEEvent::instanceDebugInfo(const ProblemDefinition& problem, std::wostream& out) const
{
	int constIdx = 0;
	BOOST_FOREACH(const GraphicalModel::Constraint<ACEEvent>::ptr& con, 
			problem.instanceConstraints()->forInstance(_idx))
	{
		out << L"<b>" << problem.instanceConstraints()->constraintName(constIdx) 
			<< L"[</b>";
		con->dumpDebugInfo(out);
		out << L"<b>]</b>; ";
		++constIdx;
	}
}

double ACEEvent::logZ(const ACEEvent::ACEEventConstraints& constraints,
		const ACEEvent::ACEEventConstraints& instanceConstraints) const 
{
	double Z = 0.0;
	std::vector<double> scores;
	if (_entityVariables.size() > 0) {
		scores.resize(_entityVariables[0]->nValues());
	}

	for (unsigned int i=0; i<_entityVariables.size(); ++i) {
		const ACEEntityVariable& var = *_entityVariables[i];
		for (unsigned int y =0; y< var.nValues(); ++y) {
			scores[y] = log(var.marginals()[y]);
		}
		BOOST_FOREACH(const ACEEvent::ACEEventConstraint_ptr& constraint, constraints) {
			constraint->contributeToLogZ(*this, i, scores);
		}
		BOOST_FOREACH(const ACEEvent::ACEEventConstraint_ptr& constraint, instanceConstraints) {
			constraint->contributeToLogZ(*this, i, scores);
		}
		Z+=logSumExp(scores);
	}

	return Z;
}
	
void ACEEvent::loadFromDocTable(const std::string& docTableFile,
		const ProblemDefinition& problem, 
		GraphicalModel::DataSet<ACEEvent>& events, int max_docs)
{
	DocumentTable::DocumentTable docTable = 
		DocumentTable::readDocumentTable(docTableFile);

	SessionLogger::info("event_type") << L"Loading passages with ACE events of "
		<< L"type '" << problem.eventType() << L"' from " << docTableFile;

	if (ParamReader::isParamTrue("event_debug_mode")) {
		max_docs = 50;
		SessionLogger::info("limiting_docs") << "Limiting number of documents "
			<< " read to " << max_docs << " because of debug mode.";
	}

	if (max_docs >= 0 && static_cast<size_t>(max_docs) < docTable.size()) {
		SessionLogger::info("limiting_docs") << "Document table has size " 
			<< docTable.size() << " but we are only using the first "
			<< max_docs << " documents.";
		docTable.resize(max_docs);
	}

	size_t with_events = 0;

	for (size_t start = 0; start<docTable.size(); start+=100) {
		unsigned int end = (std::min)(start+100, docTable.size());
		SessionLogger::info("file_load_progress") << "Loading documents "
			<< start << " to " << (end-1);
		for (unsigned int idx = start; idx<end; ++idx) {
			DocumentTable::DocumentPath docFile = docTable[idx];
			std::string filename = docFile.second;

			// Load the SerifXML and get the DocTheory
			try {
				std::pair<Document*, DocTheory*> doc_pair = 
					SerifXML::XMLSerializedDocTheory(filename.c_str()).generateDocTheory();
				if (!doc_pair.second)
					continue;
				wstring docid = doc_pair.first->getName().to_string();
				std::vector<ACEEvent_ptr> eventsInDoc =
					ACEEvent::createFromDocument(docid, doc_pair.second, problem);
				if (!eventsInDoc.empty()) {
					BOOST_FOREACH(const ACEEvent_ptr& event, eventsInDoc) {
						events.addGraph(event);
					}
					++with_events;
				}
			} catch (UnexpectedInputException& e) {
				SessionLogger::warn("swallowed_file_read_exception")
					<< "Swallowed exception " << e.getMessage() << " while reading "
					<< filename << "; skipping";
			}
		}
	}

	SessionLogger::info("docs_with_events") << "Read " << events.graphs.size()
		<< "events of interest in " << with_events << " documents out of "
		<< docTable.size() << " total documents supplied.";
}

void ACEEvent::findConflictingConstraints(
		std::vector<GraphicalModel::Constraint<ACEEvent>* >& conflictingConstraints)  const
{
	for (unsigned int j=0; j<nVariables(); ++j) {
		factors(j).findConflictingConstraints(conflictingConstraints);
	}
}

void ACEEvent::dumpGraph() const {
	std::wstringstream dump;

	dump << "Graph for " << passage().docid() << L"("
		<< passage().startSentence() << L", " << passage().endSentence()
		<< L")\n";

	bool first = true;
	for (size_t varIdx = 0; varIdx < nEntities(); ++varIdx) {
		const ACEEntityVariable& var = *_entityVariables[varIdx];
		if (first) {
			first = false;
			for (size_t clsIdx = 0; clsIdx < var.nValues(); ++clsIdx) {
				dump << std::setw(6) << clsIdx;
			}
			dump << L"\n";
		}

		const std::vector<double>& marginals = var.marginals();
		for (size_t clsIdx = 0; clsIdx < var.nValues(); ++clsIdx) {
			dump << std::setw(6) 
				<< std::fixed << std::setprecision(0) << 100.0*marginals[clsIdx];
		}
		dump << L"\n";
	}
	dump << L"\n";

	SessionLogger::info("dump_graph") << dump.str();
}


