#ifndef _7ZIP_DECOMPRESSOR_H_
#define _7ZIP_DECOMPRESSOR_H_

#include "Generic/common/Decompressor.h"

class SevenZipDecompressor : public DecompressorImplementation {
public:
	unsigned char* decompressIntoMemory(const std::string& filename,
			size_t& size) const;
	 
};

#endif

