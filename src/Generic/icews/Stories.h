// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_STORIES_H
#define ICEWS_STORIES_H

#include "Generic/actors/Identifiers.h"
#include "Generic/common/Attribute.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/iterator/iterator_facade.hpp>

class Document;
class DocTheory;
class DatabaseConnection;

/** Access to the ICEWS stories table. */
class Stories {
public:
	struct IteratorCore;
	typedef boost::shared_ptr<IteratorCore> IteratorCore_ptr;

	/** Read the specified story from the database, and create a new 
	* Document object for it.  The caller is responsible for 
	* deleting the returned Document. */
	static Document* readDocument(StoryId storyId);

	class StoryIDIterator; // defined below.
	typedef std::pair<StoryIDIterator, StoryIDIterator> StoryIDRange;

	/** Return an pair of iterators (begin and end) over story ids, based on
	* the ranges specified in the parameter file. */
	static StoryIDRange getStoryIds();

	/** Return the publication date for a document, using various techniques */
	static std::string getStoryPublicationDate(Document *document);

	/** Return the publication date for the given story */
	static std::string getStoryPublicationDate(StoryId storyId);

	/** Attempt to extract the story id from the docid; if no story id is
	* found, then return a null storyid. */
	static StoryId extractStoryIdFromDocId(Symbol docId);

	/** Attempt to extract the publication date from the docid; if no publication
	* date is found, then return an empty string. */
	static std::string extractPublicationDateFromDocId(Symbol docId);

	// SerifXML cache:
	static void saveCachedSerifxmlForStory(StoryId storyId, const DocTheory *docTheory);
	static std::pair<Document*, DocTheory*> loadCachedSerifxmlForStory(StoryId storyId);

	/** Iterator type for iterating over story ids. */
	class StoryIDIterator: public boost::iterator_facade<StoryIDIterator, StoryId const, boost::forward_traversal_tag, StoryId const>  {
	private:
		friend class boost::iterator_core_access;
		friend class Stories;
		explicit StoryIDIterator(IteratorCore_ptr it): _base(it) {}
		void increment() { _base->increment(); }
		bool equal(StoryIDIterator const& other) const { return _base->equal(other._base); }
		StoryId dereference() const { return _base->dereference(); }
		boost::shared_ptr<IteratorCore> _base;
	};

private:
	static void ensureStorySerifxmlTableExists(boost::shared_ptr<DatabaseConnection> icews_db);

public:
	struct IteratorCore {
		virtual StoryId dereference() = 0;
		virtual void increment() = 0;
		virtual bool equal(IteratorCore_ptr const &other) = 0;
	};
};


#endif
