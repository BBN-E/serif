// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.
//
// Encrypt or decrypt a file.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include <iostream>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/FeatureModule.h"
#include <openssl/evp.h>

int usage(const char* progname, int exitcode) {
	std::cerr << "Usage: \n" 
			  << "  " << progname << " SRC\n"
			  << "Decrypt and display the given file.\n";
	return exitcode;
}

int main(int argc, char **argv) {
	FeatureModule::load("OpenSSLCipherStream");
	if (argc != 2)
		return usage(argv[0], -1);
	const char* src_name = argv[1];

	try {
		typedef boost::scoped_ptr<UTF8InputStream> UTF8InputStream_ptr;
		UTF8InputStream_ptr src(UTF8InputStream::build(src_name, true));
		
		wchar_t ch;
		while (src && src->get(ch))
			std::wcout << ch;
		src->close();
	} catch (UnexpectedInputException &e) {
		std::cerr << "Error: " << e << std::endl;;
	}
	//*/
}
