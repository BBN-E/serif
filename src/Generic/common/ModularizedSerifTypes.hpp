// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved
#ifndef SERIF_GENERIC_COMMON_MODULARIZED_SERIF_TYPES
#define SERIF_GENERIC_COMMON_MODULARIZED_SERIF_TYPES

#include <vector>
#include "Interfaces/SerifCAS.hpp"

namespace serif {

	class ModularizedSerifTypes {
	public:
		typedef std::vector<std::wstring> paramtype; 
		enum SerifProcessErrorType {
			ok,no_file_found,invalid_path,unexpected_format,
			interruped_connection,cannot_delete_file,
			cannot_read_file,connot_write_file} ; 

	static SerifCAS * NoDocumentFound; //marker indicating that no document was found on this attempt by the doc source
	static SerifCAS * DocumentSourceClosed; //marker indicating that this document source is complete


	};
	
};

#endif

