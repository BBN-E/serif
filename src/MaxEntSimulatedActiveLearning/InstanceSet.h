// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef INSTANCESET_H
#define INSTANCESET_H

/** This class does NOT have ownership of any of the objects it points to.  When an InstanceSet
is deleted, the objects it pointed to are NOT freed.  Another class must take care of the 
memory management for these objects if this is necessary, possibly by explicitly calling 
the InstanceSet::deleteObjects() function.  */

#include <vector>

#include "Definitions.h"

#include "common/StringTransliterator.h"
#include "discTagger/DTTagSet.h"
#include "relations/PotentialTrainingRelation.h"
#include "relations/RelationUtilities.h"
#include "relations/discmodel/DTRelSentenceInfo.h"
#include "relations/discmodel/RelationObservation.h"

#include "RelationInstance.h"
#include "Utilities.h"

using namespace std;

class InstanceSet {

public:
	explicit InstanceSet(DTTagSet* = 0);
	~InstanceSet();

	static std::wstring makeHYAString(RelationInstance const * const instance);

	void clear(); 
	// NOTE that this function does NOT dispose of the memory allocated to the contained objects themselves;
	// it only empties all the vectors and hashmaps.
	// It does NOT change the TagSet.

	int getNumUsed() const;

	void setTagset(DTTagSet*);

	/*	Add the given instance to this InstanceSet.  If allSentences is true, then also look for 
		and add all the other relations in the same sentence. */
	void addData(RelationInstance*, bool includeWholeSentence = false);

	int addDataFromSerifFileList(const wchar_t * const filelistName, int beamWidth);
	int addDataFromSerifFileList(const char * const filelistName, int beamWidth);

	int addDataFromAnnotationFile(const wchar_t * const fileName, const InstanceSet& sourceSet);
	int addDataFromAnnotationFile(const char * const fileName, const InstanceSet& sourceSet);

	void writeToAnnotationFile(const wchar_t * const fileName) const;
	void writeToAnnotationFile(const char * const fileName) const;

	void clearUsed();

	// readUsedFromFile() ADDS TO (does not replace) the previous used set
	// id's must be of instances already in this InstanceSet
	void readUsedFromFile(const wchar_t * const fileName);
	void readUsedFromFile(const char * const fileName);
	void writeUsedToFile(const wchar_t * const fileName) const;
	void writeUsedToFile(const char * const fileName) const;

	/*	this will free all of the SentenceInfo objects and all of the RelationInstance objects
	that this InstanceSet knows about.  It will NOT free the TagSet object.  */
	void deleteObjects();  

	/*	keeping track of who's been used; 
		these throw an exception if the instance is not in this InstanceSet */
	bool isUsed(RelationInstance*) const;
	void setUsed(RelationInstance*);

	// let's act like a set:
	bool contains(RelationInstance*) const;
	bool contains(DTRelSentenceInfo*) const;

	// let's act like a vector of RelationInstance:
	size_t size() const;
	RelationInstance*& operator[](unsigned n);
	const RelationInstance* operator[](unsigned n) const;
	vector<RelationInstance*>::iterator begin();
	vector<RelationInstance*>::iterator end();

	// and have some iterators & accessors for the sentenceInfos:
	vector<DTRelSentenceInfo*>::iterator sentencesBegin();
	vector<DTRelSentenceInfo*>::iterator sentencesEnd();
	vector<RelationInstance*>::iterator instancesBegin(DTRelSentenceInfo*);
	vector<RelationInstance*>::iterator instancesEnd(DTRelSentenceInfo*);

	// and we can compare RelationInstance*
	class lessthanRelInstancePtr {
	public:
		bool operator()(const RelationInstance*& one, const RelationInstance*& two) const {
			return (one->_margin < two->_margin);
		}
	};

	// adding one by one:
	void addInstance(RelationInstance*);

	// and getting by name:
	RelationInstance* getInstance(Symbol id) const;

private:
	void saveUsedInstances(const wchar_t* const fileName) const;
	void loadUsedInstances(const wchar_t* const fileName);
	void addAllInstances(DTRelSentenceInfo*, Symbol docID);
	void addSentenceInfo(DTRelSentenceInfo* info);
	RelationInstance* InstanceSet::loadAnnotatedInstance(UTF8InputStream& fileStream, const InstanceSet& sourceSet);
	void ensureTagSet() const;	

	typedef DTRelSentenceInfo* InfoPtr;
	class eqDTRelSentenceInfoPtr {
	public:
		bool operator()(const InfoPtr& one, const InfoPtr& two) const {
			return (one == two);
		}
	};

	class hashDTRelSentenceInfoPtr {
	public:
		size_t operator()(const InfoPtr& x) const {
			return (size_t)(x);
		}
	};
	typedef	hash_map<InfoPtr, vector<RelationInstance*>, hashDTRelSentenceInfoPtr, eqDTRelSentenceInfoPtr> InfoToInstancesMap;
	typedef	hash_map<Symbol, RelationInstance*, Symbol::Hash, Symbol::Eq> UidToInstancesMap;

	DTTagSet* _tagSet;
	int _numUsed;

	// main data structures:
	InfoToInstancesMap _sentenceInfoToRelations; 
	UidToInstancesMap _uidToInstance;
	vector<RelationInstance*> _instances;  // sort() needs a random access container to work on
	vector<DTRelSentenceInfo*> _sentences; // used for devTest and other things that want order
	Symbol::HashSet* _usedInstances;  // stores the unique identifiers of the instances that have already been added to training
};

#endif
