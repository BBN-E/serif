// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_RESULTS_COLLECTOR_H
#define IDF_RESULTS_COLLECTOR_H

#include <string>
#include "Generic/common/LocatedString.h"
#include "Generic/common/UTF8OutputStream.h"

namespace DataPreprocessor {

	/**
	 * A wrapper class to abstract away the collection of
	 * results from the IdfTrainerPreprocessor. Test cases
	 * can use this to store results in a string a retrieve
	 * them, and the main utility can use this to store
	 * results in an output file.
	 *
	 * @author Dave Herman
	 */
	class ResultsCollector {
	public:
		/// Constructs a new ResultsCollector and opens it on the given file.
		ResultsCollector(const char *output_file);

		/// Constructs a new ResultsCollector without an output file.
		ResultsCollector();

		/// Destroys this ResultsCollector, closing the output file if there is one.
		~ResultsCollector();

		/// Opens the given output file, if there is not already one open.
		void open(const char *output_file);

		/// Returns the collected results, if there is no output file.
		const wchar_t *getResults();

		/// Collects the given results.
		ResultsCollector& operator<< (const std::wstring& str);

		/// Collects the given results.
		ResultsCollector& operator<< (const wchar_t* str);

		/// Collects the given results.
		ResultsCollector& operator<< (const LocatedString& str);
	private:
		/// The output stream to collect results to a file.
		UTF8OutputStream *_stream;

		/// The string to collect results in if no file is open.
		std::wstring *_string;
	};

} // namespace DataPreprocessor

#endif
