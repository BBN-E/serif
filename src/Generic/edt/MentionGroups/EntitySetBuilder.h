// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_SET_BUILDER_H
#define ENTITY_SET_BUILDER_H

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include <Generic/edt/MentionGroups/MentionGroup.h>

// Forward declarations.
class DocTheory;
class EntitySet;
class EntityType;
class LinkInfoCache;
class Mention;
class MentionGroupConfiguration;
class MentionGroupConstraint;
class MentionGroupMerger;

/**
  *  Directs the construction of a new EntitySet.
  */
class EntitySetBuilder {
public:
	EntitySetBuilder();
	~EntitySetBuilder();

	/** Return a new EntitySet given the information contained in docTheory */
	EntitySet* buildEntitySet(DocTheory * docTheory) const;

private:
	boost::scoped_ptr<MentionGroupConfiguration> _config;

	/** Store of shared resources and cached info about the document. */
	boost::scoped_ptr<LinkInfoCache> _cache;
	
	boost::scoped_ptr<MentionGroupMerger> _mergers;

	/** Apply a MentionGroupMerger operation to a set of MentionGroups. */
	void merge(MentionGroupList &groups) const;

	/** Return an initial set of MentionGroups - one Mention per group. */
	MentionGroupList initializeMentionGroups(DocTheory *docTheory) const;

	/** Return an EntitySet representing the contents of a set of MentionGroups. */
	EntitySet* produceEntitySet(MentionGroupList& groups, DocTheory *docTheory) const;

	/** Return true if mention may be included in an entity - i.e. is a NAME, DESC or PRON. */
	bool isCoreferenceableMention(const Mention *mention) const;

	bool _create_entities_of_undetermined_type;

};

#endif
