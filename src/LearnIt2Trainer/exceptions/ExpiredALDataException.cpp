#include "ExpiredALDataException.h"

ExpiredALDataException::ExpiredALDataException(const char* source) :
 UnrecoverableException(source, "Attempt to access a pointer to an expired trainer.")
 {}
