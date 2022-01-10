// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "OpenSSLCipherStream/UTF8OpenSSLCipherInputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ParamReader.h"

#include <openssl/evp.h>

/** Private constants & helper functions */
namespace {
	const char OPEN_SSL_MAGIC_PREFIX[]="Salted__";
	const int SRC_BUF_SIZE = 1024*8;
	const int DST_BUF_SIZE = SRC_BUF_SIZE+1024;

	template<typename CharType>
	std::basic_string<CharType> readFixedSizeString(std::basic_istream<CharType> &stream, int size) {
		std::basic_string<CharType> result;
		result.reserve(size);
		for (int n=0; (!stream.bad()) && (n<size); ++n)
			result.push_back((CharType)stream.get());
		return result;
	}

	void initializeOpenSSL() {
		static bool initialized = false;
		if (!initialized) {
			OpenSSL_add_all_algorithms(); // required by EVP_get_cipherbyname()
			initialized = true;
		}
	}

	const EVP_CIPHER* choose_cipher(const char* filename) {
		initializeOpenSSL();

		// Look up which cipher type to use in the parameter file
		std::string cipher_type = "auto";
		if (ParamReader::isInitialized())
			cipher_type = ParamReader::getParam("openssl_cipher_type", "auto");

		// If cipher_type is auto, then check the filename extension, 
		// and return the corresponding cipher (if it exists).  Otherwise,
		// use the default cipher type.
		if (cipher_type == "auto") {
			std::string path(filename);
			int dotpos = path.rfind('.');
			if (dotpos != -1) {
				const EVP_CIPHER* cipher = EVP_get_cipherbyname(path.substr(dotpos+1).c_str());
				if (cipher != NULL) return cipher;
			}
			cipher_type = ParamReader::getParam("openssl_default_cipher_type", "rc4");
		}

		const EVP_CIPHER* cipher = EVP_get_cipherbyname(cipher_type.c_str());
		if (cipher == NULL)
			throw UnexpectedInputException("UTF8OpenSSLCipherInputStream::decrypt",
				"Unknown cipher type: ", cipher_type.c_str());
		return cipher;
	}

	/** Read the given file, decrypt it with the key, and write the result
	* to the string variable "result". */
	void decrypt(const char* filename, std::string &result, const std::string &password) {
		// Open the file.
		std::basic_ifstream<unsigned char> input_stream(filename, std::ios_base::binary);

		// Read the magic prefix and the salt.
		readFixedSizeString(input_stream, strlen(OPEN_SSL_MAGIC_PREFIX));
		std::basic_string<unsigned char> salt = readFixedSizeString(input_stream, PKCS5_SALT_LEN);

		// Choose a cipher to use.
		const EVP_CIPHER* cipher = choose_cipher(filename);

		// Choose a digest to use
		const EVP_MD *dgst = EVP_md5();

		// Convert password to key+iv.
		unsigned char key[EVP_MAX_KEY_LENGTH];
		unsigned char iv[EVP_MAX_IV_LENGTH];
		
		if (!EVP_BytesToKey(cipher, dgst, salt.c_str(),
			reinterpret_cast<const unsigned char *>(password.c_str()),
			password.size(), 1, key, iv))
			throw UnexpectedInputException("UTF8OpenSSLCipherInputStream::decrypt",
				"Error while converting password->key");
	
		// Set up the encryption context.
		EVP_CIPHER_CTX ctx;
		EVP_CIPHER_CTX_init(&ctx);
		EVP_DecryptInit_ex(&ctx, cipher, NULL, key, iv);

		// Buffers to read from/to.
		unsigned char src[SRC_BUF_SIZE];
		unsigned char dst[DST_BUF_SIZE];

		if (static_cast<int>((sizeof(dst)-sizeof(src))/sizeof(unsigned char)) < 
			EVP_CIPHER_block_size(cipher))
			throw InternalInconsistencyException("UTF8OpenSSLCipherInputStream::decrypt",
				"DST_BUF_SIZE is not large enough!");

		while (!input_stream.eof()) {
			// Read in a block of data.
			input_stream.read(src, sizeof(src)/sizeof(src[0]));
			size_t nb = input_stream.gcount(); // number of bytes read
			if (input_stream.bad())
				throw UnexpectedInputException("UTF8OpenSSLCipherInputStream::decrypt",
					"Error while reading from: ", filename);
			if (nb == 0) break;
			// Decrypt the data.
			int olen, tlen;
			if (EVP_DecryptUpdate(&ctx, dst, &olen, src, nb) != 1)
				throw UnexpectedInputException("UTF8OpenSSLCipherInputStream::decrypt",
					"Error while decrypting (update): ", filename);
			if (EVP_DecryptFinal_ex(&ctx, dst + olen, & tlen) != 1)
				throw UnexpectedInputException("UTF8OpenSSLCipherInputStream::decrypt",
					"Error while decrypting (final): ", filename);
			// Append the decrypted data to dst.
			result.append(reinterpret_cast<const char*>(dst), olen+tlen);
		}

		EVP_CIPHER_CTX_cleanup (&ctx);
	}
}

UTF8OpenSSLCipherInputStream::UTF8OpenSSLCipherInputStream(const char *key, const char *file) 
:_is_open(false), _key(key)
{
	if (file) 
		open(file);	
}

UTF8OpenSSLCipherInputStream::~UTF8OpenSSLCipherInputStream () {
	close();
}


void UTF8OpenSSLCipherInputStream::open (const char *file) {
	registerFileOpen(file);
	// Read the first few bytes of the file to decide whether it's encrypted or not.
	std::ifstream magicStream(file, std::ios::binary);
	std::string prefix = readFixedSizeString(magicStream, strlen(OPEN_SSL_MAGIC_PREFIX));
	magicStream.close();
	//std::cerr << "PREFIX [" << prefix << "]" << std::endl;
	if (prefix.compare(OPEN_SSL_MAGIC_PREFIX)!=0) {
		//std::cerr << "Non-encrypted file detected ["<<file<<"]" << std::endl;
		std::locale old_locale;
		std::locale utf8_locale( old_locale, new boost::utf8::utf8_codecvt_facet );
		this->imbue( utf8_locale );
		UTF8InputStream::open(file);
	} else {
		//std::cerr << "Encrypted file detected ["<<file<<"]" << std::endl;
		std::string decrypted;
		// Decrypt the entire file.
		decrypt(file, decrypted, _key);
		// Initailize our wstringstream with the decrypted string.
		_decrypted_stream.str(UnicodeUtil::toUTF16StdString(decrypted, UnicodeUtil::REPLACE_ON_ERROR));
		this->init(_decrypted_stream.rdbuf());
	}
	_is_open = true;
}

void UTF8OpenSSLCipherInputStream::open (const wchar_t *file) {
  open(UnicodeUtil::toUTF8StdString(file).c_str());
}

void UTF8OpenSSLCipherInputStream::close() {
	_is_open = false;
	_decrypted_stream.str(L"");
}


