// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.
//
// Encrypt a file such that it can be read with the BasicCipherStream
// module.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include <iostream>
#include "dynamic_includes/BasicCipherStreamEncryptionKey.h"

#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "BasicCipherStream/UTF8BasicCipherInputStream.h"


namespace {
	int usage(const char* progname, int exitcode) {
		std::cerr << "Usage: \n" 
				  << "  " << progname << " SRC DST\n"
				  << "Decrypt and display the given file.\n";
		return exitcode;
	}

	const bool VERBOSE=false;
}

int main(int argc, char **argv) {
	FeatureModule::load("BasicCipherStream");
	if (argc != 3)
		return usage(argv[0], -1);

	// By default, the ConsoleSessionLogger prints a newline when it
	// is destroyed (at the end of main()); disable that.
	ConsoleSessionLogger::disableDestructorNewline();

	// Read the input file.
	std::ifstream ifs(argv[1], std::ios::binary);
	if (ifs.bad() || ifs.fail()) {
		std::cerr << "Error reading input file \"" << argv[1] << "\"" << std::endl;
		return -1;
	}
	std::istreambuf_iterator<char> eos;
	std::string s(std::istreambuf_iterator<char>(ifs), eos);
	ifs.close();
	if (s.empty())
		std::cerr << "Warning: \"" << argv[1] << "\" appears to be empty!" << std::endl;
	
	// If there's a magic prefix, then assume we're decrypting;
	// otherwise, assume we're encrypting.
	const std::string& magicPrefix = UTF8BasicCipherInputStream::BASIC_CIPHER_STREAM_MAGIC_PREFIX;
	bool decrypting = (s.substr(0, magicPrefix.size())==magicPrefix);
	size_t start = decrypting ? magicPrefix.size()+2 : 0;

	// Determine the password offset
	size_t initial_password_offset = 0;
	if (decrypting) {
		if (s.size() < start) {
			std::cerr << "Error reading input file " << argv[1] 
					  << ": no offset (salt) found" << std::endl;
			return -1;
		}
		initial_password_offset += 256*((unsigned char)(s[start-2])%256);
		initial_password_offset += (unsigned char)(s[start-1])%256;
		//std::cerr << "Decrypting: offset=" << initial_password_offset << std::endl;
	} else {
		srand((unsigned) time(0));
		initial_password_offset = rand() % (256*256);
		//std::cerr << "Encrypting: offset=" << initial_password_offset << std::endl;
	}

	// Encrypt/decrypt it.
	std::string encrypted;
	std::string password(BASIC_CIPHER_STREAM_PASSWORD);
	size_t password_offset(initial_password_offset);
	for (size_t i=start; i<s.size(); ++i) {
		encrypted.push_back(s[i] ^ password[password_offset++%password.size()]);
	}

	// Write the magic prefix, offset, and the encrypted contents.
	std::ofstream ofs(argv[2], std::ios::binary);
	if (ifs.bad() || ifs.fail()) {
		std::cerr << "Error writing output file \"" << argv[1] << "\"" << std::endl;
		return -1;
	}
	if (!decrypting) {
		ofs.write(magicPrefix.c_str(), magicPrefix.size());
		ofs.put((unsigned char)((initial_password_offset/256)%256));
		ofs.put((unsigned char)(initial_password_offset%256));
	}
	ofs.write(encrypted.c_str(), encrypted.size());
	ofs.close();
	if (VERBOSE) {
		std::cerr << (decrypting?"Decrypted":"Encrypted")
				  << " \"" << argv[1] << "\" (" << encrypted.size() << " bytes)" << std::endl;
	}
}
