#include "CouldNotChooseFeatureException.h"

CouldNotChooseFeatureException::CouldNotChooseFeatureException
	(const char* source, const std::string& string) :
 UnrecoverableException(source, std::string("Could not choose feature: ") + string) 
 {}
