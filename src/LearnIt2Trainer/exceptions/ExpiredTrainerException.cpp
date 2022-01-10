#include "ExpiredTrainerException.h"

ExpiredTrainerException::ExpiredTrainerException(const char* source) :
 UnrecoverableException(source, "Attempt to access a pointer to an expired trainer.")
 {}
