// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include <cstring>
#include "common/UTF8Token.h"
#include "common/Symbol.h"
#include "parse/ParserTrainer/WordCollector.h"
#include "common/UTF8InputStream.h"
#include "common/UnexpectedInputException.h"
#include "Generic/common/FeatureModule.h"
#include <boost/scoped_ptr.hpp>
// DIVERSITY CHANGES: NONE

int main(int argc, char* argv[]) {
	UTF8Token token;
	WordCollector* WC;

	//std::cerr<< "new version will write extra table for word feats\n";

    if (argc != 4) {
		std::cerr << "wrong number of arguments to word collector\n";
		std::cerr << "Usage: headified_input_file output_vocab_file language\n\n";
		std::cerr << "(language parameter is new: it should be English, Arabic, etc)\n";
        return -1;
    }

	try {

		// Load the specified language feature module.
		FeatureModule::load(argv[3]);

		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(argv[1]);

		WC = new WordCollector();

		while (true) {
	
			in >> token;
			if (in.eof())
				break;
			if (wcscmp(token.chars(), L"(") != 0)
				throw UnexpectedInputException("VocabCollectorMain::main()","ERROR: ill-formed sentence\n");
			WC->read_from_file(in);

		}

		in.close();

		//std::cerr<< "version with word feats ready to print\n";

		WC->print_all(argv[2]);

	} catch (UnexpectedInputException uc) {
		uc.putMessage(std::cerr);
		return -1;

	 
	} catch (std::exception e) {
		std::cerr << "Exception: " << e.what() << std::endl;\
		return -1;
	}

	return 0;
}
