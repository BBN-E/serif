#include "Generic/common/leak_detection.h"
#include "ACEEventGoldStandard.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <utility>
#include <limits>
#include <boost/foreach.hpp>
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/DocumentTable.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "LearnIt/util/FileUtilities.h"
#include "ACEPassageDescription.h"
#include "ProblemDefinition.h"
#include "ACEPassageDescription.h"
#include "ACEEvent.h"
#include "ACEEntityVariable.h"

using std::pair;
using std::make_pair;
using std::vector;
using std::string;
using std::wstringstream;
using std::stringstream;

ACEEventGoldStandard::ACEEventGoldStandard(const ProblemDefinition_ptr& problem,
		const string& gsDocTable, const string& serifDocTable) 
: _problem(problem)
{
	_gsDocTable = DocumentTable::readDocumentTable(gsDocTable);
	_serifDocTable = DocumentTable::readDocumentTable(serifDocTable);

	if (ParamReader::isParamTrue("event_debug_mode")) {
		size_t limit = 50;
		SessionLogger::info("limit_docs") << "Due to debug mode, only loading "
			<< "at most " << limit << " test documents.";
		if (limit < _gsDocTable.size()) {
			_gsDocTable.resize(limit);
		}
		if (limit < _serifDocTable.size()) {
			_serifDocTable.resize(limit);
		}
	}


	// sanity check that our correct-answer and Serif-processed document
	// tables match up
	if (_gsDocTable.size() != _serifDocTable.size()) {
		throw UnexpectedInputException("ACEEventGoldStandard::ACEEventGoldStandard",
				"Correct-answer and Serif-processed gold standard document "
				"tables must have the same size.");
	}

	for (size_t i=0; i<_gsDocTable.size(); ++i) {
		if (_gsDocTable[i].first != _gsDocTable[i].first) {
			wstringstream err;
			err << "Correct-answer and Serif-processed gold standard document "
				<<	"tables must have the same docids in the same order, but "
				<< "there is a mismatch of line " << i;
			throw UnexpectedInputException("ACEEventGoldStandard::ACEEventGoldStandard",
					err);
		}
	}

	SessionLogger::info("gold_standard_size") << "Loaded " << _gsDocTable.size()
		<< " gold standard documents";
}

bool exactlyOneEventInPassage(const DocTheory* dt, const ACEPassageDescription& passage,
		const ProblemDefinition& problem) {
	if (passage.startSentence() >= (size_t)dt->getNSentences()
			|| passage.endSentence() >= (size_t)dt->getNSentences())
	{
		wstringstream err;
		err << L"For docid " << passage.docid() << L", passage derived from "
			<< L"predicted documents has span " << passage.startSentence()
			<< L"-" << passage.endSentence() << L" but document (gold or "
			<< L"supervised being process has only " << dt->getNSentences()
			<< L"sentence.";

		throw UnexpectedInputException("exactlyOneEventInPassage", err);
	}

	unsigned int n_events = 0;
	for (size_t sent = passage.startSentence(); sent <= passage.endSentence(); ++sent) {
		const SentenceTheory* st = dt->getSentenceTheory(sent);
		const EventMentionSet* ems = st->getEventMentionSet();
		for (int event =0; event < ems->getNEventMentions(); ++event) {
			const EventMention* em = ems->getEventMention(event);
			if (em->getEventType() == problem.eventType()) {
				++n_events;
			}
		}
	}

	return n_events == 1;
}

PassageEntityMapping::PassageEntityMapping(const ACEPassageDescription& passage, 
		const DocTheory* serifTheory, const DocTheory* goldTheory)
{
	const std::vector<const Entity*> serifEntities =
		entitiesInPassage(serifTheory, passage);
	const std::vector<const Entity*> goldEntities =
		entitiesInPassage(goldTheory, passage);

	similarityMatrix = std::vector<std::vector<double> > (serifEntities.size(),
			std::vector<double>(goldEntities.size(), 0.0));

	for (size_t i=0; i<serifEntities.size(); ++i) {
		std::vector<double>& scores = similarityMatrix[i];

		for (size_t j=0; j<goldEntities.size(); ++j) {
			scores[j] = (scoreEntityAlignment(serifEntities[i],
					goldEntities[j], serifTheory, goldTheory) 
					+ scoreEntityAlignment(goldEntities[j], serifEntities[i], 
						goldTheory, serifTheory))/2.0;
		}
	}
}

double PassageEntityMapping::scoreMentionAlignment(const Mention* source,
		const Mention* target)
{
	int source_start = source->getNode()->getStartToken();
	int source_end = source->getNode()->getEndToken();
	int source_len = source_end - source_start;
	int target_start = target->getNode()->getStartToken();
	int target_end = target->getNode()->getEndToken();
	int target_len = target_end - target_start;

	double overlap = 0.0;

	if (target_start > source_end || target_end < source_start) {
		overlap = 0;
	}  else { // at least some overlap
		if ((target_start >= source_start && target_end <= source_end) 
				|| (source_start >= target_start && source_end <=target_end))
		{
			// complete overlap, return the smaller
			overlap = (std::min)(target_len, source_len);
		} else { // partial overlap
			if (source_start<= target_end && target_end <= source_end) {
				overlap = target_end - source_start;
			} else if (target_start <=source_end && source_end <=target_end) {
				overlap = source_end - target_start;
			} else {
				throw UnexpectedInputException("scoreMentionAlignment",
						"Unhandled case");
			}
		}
	}

	return 2.0*overlap / (source_len + target_len);
}

