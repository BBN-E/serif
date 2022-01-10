// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// a collection of possibly useful utilities

#ifndef UTILITIES_H
#define UTILITIES_H

#include "common/ParamReader.h"
#include "common/Symbol.h"
#include "common/UTF8InputStream.h"
#include "common/UTF8Token.h"
#include "theories/Mention.h"

#include "Definitions.h"

int countLinesInFile(const wchar_t *const filename);
int countStateLinesInFile(const wchar_t *const filename);

bool validMention(Mention* m);

// all of these throw an exception if the value cannot be read
double getDoubleParam(const char* const paramName, const char* const source = "");
int getIntParam(const char* const paramName, const char* const source = "");
void getStringParam(const char* const paramName, char* buffer, int bufferSize, const char* const source);
void getWideStringParam(const char* const paramName, wchar_t* buffer, int bufferSize, const char* const source);

#endif
