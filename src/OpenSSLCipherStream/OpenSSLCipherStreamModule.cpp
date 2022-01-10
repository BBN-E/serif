// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include <iostream>
#include "OpenSSLCipherStream/OpenSSLCipherStreamModule.h"
#include "OpenSSLCipherStream/UTF8OpenSSLCipherInputStream.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/ParamReader.h"
#include "dynamic_includes/OpenSSLEncryptionKey.h"

// Factories
class UTF8OpenSSLCipherInputStreamFactory: public UTF8InputStream::Factory {
	virtual UTF8InputStream *build(const char* filename, bool encrypted) {
		if (encrypted)
			return new UTF8OpenSSLCipherInputStream(OPENSSL_PASSWORD, filename);
		else {
			static bool alwaysDecrypt = ParamReader::isParamTrue("cipher_stream_always_decrypt");
			if (alwaysDecrypt)
				return new UTF8OpenSSLCipherInputStream(OPENSSL_PASSWORD, filename);
			else
				return UTF8InputStream::Factory::build(filename);
		}
	}
};

// Plugin setup function.
extern "C" DLL_PUBLIC void* setup_OpenSSLCipherStream() {
	UTF8InputStream::setFactory(boost::shared_ptr<UTF8InputStream::Factory>
		(new UTF8OpenSSLCipherInputStreamFactory()));
	return FeatureModule::setup_return_value();
}

