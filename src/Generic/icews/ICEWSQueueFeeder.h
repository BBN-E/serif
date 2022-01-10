// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_QUEUE_FEEDER_H
#define ICEWS_QUEUE_FEEDER_H

#include <string>
#include "Generic/driver/QueueDriver.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/icews/Stories.h"

template<class QueueDriverBase>
class ICEWSQueueFeeder: public QueueDriverBase {
public:
	ICEWSQueueFeeder(): _stories(Stories::getStoryIds()) {}

private:
	Stories::StoryIDRange _stories;

	bool hasSource() { return false; }

	bool gotQuitSignal() {
		if (_stories.first == _stories.second) {
			this->markDstAsDone();
			this->markSrcAsDone();
			return true;
		} else {
			return QueueDriverBase::gotQuitSignal();
		}
	}

	virtual QueueDriver::Element_ptr next() {
		if (_stories.first == _stories.second)
			return QueueDriver::Element_ptr();
		
		// Get the next storyid.
		StoryId storyId = *(_stories.first);
		++(_stories.first);

		std::ostringstream msg;
		msg << "Getting " << storyId.getId() << " storyid";
		SessionLogger::info("disk_queue") << msg.str();
		
		// Read the story as a document.
		Document *doc = Stories::readDocument(storyId);
		// Return it as an Element_ptr.
		DocTheory *docTheory = _new DocTheory(doc);
		std::string name = UnicodeUtil::toUTF8StdString(doc->getName().to_string());
		return boost::make_shared<QueueDriver::Element>(name, doc, docTheory);
	}
};

#endif
