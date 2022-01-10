// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/theories/EntityType.h"
class SynNode;
class MentionSet;
class EnglishPMDescriptorClassifier;
class P1DescriptorClassifier;

/* added for speed up
#include "dynamic_includes/common/ProfilingDefinition.h"

#ifdef DO_DESCRIPTOR_CLASSIFIER_TRACE
#include "Generic/common/GenericTimer.h"
#endif
*/

#include "Generic/descriptors/DescriptorClassifier.h"



class EnglishDescriptorClassifier : public DescriptorClassifier {
private:
	friend class EnglishDescriptorClassifierFactory;

public:

	~EnglishDescriptorClassifier();
	virtual void cleanup();

	/* added for speed up
	#ifdef DO_DESCRIPTOR_CLASSIFIER_TRACE
	void printTrace();
	#endif
	*/

private:
	EnglishDescriptorClassifier();

	// the actual probability work do-er
	// returns the number of possibilities selected
	virtual int classifyDescriptor(MentionSet *currSolution, const SynNode* node, EntityType types[], 
									double scores[], int maxResults);

	EnglishPMDescriptorClassifier *_pmDescriptorClassifier;
	P1DescriptorClassifier *_p1DescriptorClassifier;
	enum { PM, P1 };
	int _classifier_type;

/* added for speed up
public:
	struct HashKey {
		size_t operator()(const Symbol& s) const {
			return s.hash_code();
		}
	};
    struct EqualKey {
        bool operator()(const Symbol& s1, const Symbol& s2) const {
            return s1 == s2;
        }
    };
	typedef hash_map<Symbol, Symbol, HashKey, EqualKey> Table;

	static Table* _nomMentionList;
	static bool _initialized; 
*/

};

class EnglishDescriptorClassifierFactory: public DescriptorClassifier::Factory {
	virtual DescriptorClassifier *build() { return _new EnglishDescriptorClassifier(); }
};