const std::vector<const Entity*> PassageEntityMapping::entitiesInPassage(const DocTheory* dt,
		const ACEPassageDescription& passage) 
{
	std::vector<const Entity*> ret;
	const EntitySet* es = dt->getEntitySet();
	for (int ent =0; ent<es->getNEntities(); ++ent) {
		const Entity* e = es->getEntity(ent);
		if (ACEEvent::foundInSentenceRange(e, 
					std::make_pair(passage.startSentence(), passage.endSentence())))
		{
			ret.push_back(e);
		}
	}
	return ret;
}

double PassageEntityMapping::scoreEntityAlignment(const Entity* source,
		const Entity* target, const DocTheory* sourceDT, const DocTheory* targetDT)
{
	double sum = 0.0;
	for (int i=0; i<source->getNMentions(); ++i) {
		double best_score = 0.0;
		for (int j=0; j<target->getNMentions(); ++j) {
			best_score = (std::max)(best_score,
				scoreMentionAlignment(sourceDT->getMention(source->getMention(i)),
				targetDT->getMention(target->getMention(j))));
		}
		sum += best_score;
	} 
	return sum / source->getNMentions();
}

GoldAnswers_ptr getGoldAnswers(const DocTheory* goldTheory,
		const ACEPassageDescription& passage, const ProblemDefinition& problem)
{
	std::vector<const Entity*> goldEntities = PassageEntityMapping::entitiesInPassage(goldTheory, 
			passage);
	GoldAnswers_ptr ret = boost::make_shared<GoldAnswers>(goldEntities.size());

	for (size_t i=0; i<goldEntities.size(); ++i) {
		(*ret)[i] = ACEEntityVariable::getFirstGoldLabel(goldTheory, goldEntities[i], problem,
				passage.startSentence(), passage.endSentence());
	}

	return ret;
}

std::vector<GoldStandardACEInstance> ACEEventGoldStandard::goldStandardInstances(
		size_t fileIdx)
{
	std::vector<GoldStandardACEInstance> ret;
	std::pair<Document*, DocTheory*> serifDocPair;
	std::pair<Document*, DocTheory*> correctDocPair;

	if (_serifDocTable[fileIdx].first != _gsDocTable[fileIdx].first) {
		stringstream err;
		err << "File mismatch for index " << fileIdx << "; gold has "
			<< _gsDocTable[fileIdx].first << " but predicted has "
			<< _serifDocTable[fileIdx].first;
		throw UnexpectedInputException("ACEEventGoldStandard::goldStandardInstances",
				err.str().c_str());
	}

	try {
		serifDocPair =
			SerifXML::XMLSerializedDocTheory(_serifDocTable[fileIdx].second.c_str())
			.generateDocTheory();
	} catch (UnexpectedInputException& e) {
		SessionLogger::warn("swalled_file_read_exception")
			<< "Swallowed exception " << e.getMessage() << " while reading "
			<< "predicted file " << _serifDocTable[fileIdx].second 
			<< "; skipping. RESULTS WILL BE INVALID.";
	}

	try{
		correctDocPair = SerifXML::XMLSerializedDocTheory(_gsDocTable[fileIdx].second.c_str())
			.generateDocTheory();
	} catch (UnexpectedInputException& e) {
		SessionLogger::warn("swalled_file_read_exception")
			<< "Swallowed exception " << e.getMessage() << " while reading "
			<< "gold file " << _gsDocTable[fileIdx].second
			<< "; skipping. RESULTS WILL BE INVALID.";
	}

	if (serifDocPair.second && correctDocPair.second) {
		if (serifDocPair.second->getNSentences() == correctDocPair.second->getNSentences()) {
			// we only count as instances those places where the Serif processed version
			// found an event which was present in the gold standard, since all other
			// cases we couldn't possibly get correct
			std::vector<ACEEvent_ptr> serifEvents =
				ACEEvent::createFromDocument(serifDocPair.first->getName(),
						serifDocPair.second, *_problem);
			BOOST_FOREACH(const ACEEvent_ptr& serifEvent, serifEvents) {
				if (exactlyOneEventInPassage(serifDocPair.second, serifEvent->passage(),*_problem)
						&& exactlyOneEventInPassage(correctDocPair.second, serifEvent->passage(), *_problem))
				{
					PassageEntityMapping_ptr pem = boost::make_shared<PassageEntityMapping>(
							serifEvent->passage(), serifDocPair.second,
							correctDocPair.second);
					GoldAnswers_ptr goldAnswers = getGoldAnswers(
							correctDocPair.second, serifEvent->passage(),
							*_problem);
					ret.push_back(GoldStandardACEInstance(serifEvent, pem, goldAnswers));
				}
			}
		} else {
			SessionLogger::warn("sentence_breaking_mismatch") << L"For docid "
				<< _gsDocTable[fileIdx].first << L", gold has "
				<< correctDocPair.second->getNSentences() << L" sentences but "
				<< L"Serif-processed document has "
				<< serifDocPair.second->getNSentences();
			throw SkipThisDocument();
		}
	}

	return ret;
}


