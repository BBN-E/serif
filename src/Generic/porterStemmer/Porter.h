// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef PORTER_H
#define PORTER_H
/*
This code was downloaded from 
http://www.tartarus.org/~martin/PorterStemmer/c_thread_safe.txt
http://www.tartarus.org/~martin/PorterStemmer/index.html
"All these encodings of the algorithm can be used free of charge for any purpose."
The perl version of the IR system uses an implementation from the same website (Porter.pm)

I have made minor changes to make it easier to use this with c++, but I haven't really moved to 
an OO design (marjorie)

*/

/* This is the Porter stemming algorithm, coded up as thread-safe ANSI C
   by the author.

   It may be be regarded as cononical, in that it follows the algorithm
   presented in

   Porter, 1980, An algorithm for suffix stripping, Program, Vol. 14,
   no. 3, pp 130-137,

   only differing from it at the points maked --DEPARTURE-- below.

   See also http://www.tartarus.org/~martin/PorterStemmer

   The algorithm as described in the paper could be exactly replicated
   by adjusting the points of DEPARTURE, but this is barely necessary,
   because (a) the points of DEPARTURE are definitely improvements, and
   (b) no encoding of the Porter stemmer I have seen is anything like
   as exact as this version, even with the points of DEPARTURE!

   You can compile it on Unix with 'gcc -O3 -o stem stem.c' after which
   'stem' takes a list of inputs and sends the stemmed equivalent to
   stdout.

   The algorithm as encoded here is particularly fast.

   Release 2 (the more old-fashioned, non-thread-safe version may be
   regarded as release 1.)
*/

#include <stdlib.h>  /* for malloc, free */
#include <string.h>  /* for memcmp, memmove */
#include "Generic/common/Symbol.h"

/* You will probably want to move the following declarations to a central
   header file.
*/

class Porter{
public:
/* stemmer is a structure for a few local bits of data,
*/

typedef struct {
   char * b;       /* buffer for word to be stemmed */
   int k;          /* offset to the end of the string */
   int j;          /* a general offset into the string */
} stemmer;


	static stemmer * create_stemmer(void);
	static void free_stemmer(stemmer * z);
	static int stem(stemmer * z, char * b, int k);
	static int stem(char * b, int k);
	static Symbol stem(Symbol s);

private:
		static stemmer* _stemmer;
		static int cons(stemmer * z, int i);
		static int m(stemmer * z);
		static int vowelinstem(stemmer * z);
		static int doublec(stemmer * z, int j);
		static int cvc(stemmer * z, int i);
		static int ends(stemmer * z, const char * s);
		static void setto(stemmer * z, const char * s);
		static void r(stemmer * z, const char * s);
		static void step1ab(stemmer * z);
		static void step1c(stemmer * z);
		static void step2(stemmer * z);
		static void step3(stemmer * z);
		static void step4(stemmer * z);
		static void step5(stemmer * z);



};
#endif
