#include "Generic/common/leak_detection.h"
#include "ACEEntityVariable.h"

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EventSet.h"
#include "Generic/theories/EventMentionSet.h"
#include "Generic/theories/Event.h"
#include "Generic/theories/Mention.h"

#include "ProblemDefinition.h"


void ACEEntityVariable::setFromGold(const DocTheory* dt, const Entity* e,
		const ProblemDefinition& problem, unsigned int start_sentence,
		unsigned int end_sentence)
{
	_gold = getFirstGoldLabel(dt, e, problem, start_sentence, end_sentence);
}

int ACEEntityVariable::getFirstGoldLabel(const DocTheory* dt, const Entity* e,
		const ProblemDefinition& problem, unsigned int start_sentence, 
		unsigned int end_sentence)
{
	for (unsigned int i=start_sentence; i<=end_sentence; ++i) {
		const EventMentionSet* ems = dt->getSentenceTheory(i)->getEventMentionSet();

		for (int j = 0; j<ems->getNEventMentions(); ++j) {
			const EventMention* em = ems->getEventMention(j);
			if (em->getEventType() == problem.eventType()) {
				for (int k=0; k<em->getNArgs(); ++k) {
					const Mention* m = em->getNthArgMention(k);
					const Entity* evE = dt->getEntitySet()->getEntityByMention(m->getUID());
					if (evE && evE == e) {
						std::wstring argString = em->getNthArgRole(k).to_string();
						try {
							return problem.classNumber(argString);
						} catch (UnexpectedInputException&) {
							SessionLogger::warn("unknown_role") << L"Unknown role: " 
								<< argString;
						}
					}
				}
			}
		}
	}

	return -1;
}

