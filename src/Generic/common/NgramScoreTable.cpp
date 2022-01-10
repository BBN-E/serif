// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/NgramScoreTable.h"

#include <limits>

template <size_t N>
const float NgramScoreTableGen<N>::targetLoadingFactor = static_cast<float>(0.7);


template <size_t N>
NgramScoreTableGen<N>::NgramScoreTableGen(UTF8InputStream& stream)
  : N_flexible(N), 
    numEntries(get_num_entries(stream)), 
    numBuckets(get_num_buckets(numEntries)), 
    table(numBuckets, hasher, eqTester), 
    size(numEntries)
{

    UTF8Token token;

    for (int i = 0; i < numEntries; i++) {

        Symbol* ngram = _new Symbol[N];

        stream >> token;

        if (token.symValue() != SymbolConstants::leftParen) {
          char c[100];
          sprintf( c, "ERROR 0: ill-formed ngram record at entry: %d", i );
		  throw UnexpectedInputException("NgramScoreTableGen::()", c);
        }

        stream >> token;
        if (token.symValue() != SymbolConstants::leftParen) {
          char c[100];
          sprintf( c, "ERROR 1: ill-formed ngram record at entry: %d", i );
          throw UnexpectedInputException("NgramScoreTableGen::()",c);
        }

        for (size_t j = 0; j < N; j++) {
            stream >> token;
            ngram[j] = token.symValue();
        }

        stream >> token;
        if (token.symValue() != SymbolConstants::rightParen) {
          char c[100];
          sprintf( c, "ERROR 2: ill-formed ngram record at entry: %d", i );
          throw UnexpectedInputException("NgramScoreTableGen::()",c);
        }

        float score;
        stream >> score;
	
		// check for "inf" score value
		if (!stream) {
			stream.clear();
			stream.ignore(numeric_limits<streamsize>::max(), L')');
			stream.putBack(L')');
			score = numeric_limits<float>::max();
		}

        stream >> token;
        if (token.symValue() != SymbolConstants::rightParen) {
          char c[100];
          sprintf( c, "ERROR 3: ill-formed ngram record at entry: %d", i );
          throw UnexpectedInputException("NgramScoreTableGen::()",c);
        }

        table[ngram] = score;

    }
}


template <size_t N>
NgramScoreTableGen<N>::NgramScoreTableGen(int init_size)
  : N_flexible(N), 
    numEntries(0), 
    numBuckets(get_num_buckets(init_size)), 
    table(numBuckets, hasher, eqTester), 
    size(0)
{
}


template <size_t N>
NgramScoreTableGen<N>::NgramScoreTableGen(size_t n, UTF8InputStream& stream)
  : N_flexible(n), 
    hasher(HashKey<N>(n)), 
    eqTester(EqualKey<N>(n)), 
    numEntries(get_num_entries(stream)), 
    numBuckets(get_num_buckets(numEntries)), 
    table(numBuckets, hasher, eqTester), 
    size(numEntries)
{

    UTF8Token token;

    for (int i = 0; i < numEntries; i++) {

        Symbol* ngram = _new Symbol[N_flexible];

        stream >> token;

        if (token.symValue() != SymbolConstants::leftParen) {
          char c[100];
          sprintf( c, "ERROR 4: ill-formed ngram record at entry: %d", i );
		  throw UnexpectedInputException("NgramScoreTableGen::()", c);
        }

        stream >> token;
        if (token.symValue() != SymbolConstants::leftParen) {
          char c[100];
          sprintf( c, "ERROR 5: ill-formed ngram record at entry: %d", i );
          throw UnexpectedInputException("NgramScoreTableGen::()",c);
        }

        for (size_t j = 0; j < N_flexible; j++) {
            stream >> token;
            ngram[j] = token.symValue();
        }

        stream >> token;
        if (token.symValue() != SymbolConstants::rightParen) {
          char c[100];
          sprintf( c, "ERROR 6: ill-formed ngram record at entry: %d", i );
          throw UnexpectedInputException("NgramScoreTableGen::()",c);
        }

        float score;
        stream >> score;

		// check for "inf" score value
		if (!stream) {
			stream.clear();
			stream.ignore(numeric_limits<streamsize>::max(), L')');
			stream.putBack(L')');
			score = numeric_limits<float>::max();
		}

        stream >> token;
        if (token.symValue() != SymbolConstants::rightParen) {
          char c[100];
          sprintf( c, "ERROR 7: ill-formed ngram record at entry: %d", i );
          throw UnexpectedInputException("NgramScoreTableGen::()",c);
        }

        table[ngram] = score;

    }
}


