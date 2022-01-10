#include <sstream>
#include "Generic/common/UnexpectedInputException.h"
#include "SevenZipDecompressor.h"

//Include 7-zip C library
extern "C" {
	#include "../../../External/7Zip/C/Archive/7z/7zIn.h"
	#include "../../../External/7Zip/C/Archive/7z/7zExtract.h"
	#include "../../../External/7Zip/C/7zCrc.h"
}

typedef struct _CFileInStream
{
  ISzInStream InStream;
  FILE * File;
} CFileInStream;

SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
  if ( size > 0 )
  {
	  CFileInStream *s = (CFileInStream *)object;
	  size_t processedSizeLoc = fread(buffer, 1, size, s->File); 
	  if (processedSize != 0)
	  {
		*processedSize = processedSizeLoc;
	  }
  }
  return SZ_OK;
}

SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
  CFileInStream *s = (CFileInStream *)object;
  int res = fseek(s->File, (long)pos, SEEK_SET);
  if (res == 0)
    return SZ_OK;
  return SZE_FAIL;
}


// largely copied from Brandy's ThreadedDocumentLoader
unsigned char* SevenZipDecompressor::decompressIntoMemory(
		const std::string& filename, size_t& size) const 
{
	FILE* SzFile = fopen(filename.c_str(), "rb");
	if (SzFile == NULL) {
		std::stringstream err;
		err << "Could not open file " << filename;
		throw UnexpectedInputException("SevenZipDecompressor::decompressIntoMemory",
				err.str().c_str());
	}

	CFileInStream archiveStream;
	archiveStream.File = SzFile;
	archiveStream.InStream.Read = SzFileReadImp;
	archiveStream.InStream.Seek = SzFileSeekImp;

	ISzAlloc allocImp;
	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;

	ISzAlloc allocTempImp;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	CrcGenerateTable();

	CArchiveDatabaseEx db;
	SzArDbExInit(&db);
	SZ_RESULT res = SzArchiveOpen(&(archiveStream.InStream), &db, 
			&allocImp, &allocTempImp);

	if (res != SZ_OK) {
		std::stringstream err;
		err << "7Zip decompression error code " << res << " for filename "
			<< filename;
		throw UnexpectedInputException("SevenZipDecompressor::decompressIntoMemory",
				err.str().c_str());
	}

	UInt32 blockIndex = 0xFFFFFFFF;
	unsigned char* outBuffer = 0; 
	size_t offset;
	size_t outSizeProcessed;
	int index = 0;
	//size_t bufferSize;
	res = SzExtract(&archiveStream.InStream, &db, index, &blockIndex, 
			&outBuffer, &size, &offset, &outSizeProcessed, 
			&allocImp, &allocTempImp);
	SzArDbExFree(&db, allocImp.Free);
	fclose(archiveStream.File);
	if (res != SZ_OK) {
		std::stringstream err;
		err << "7Zip decompression error code " << res << " for filename "
			<< filename;
		throw UnexpectedInputException("SevenZipDecompressor::decompressIntoMemory",
				err.str().c_str());
	}
	return outBuffer;
}

