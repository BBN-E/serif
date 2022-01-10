// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WORD_CLUSTER_CLASS_H
#define WORD_CLUSTER_CLASS_H

#include <wchar.h>
#include "Generic/common/hash_map.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/wordClustering/WordClusterTable.h"

class WordClusterClass {

public:
	WordClusterClass(Symbol word, bool lowercase = false, bool domain = false, bool secondary = false) {
		/*_c4 = prefix(word, 4, lowercase);
		_c6 = prefix(word, 6, lowercase);
		_c8 = prefix(word, 8, lowercase);
		_c12 = prefix(word, 12, lowercase);
		_c16 = prefix(word, 16, lowercase);
		_c20 = prefix(word, 20, lowercase);*/

		calculatePrefixes(word, lowercase, domain, secondary);
	}

	WordClusterClass(const WordClusterClass &other) {
		_c4 = other._c4;
		_c6 = other._c6;
		_c8 = other._c8;
		_c12 = other._c12;
		_c16 = other._c16;
		_c20 = other._c20;
	}

	static WordClusterClass nullCluster() {
		WordClusterClass c;
		c._c4 = 0;
		c._c6 = 0;
		c._c8 = 0;
		c._c12 = 0;
		c._c16 = 0;
		c._c20 = 0;
		return c;
	}

	int c4() const { return _c4; }
	int c6() const { return _c6; }
	int c8() const { return _c8; }
	int c12() const { return _c12; }
	int c16() const { return _c16; }
	int c20() const { return _c20; }

	bool operator==(const WordClusterClass &other) const {
		return (_c4 == other._c4 &&
				_c6 == other._c6 &&
				_c8 == other._c8 &&
				_c12 == other._c12 &&
				_c16 == other._c16 &&
				_c20 == other._c20);
	}

	WordClusterClass &operator=(const WordClusterClass &other) {
		_c4 = other._c4;
		_c6 = other._c6;
		_c8 = other._c8;
		_c12 = other._c12;
		_c16 = other._c16;
		_c20 = other._c20;
		return *this;
	}

private:
	int _c4;
	int _c6;
	int _c8;
	int _c12;
	int _c16;
	int _c20;

	const static int min4 = (1 << (4 - 1)); 
	const static int min6 = (1 << (6 - 1));
	const static int min8 = (1 << (8 - 1));
	const static int min12 = (1 << (12 - 1));
	const static int min16 = (1 << (16 - 1));
	const static int min20 = (1 << (20 - 1));

	const static int max4 = (1 << 4) - 1;
	const static int max6 = (1 << 6) - 1;
	const static int max8 = (1 << 8) - 1;
	const static int max12 = (1 << 12) - 1;
	const static int max16 = (1 << 16) - 1;
	const static int max20 = (1 << 20) - 1;

	WordClusterClass() {}

	static int prefix(Symbol word, int num_bits, bool lowercase, bool domain, bool secondary ) {
		int min = (1 << (num_bits - 1));
		int *word_code;
		if (secondary)
			word_code = WordClusterTable::secondaryGet(word, lowercase);
		else if (domain)
			word_code = WordClusterTable::domainGet(word, lowercase);
		else
			word_code = WordClusterTable::get(word, lowercase);
		if ((word_code != NULL) && ((*word_code) >= min)) {
			int max = (1 << num_bits) - 1;
			int prefix = *word_code;
			while (prefix > max)
				prefix >>= 1;
			return prefix;
		}
		return 0;
	}

	void calculatePrefixes(Symbol word, bool lowercase, bool domain, bool secondary) {
		int *word_code;
		if (secondary)
			word_code = WordClusterTable::secondaryGet(word, lowercase);
		else if (domain)
			word_code = WordClusterTable::domainGet(word, lowercase);
		else
			word_code = WordClusterTable::get(word, lowercase);
		if (word_code == NULL) {
			_c4 = 0;
			_c6 = 0;
			_c8 = 0;
			_c12 = 0;
			_c16 = 0;
			_c20 = 0;
			return;
		}

		int prefix = *word_code;

		if (prefix >= min20) {
			while (prefix > max20) 
				prefix >>=1;
			_c20 = prefix;
		} else 
			_c20 = 0;

		if (prefix >= min16) {
			while (prefix > max16) 
				prefix >>=1;
			_c16 = prefix;
		} else 
			_c16 = 0;

		if (prefix >= min12) {
			while (prefix > max12) 
				prefix >>=1;
			_c12 = prefix;
		} else 
			_c12 = 0;

		if (prefix >= min8) {
			while (prefix > max8) 
				prefix >>=1;
			_c8 = prefix;
		} else 
			_c8 = 0;

		if (prefix >= min6) {
			while (prefix > max6) 
				prefix >>=1;
			_c6 = prefix;
		} else 
			_c6 = 0;

		if (prefix >= min4) {
			while (prefix > max4) 
				prefix >>=1;
			_c4 = prefix;
		} else 
			_c4 = 0;
	}
};



#endif
