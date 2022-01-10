#ifndef KR_VERSION_H
#define KR_VERSION_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#define SERIF_KOREAN_VERSION_MAJOR 0
#define SERIF_KOREAN_VERSION_MINOR 0

#define SERIF_KOREAN_VERSION_STRING "0.00"

#include "Generic/common/version.h"


class KoreanSerifVersion : public SerifVersion {

private:
	LanguageAttribute _getSerifLanguage() { return Language::KOREAN; }; 
	char* _getLanguageLibraryName() { return "Korean"; }
	wchar_t* _getLanguageLibraryWideName() { return L"Korean"; }
	char* _getLanguageLibraryVersionString() { return SERIF_KOREAN_VERSION_STRING; }

};

#endif
