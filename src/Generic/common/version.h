// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SERIF_VERSION_H
#define SERIF_VERSION_H

#include "Generic/common/Attribute.h"

/** Information about Serif's version number. */
class SerifVersion {
public:
	/** Return a product name for Serif.  By default this is just
	 * "Serif", but specialized versions of Serif may return a
	 * different product name. */
	static const char* getProductName();

	/** Return a version string containing the current version number for
	  * Serif.  This will have the form "MAJOR.MINOR.MICRO", where MAJOR,
	  * MINOR, and MICRO are unsigned integers. */
	static const char* getVersionString();

	/** Return a copyright message for Serif. */
	static const char* getCopyrightMessage();

	/** If a language feature module is in use, then return the langauge
	  * attribute for that language; otherwise, return Language::UNSPECIFIED. */
	static LanguageAttribute getSerifLanguage();

	/** Register the fact that we are using a language feature module.  Note:
	  * Use of multiple language feature modules at the same time is not
	  * supported.*/
	static void setSerifLanguage(LanguageAttribute serifLanguage);

	/** Modify the name returned by getProductName(). */
	static void setProductName(const char *newName);

	// Convenience methods for checking common languages -- these are used in
	// a few locations in core SERIF for backwards compatibility, but their
	// use is discouraged in any new code; instead, you should modify the
	// appropriate language module to use hooks as necessary to change behavior.
	static bool isEnglish() { return _serifLanguage == Language::ENGLISH; }
	static bool isArabic() { return _serifLanguage == Language::ARABIC; }
	static bool isChinese() { return _serifLanguage == Language::CHINESE; }
	static bool isKorean() { return _serifLanguage == Language::KOREAN; }
private:
	static LanguageAttribute _serifLanguage;
};

#endif
