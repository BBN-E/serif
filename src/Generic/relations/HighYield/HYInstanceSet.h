// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HY_INSTANCESET_H
#define HY_INSTANCESET_H

/** This class does NOT have ownership of any of the objects it points to.  When an HYInstanceSet
is deleted, the objects it pointed to are NOT freed.  Another class must take care of the 
memory management for these objects if this is necessary, possibly by explicitly calling 
the HYInstanceSet::deleteObjects() function.  */

#include <vector>

#include "Generic/common/StringTransliterator.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/relations/PotentialTrainingRelation.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/relations/discmodel/DTRelSentenceInfo.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/HighYield/HYRelationInstance.h"

using namespace std;

class HYInstanceSet {

public:
	explicit HYInstanceSet(DTTagSet* = 0);
	~HYInstanceSet();

	static std::wstring makeHYString(HYRelationInstance const * const instance);

	void clear(); 
	// NOTE that this function does NOT dispose of the memory allocated to the contained objects themselves;
	// it only empties all the vectors and hashmaps.
	// It does NOT change the TagSet.

	int getNumUsed() const;

	void setTagset(DTTagSet*);

	/*	Add the given instance to this HYInstanceSet.  If allSentences is true, then also look for 
		and add all the other relations in the same sentence. */
	void addData(HYRelationInstance*, bool includeWholeSentence = false);

	int addDataFromSerifFileList(const wchar_t * const filelistName, int beamWidth);
	int addDataFromSerifFileList(const char * const filelistName, int beamWidth);

	int addDataFromAnnotationFile(const wchar_t * const fileName, const HYInstanceSet& sourceSet);
	int addDataFromAnnotationFile(const char * const fileName, const HYInstanceSet& sourceSet);

	void writeToAnnotationFile(const wchar_t * const fileName) const;
	void writeToAnnotationFile(const char * const fileName) const;

	void clearUsed();

	// readUsedFromFile() ADDS TO (does not replace) the previous used set
	// id's must be of instances already in this HYInstanceSet
	void readUsedFromFile(const wchar_t * const fileName);
	void readUsedFromFile(const char * const fileName);
	void writeUsedToFile(const wchar_t * const fileName) const;
	void writeUsedToFile(const char * const fileName) const;

	/*	this will free all of the SentenceInfo objects and all of the HYRelationInstance objects
	that this HYInstanceSet knows about.  It will NOT free the TagSet object.  */
	void deleteObjects();  

	/*	keeping track of who's been used; 
		these throw an exception if the instance is not in this HYInstanceSet */
	bool isUsed(HYRelationInstance*) const;
	void setUsed(HYRelationInstance*);

	// let's act like a set:
	bool contains(HYRelationInstance*) const;
	bool contains(DTRelSentenceInfo*) const;

	// let's act like a vector of HYRelationInstance:
	size_t size() const;
	HYRelationInstance*& operator[](unsigned n);
	const HYRelationInstance* operator[](unsigned n) const;
	vector<HYRelationInstance*>::iterator begin();
	vector<HYRelationInstance*>::iterator end();

	// and have some iterators & accessors for the sentenceInfos:
	vector<DTRelSentenceInfo*>::iterator sentencesBegin();
	vector<DTRelSentenceInfo*>::iterator sentencesEnd();
	vector<HYRelationInstance*>::iterator instancesBegin(DTRelSentenceInfo*);
	vector<HYRelationInstance*>::iterator instancesEnd(DTRelSentenceInfo*);

	// and we can compare HYRelationInstance*
	class lessthanRelInstancePtr {
	public:
		bool operator()(const HYRelationInstance*& one, const HYRelationInstance*& two) const {
			return (one->_margin < two->_margin);
		}
	};

	// adding one by one:
	void addInstance(HYRelationInstance*);

	// and getting by name:
	HYRelationInstance* getInstance(Symbol id) const;

private:
	void saveUsedInstances(const wchar_t* const fileName) const;
	void loadUsedInstances(const wchar_t* const fileName);
	void addAllInstances(DTRelSentenceInfo*, Symbol docID);
	void addSentenceInfo(DTRelSentenceInfo* info);
	HYRelationInstance* loadAnnotatedInstance(UTF8InputStream& fileStream, const HYInstanceSet& sourceSet);
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
	typedef	serif::hash_map<InfoPtr, vector<HYRelationInstance*>, hashDTRelSentenceInfoPtr, eqDTRelSentenceInfoPtr> InfoToInstancesMap;
	typedef	serif::hash_map<Symbol, HYRelationInstance*, Symbol::Hash, Symbol::Eq> UidToInstancesMap;

	DTTagSet* _tagSet;
	int _numUsed;

	// main data structures:
	InfoToInstancesMap _sentenceInfoToRelations; 
	UidToInstancesMap _uidToInstance;
	vector<HYRelationInstance*> _instances;  // sort() needs a random access container to work on
	vector<DTRelSentenceInfo*> _sentences; // used for devTest and other things that want order
	Symbol::HashSet* _usedInstances;  // stores the unique identifiers of the instances that have already been added to training
};

#endif
