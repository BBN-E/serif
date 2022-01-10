// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NATIONALITY_RECOGNIZER_H
#define NATIONALITY_RECOGNIZER_H

#include <boost/shared_ptr.hpp>

#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"

class SynNode;


/** This just tells you if a parse node is a nationality adjective and
  * whether it refers to the country (or continent, etc) or to a person 
  * or group of people.
  */

class NationalityRecognizer {
public:
	/** Create and return a new NationalityRecognizer. */
	static bool isNamePersonDescriptor(const SynNode *node) { return _factory()->isNamePersonDescriptor(node); }
	/** Hook for registering new NationalityRecognizer factories */
	struct Factory { virtual bool isNamePersonDescriptor(const SynNode *node) = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	static bool isNationalityWord(Symbol word);
	static bool isCertainNationalityWord(Symbol word);
	static bool isLowercaseNationalityWord(Symbol word);
	static bool isLowercaseCertainNationalityWord(Symbol word);

	virtual ~NationalityRecognizer();

	/// heuristic for nationality person descs ("the Americans")
	//static bool isNamePersonDescriptor(const SynNode *node);

private:
	static void initialize();

protected:
	static bool _initialized;

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
	/// List of nationality names to help identify person descriptors
	static hash_set<Symbol, HashKey, EqualKey> _natNames;
	/// List of names that must be nationalities, regardless of other factors
	static hash_set<Symbol, HashKey, EqualKey> _certainNatNames;
private:
	static boost::shared_ptr<Factory> &_factory();
};
//#if defined(ENGLISH_LANGUAGE)
//	#include "English/common/en_NationalityRecognizer.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/common/ar_NationalityRecognizer.h"
//#else
//	#include "Generic/common/xx_NationalityRecognizer.h"
//#endif

#endif
