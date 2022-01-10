// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "BasicCipherStream/UTF8BasicCipherInputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ParamReader.h"

#include <boost/foreach.hpp>

// Note: "\xfe\xfe" is an invalid UTF-8 sequence.  This ensures that any
// valid input file can not contain our expected magic prefix -- thus, 
// we will never get any "false positives" where we think a file is encrypted
// when really it is not (unless the input file does not contain valid 
// UTF-8 to begin with, in which case we can't process it anyway).
const std::string UTF8BasicCipherInputStream::BASIC_CIPHER_STREAM_MAGIC_PREFIX
    ("SerifEncrypted_BBN_Proprietary_IP\n\n\xfe\xfe");

/** Private constants & helper functions */
namespace {
	const int SRC_BUF_SIZE = 1024*8;
	const int DST_BUF_SIZE = SRC_BUF_SIZE+1024;

	template<typename CharType>
	std::basic_string<CharType> readFixedSizeString(std::basic_istream<CharType> &stream, int size) {
		std::basic_string<CharType> result;
		result.reserve(size);
		for (int n=0; stream.good() && (n<size); ++n) {
			int c = stream.get();
			if (stream.good())
				result.push_back((CharType)c);
		}
		return result;
	}

	/** Read the given file, decrypt it with the key, and write the result
	* to the string variable "result".  This expects the magic prefix to 
	* be already removed. */
	void decrypt(std::ifstream &ifs, std::string &result, const std::string &password) {
		if (ifs.bad())
			throw UnexpectedInputException("UTF8BasicCipherInputStream::open()",
										   "Error reading source file");
		// Get the random "salt" -- this tells us what offset to start at for the 
		// password.
		size_t password_offset = 0;
		password_offset += 256*((unsigned char)(ifs.get())%256);
		password_offset += (unsigned char)(ifs.get())%256;
		for (size_t i=0; ifs.good(); ++i) {
			int c = ifs.get();
			if (ifs.good())
				result.push_back((char)(c ^ password[password_offset++%password.size()]));
		}
	}
}

UTF8BasicCipherInputStream::UTF8BasicCipherInputStream(const char *key, const char *file) 
:_is_open(false), _key(key)
{
	if (file) 
		open(file);	
}

UTF8BasicCipherInputStream::~UTF8BasicCipherInputStream () {
	close();
}


void UTF8BasicCipherInputStream::open (const char *file) {
	registerFileOpen(file);
	// Read the first few bytes of the file to decide whether it's encrypted or not.
	std::ifstream byteStream(file, std::ios::binary);
	std::string prefix = readFixedSizeString(byteStream, static_cast<int>(BASIC_CIPHER_STREAM_MAGIC_PREFIX.size()));
	//std::cerr << "PREFIX [" << prefix << "]" << std::endl;
	if (prefix != BASIC_CIPHER_STREAM_MAGIC_PREFIX) {
		byteStream.close();
		//std::cerr << "Non-encrypted file detected ["<<file<<"]" << std::endl;
		std::locale old_locale;
		std::locale utf8_locale( old_locale, new boost::utf8::utf8_codecvt_facet );
		this->imbue( utf8_locale );
		UTF8InputStream::open(file);
	} else {
		//std::cerr << "Encrypted file detected ["<<file<<"]" << std::endl;
		std::string decrypted;
		// Decrypt the entire file.
		decrypt(byteStream, decrypted, _key);
		byteStream.close();
		// Initailize our wstringstream with the decrypted string.
		_decrypted_stream.str(UnicodeUtil::toUTF16StdString(decrypted, UnicodeUtil::REPLACE_ON_ERROR));
		this->init(_decrypted_stream.rdbuf());
	}
	_is_open = true;
}

void UTF8BasicCipherInputStream::open (const wchar_t *file) {
  open(UnicodeUtil::toUTF8StdString(file).c_str());
}

void UTF8BasicCipherInputStream::close() {
	_is_open = false;
	_decrypted_stream.str(L"");
}


