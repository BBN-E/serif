// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/factfinder/FactPatternManager.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/patterns/PatternSet.h"

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <boost/scoped_ptr.hpp>

FactPatternManager::FactPatternManager(std::string factPatternList) {
	// Read the list of fact pattern files
	boost::scoped_ptr<UTF8InputStream> factPatternListFile_scoped_ptr(UTF8InputStream::build(factPatternList.c_str()));
	UTF8InputStream& factPatternListFile(*factPatternListFile_scoped_ptr);

	std::wstring patternFile;
	std::wstring input;
	while (true) {
		factPatternListFile.getLine(input);
		if (factPatternListFile.eof()) break;
		boost::algorithm::trim(input);
		size_t pos = input.find(L" ");
		std::wstring extraFlagStr = L"";
		std::wstring entityTypeStr = L"";
		if (pos != input.npos) {
			extraFlagStr = input.substr(pos + 1);
			boost::algorithm::trim(extraFlagStr);
			entityTypeStr = input.substr(0, pos);
		} else
			entityTypeStr = input;

		factPatternListFile.getLine(patternFile);

		std::string patternFileStr = UnicodeUtil::toUTF8StdString(patternFile);
		patternFileStr = ParamReader::expand(patternFileStr);

		//Skip commented lines
		if (patternFile.size() > 0 && patternFile[0] == '#')
			continue;

		std::cerr << "  Reading pattern " << patternFileStr << "..." << std::endl;
		Sexp *sexp = _new Sexp(patternFileStr.c_str(), true, true, true);
		PatternSet_ptr qps = boost::make_shared<PatternSet>(sexp);
		Symbol qpsEntityTypeSymbol = Symbol(entityTypeStr);
		Symbol extraFlagSymbol = Symbol(extraFlagStr);

		_factPatternSets.push_back(qps);
		_entityTypeSymbol.push_back(qpsEntityTypeSymbol);
		_extraFlagSymbol.push_back(extraFlagSymbol);
		
		delete sexp;
	}
	factPatternListFile.close();
}

FactPatternManager::~FactPatternManager() {
}