template <size_t N>
NgramScoreTableGen<N>::NgramScoreTableGen(size_t n, int init_size)
  : N_flexible(n), 
    hasher(HashKey<N>(n)), 
    eqTester(EqualKey<N>(n)), 
    numEntries(0), 
    numBuckets(get_num_buckets(init_size)), 
    table(numBuckets, hasher, eqTester), 
    size(0)
{
}


template <size_t N>
NgramScoreTableGen<N>::~NgramScoreTableGen() {
	typename Table::iterator iter = table.begin();
	while (iter != table.end()) {
		Symbol *tmp = (*iter).first;
		++iter;
		delete [] tmp;
	}
}


template <size_t N>
void NgramScoreTableGen<N>::add(Symbol* ngram)
{
	add(ngram, 1);
}

// creates new Symbol[] iff it's a new key
template <size_t N>
void NgramScoreTableGen<N>::add(Symbol* ngram, float value)
{

  typename Table::iterator iter = table.find(ngram);
  if (iter == table.end()) {
    if (N > 0) {
      Symbol* new_ngram = _new Symbol[N];
      for (size_t j = 0; j < N; j++) {
        new_ngram[j] = ngram[j];
      }
      table[new_ngram] = value;
      size += 1;
    } else {
      Symbol* new_ngram = _new Symbol[N_flexible];
      for (size_t j = 0; j < N_flexible; j++) {
        new_ngram[j] = ngram[j];
      }
      table[new_ngram] = value;
      size += 1;
    }
  } else {
    (*iter).second += value;
  }

}

template <size_t N>
void NgramScoreTableGen<N>::print(const char *filename)
{
	//ofstream out;
	UTF8OutputStream out;
	out.open(filename);
	print_to_open_stream (out);

	out.close();

	return;
}

// prints ((ngram) score)
template <size_t N>
void NgramScoreTableGen<N>::print_to_open_stream(UTF8OutputStream& out) 
{

  out << size;
  out << "\n";

  size_t N_local = N;
  if (N == 0) {
    N_local = N_flexible;
  }
  
  for (typename Table::iterator iter = table.begin() ; iter != table.end() ; ++iter) {
    out << "((";
    for (size_t j = 0; j < N_local - 1; j++) {
      out << (*iter).first[j].to_string();
      out << " ";
    }
    out << (*iter).first[N_local - 1].to_string();
  
    out << ") ";
    out << (*iter).second;
    out << ")";
    out << "\n";
  }
  
}

// for use with min_history_count threshold
template <size_t N>
NgramScoreTableGen<N>* NgramScoreTableGen<N>::prune(int threshold)
{
  if (N > 0) {
    NgramScoreTableGen<N>* new_table = _new NgramScoreTableGen<N>(size);
    
    //typename Table::iterator iter;
    
    for (typename Table::iterator iter = table.begin() ; iter != table.end() ; ++iter) {
      if ((*iter).second > threshold) {
        new_table->add((*iter).first, (*iter).second);
      }
    }
    return new_table;
  } else {
    return NULL;
  }
  
}


// used in k-estimation
template <size_t N>
void NgramScoreTableGen<N>::reset()
{
	typename Table::iterator iter;

	for (iter = table.begin() ; iter != table.end() ; ++iter) {
		(*iter).second = 0;
	}

}

template <size_t N>
int NgramScoreTableGen<N>::get_num_entries(UTF8InputStream& stream) {
	int num = 0; 
	if (!stream.eof()) {
		stream >> num; 
	}
	return num; 
}

template <size_t N>
int NgramScoreTableGen<N>::get_num_buckets(int init_size) { 
  int num = static_cast<int>(init_size / targetLoadingFactor);
          
  // numBuckets must be >= 1,
  //  but we set it to a minimum of 5, just to be safe
  // only relevant for very very small training sets, where
  //  there are no events of a certain type (e.g. left lexical)
  if (num < 5) {
    num = 5;
  }
  return num;
}

template class NgramScoreTableGen<0>;
template class NgramScoreTableGen<1>;
template class NgramScoreTableGen<2>;
template class NgramScoreTableGen<3>;
template class NgramScoreTableGen<4>;
template class NgramScoreTableGen<5>;
template class NgramScoreTableGen<6>;
template class NgramScoreTableGen<7>;
template class NgramScoreTableGen<8>;
template class NgramScoreTableGen<9>;
