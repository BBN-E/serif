// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/LogMath.h"
#include "Generic/common/SessionLogger.h"

/**
 * Implementation based on http://nlp.cs.byu.edu/mediawiki/index.php/Log_Domain_Computations
 *   Derived from Dan Klein's SloppyMath package
 *
 * This is a close approximation of \f$log(x + y)\f$ as \f$log(x) + log(1.0 + e^{log(y) - log(x)})\f$.
 * The calculation is slower due to the exponentiation, but this avoids floating point underflow
 * by keeping all values in log space.
 *
 * This version operates on 32-bit floats, since we store our scores as floats (not doubles).
 *
 * @author nward@bbn.com
 * @date 2010.02.11
 **/
float LogMath::logadd(float logX, float logY) {
	// Make sure that X is larger so we only do one set of comparisons
	if (logY > logX) {
		// Swap X and Y, since Y was larger
		float temp = logX;
		logX = logY;
		logY = temp;
	}

	// We consistently use -10000 as the smallest log space value
	if (logX == -10000) {
		// The largest of the two arguments is the smallest value we can represent, so adding is a NOOP
		return logX;
	}

	// How many orders of magnitude larger is X than Y?
	float negDiff = logY - logX;
	if (negDiff < -20) {
		// Y is more than 20 orders of magnitude smaller, so X dominates the addition.
		return logX;
	}

	// Calculate the addition in log space using the algebraically manipulated form
	float result = logX + log(1.0f + exp(negDiff));
	return result;
}

/**
 * Runs unit tests on each of the logarithmic math static methods defined in the LogMath class.
 * This shouldn't be run in production builds.
 *
 * @author nward@bbn.com
 * @date 2010.02.11
 **/
void LogMath::test(void) {
	//--- LogMath::logadd ---//
	SessionLogger::info("SERIF") << "Testing float version of LogMath::logadd..." << std::endl;

	// If the largest of either argument is the smallest value we represent in logspace (-10000), we should get that smallest value
	assert(LogMath::logadd(-10000, -10001) == -10000);
	assert(LogMath::logadd(-10001, -10000) == -10000);
	assert(LogMath::logadd(-10000, -10000) == -10000);

	// If either argument is more than 20 orders of magnitude smaller, it is ignored in the addition
	assert(LogMath::logadd(-10, -40) == -10);
	assert(LogMath::logadd(-40, -10) == -10);

	// Make sure that argument ordering doesn't matter
	assert(LogMath::logadd(-1, -2) == LogMath::logadd(-2, -1));
	assert(LogMath::logadd(-0.07428346f, -0.008234723f) == LogMath::logadd(-0.008234723f, -0.07428346f)); 

	// Check a few random unconditional additions and make sure they return within some small epsilon
	assert(LogMath::logadd(-1, -2) - -0.68673831f < 0.000001f);
	assert(LogMath::logadd(-1.3579f, -2.468f) - -1.07307751f < 0.000001f);

	//--- Done ---//
	SessionLogger::info("SERIF") << "LogMath tests complete!" << std::endl;
}
