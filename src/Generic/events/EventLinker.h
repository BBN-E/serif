// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_LINKER_H
#define EVENT_LINKER_H

#include <boost/shared_ptr.hpp>


class EventMentionSet;
class EventSet;
class EntitySet;
class PropositionSet;
class CorrectAnswers;

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/Symbol.h"

class EventLinker {
public:
	/** Create and return a new EventLinker. */
	static EventLinker *build() { return _factory()->build(); }
	/** Hook for registering new EventLinker factories */
	struct Factory { virtual EventLinker *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~EventLinker() {}

	virtual void linkEvents(const EventMentionSet *eventMentionSet, 
		EventSet *eventSet,
		const EntitySet *entitySet,
		const PropositionSet *propSet,
		CorrectAnswers *correctAnswers,
		int sentence_num,
		Symbol docname) = 0;

	void linkEvents(const EventMentionSet *eventMentionSet, 
					EventSet *eventSet,
					const EntitySet *entitySet,
					const PropositionSet *propSet,
					int sentence_num,
					Symbol docname) {
		linkEvents(eventMentionSet, eventSet, entitySet, propSet, 
				   0, sentence_num, docname);
	}
private:
	static boost::shared_ptr<Factory> &_factory();
};

//#ifdef ENGLISH_LANGUAGE
//	#include "English/events/en_EventLinker.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/events/ch_EventLinker.h"
//#else
//	#include "Generic/events/xx_EventLinker.h"
//#endif



#endif
