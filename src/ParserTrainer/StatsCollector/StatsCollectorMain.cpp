// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include <cstring>
#include "common/UTF8InputStream.h"
#include "common/UTF8InputStream.h"

#include "common/UTF8Token.h"
#include "parse/ParseNode.h"
#include "parse/ParserTrainer/StatsCollector.h"
#include "parse/ParserTrainer/BridgeCollector.h"
#include "common/UnexpectedInputException.h"
#include "Generic/common/FeatureModule.h"
#include <boost/scoped_ptr.hpp>


int main(int argc, char* argv[]) {
	UTF8Token token;
	ParseNode* parse;

    if (argc != 13 && argc != 12) {
       cerr << "wrong number of arguments to stats collector\n";
	   cerr << "Usage:\n";
	   cerr << "      1 input file (headified)\n";
	   cerr << "      9 output files -- \n";
	   cerr << "       kernels, extensions, prior, head,\n";
	   cerr << "       pre, post, left, right, pos\n";
	   cerr << "      1 language name (eg English, Arabic)\n";
	   cerr << "      1 sequential_bigrams_file (optional)\n";
      return -1;
	}

    StatsCollector *s_collector;

	try {
	FeatureModule::load(argv[11]);

	// argv[11] = optional sequential_bigrams_file
	if (argc == 13)
		s_collector = new StatsCollector(argv[12]);
	else s_collector = new StatsCollector();
	//std::wcerr << "Instantiated StatsCollector.\n";
	BridgeCollector b_collector;
	//std::wcerr << "Instantiated BridgeCollector.\n";
	
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
    in.open(argv[1]);
	//std::wcerr << "Opened input file.\n";
	while (true) {

		in >> token;
        if (in.eof())
			break;
	    if (wcscmp(token.chars(), L"(") != 0)
			throw UnexpectedInputException("StatsCollectorMain::main()",
			"ill-formed sentence\n");
		parse = new ParseNode();
		parse->read_from_file(in, true);

		b_collector.collect(parse);
		s_collector->collect(parse);
		delete parse;

	}
	in.close();
	//std::wcerr << "Closed input file.\n";

    b_collector.print_all(argv[2], argv[3]);
	//std::wcerr << "BridgeCollector printed output.\n";
	s_collector->print_all(argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10]);
	//std::wcerr << "StatsCollector printed output.\n";

	} catch (UnexpectedInputException uc) {
		uc.putMessage(std::cerr);
	
		return -1;

	} 
	return 0;
}
