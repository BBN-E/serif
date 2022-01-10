#ifndef _COULD_NOT_CHOOSE_FEATURE_EXCEPTION_H_
#define _COULD_NOT_CHOOSE_FEATURE_EXCEPTION_H_

#include <string>
#include "Generic/common/UnrecoverableException.h"

class CouldNotChooseFeatureException : public UnrecoverableException {
public:
	CouldNotChooseFeatureException(const char* source, const std::string& string);
};
#endif
