#ifndef _DECOMPRESSOR_H_
#define _DECOMPRESSOR_H_

// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.
#include <string>
#include <map>
#include "Generic/common/bsp_declare.h"

BSP_DECLARE(DecompressorImplementation)
class DecompressorImplementation {
public:
	virtual unsigned char* decompressIntoMemory(const std::string& filename,
			size_t& size) const = 0;
};

class Decompressor {
public:
	static bool canDecompress(const std::string& filename);
	// it is the caller's responsibility to delete the returned buffer
	static unsigned char* decompressIntoMemory(const std::string& filename,
			size_t& size, const std::string& overrideExtension = "");

	// for use by feature modules actually implementing compression
	// extensions should contain the dot (e.g. '.gz', not 'gz')
	static void registerImplementation(const std::string& extension,
			DecompressorImplementation* impl);
private:
	typedef std::map<std::string, DecompressorImplementation*> ImplMap;
	static ImplMap _implementations;
};

#endif

