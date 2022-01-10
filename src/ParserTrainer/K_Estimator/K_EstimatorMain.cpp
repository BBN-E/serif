// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"


#include "parse/ParserTrainer/K_Estimator.h"
#include "common/Symbol.h"
#include "common/UnexpectedInputException.h"
#include "Generic/common/FeatureModule.h"


int main(int argc, char* argv[]) {
   	if (argc == 17) {
		try {

		FeatureModule::load(argv[16]);
		
		K_Estimator k_est(argv[1], argv[2], argv[3], argv[4], argv[5], 
			argv[6], argv[7], argv[8], argv[9], argv[10], 
			argv[11], 
			atoi(argv[12]),
			atof(argv[13]), atof(argv[14]), atof(argv[15]));
		k_est.estimate();

		} catch (UnexpectedInputException uc) {
			uc.putMessage(std::cerr);
			return -1;
		} 

	} else {
	    cerr << "wrong number of arguments to K_Estimator\n";
		cerr << "Usage: \n";
		cerr << "  10 input files:\n";
		cerr << "    smoothing corpus event counts (head, left, right, pre, post)\n"; 
		cerr << "	 main corpus event counts (head, left, right, pre, post)\n";
		cerr << "  1 output file\n";
		cerr << "  5 parameters:\n";
		cerr << "    min_history_count increment min max language\n\n";
		cerr << "(language parameter is new: it should be English, Arabic, etc)\n";
        return -1;
    }

	return 0;
}
