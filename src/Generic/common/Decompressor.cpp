#include "Generic/common/leak_detection.h"
#include "Decompressor.h"
#include <sstream>
#include <utility>
#include "Generic/common/BoostUtil.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnexpectedInputException.h"

Decompressor::ImplMap Decompressor::_implementations;

std::string extension(const std::string& filename) {
	return BOOST_FILESYSTEM_PATH_GET_EXTENSION(boost::filesystem::path(filename));
}

bool Decompressor::canDecompress(const std::string& filename) {
	std::string ext = extension(filename);

	if (!ext.empty()) {
		return _implementations.find(ext) != _implementations.end();
	}

	return false;
}

unsigned char* Decompressor::decompressIntoMemory(const std::string& filename,
		size_t& size, const std::string& overrideExtension)
{
	std::string ext;
	if (overrideExtension.empty()) {
		ext = extension(filename);
	} else {
		ext = overrideExtension;
	}
		
	if (!ext.empty()) {
		ImplMap::iterator probe = _implementations.find(ext);

		if (probe != _implementations.end()) {
			return probe->second->decompressIntoMemory(filename, size);
		} else {
			std::stringstream err;
			err << "Cannot decompress file " << filename << " because SERIF "
				<< "doesn't know how to compress files with extension "
				<< ext;
			throw UnexpectedInputException("Decompressor::decompressIntoMemory",
				err.str().c_str());
		}
	} else {
		std::stringstream err;
		err << "Cannot decompress file " << filename << " because it has no "
			<< "extension and no extension override was specified.";
		throw UnexpectedInputException("Decompressor::decompressIntoMemory",
				err.str().c_str());
	}

}

void Decompressor::registerImplementation(const std::string& ext,
		DecompressorImplementation* impl)
{
	if (_implementations.find(ext) != _implementations.end()) {
		SessionLogger::warn("duplicate_decompressor") <<
		 "Attempting to register decompressor for extension "
			<< ext << ", but a decompressor is already registered for it."
			<< " Ignoring.";
	} else {
		_implementations.insert(std::make_pair(ext, impl));
	}
}
