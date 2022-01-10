// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include <cstring>
#include "common/UTF8InputStream.h"
#include "parse/HeadProbDeriver.h"
#include "parse/LexicalProbDeriver.h"
#include "parse/ModifierProbDeriver.h"
#include "parse/ParserTrainer/CategoryProbDeriver.h"
#include "common/UnrecoverableException.h"
#include "common/UnexpectedInputException.h"
#include <boost/scoped_ptr.hpp>

// DIVERSITY CHANGES: NONE

int main(int argc, char* argv[])
{
	if (argc != 14 && argc != 15) {
        cerr << "wrong number of arguments to table deriver\n";
		cerr << "Usage:\n";
		cerr << "   6 pairs of input/output files --\n";
		cerr << "     prior head pre post left right\n";
		cerr << "   minimum history count pruning threshold\n";
		cerr << "   lambdas file (optional)\n";
		return -1;
    }

	try {

	boost::scoped_ptr<UTF8InputStream> lambdas_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& lambdas(*lambdas_scoped_ptr);

	// if lambdas file doesn't exist, use defaults hard-coded
	//   in Deriver classes
	if (argc == 15)
		lambdas.open(argv[14]);
	float f;

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);

	// includes checking for acceptable lambdas
	//   since errors could go otherwise undetected
	
	in.open(argv[1]);
	CategoryProbDeriver* cp = new CategoryProbDeriver(in);
	in.close();
	cp->derive_tables();
	cp->print_tables(argv[2]);

	in.open(argv[3]);
	HeadProbDeriver* hp = new HeadProbDeriver(in);
	in.close();
	if (argc == 15) {
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		hp->set_unique_multiplier_pwt(f);
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		hp->set_unique_multiplier_pt(f);
	}
	hp->derive_tables();
	hp->print_tables(argv[4]);

	in.open(argv[5]);
	ModifierProbDeriver* mp_left = new ModifierProbDeriver(in);
	in.close();
	if (argc == 15) {
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		mp_left->set_unique_multiplier_PHpwt(f);
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		mp_left->set_unique_multiplier_PHpt(f);
	}
	mp_left->derive_tables();
	mp_left->print_tables(argv[6]);

	in.open(argv[7]);
	ModifierProbDeriver* mp_right = new ModifierProbDeriver(in);
	in.close();
	if (argc == 15) {
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		mp_right->set_unique_multiplier_PHpwt(f);
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		mp_right->set_unique_multiplier_PHpt(f);
	}
	mp_right->derive_tables();
	mp_right->print_tables(argv[8]);
	
	// atoi(argv[13]) gives us min_history_count
	in.open(argv[9]);
	LexicalProbDeriver* lp_left = new LexicalProbDeriver(in, atoi(argv[13]));
	in.close();
	if (argc == 15) {
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		lp_left->set_unique_multiplier_MtPHwt(f);
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		lp_left->set_unique_multiplier_MtPHt(f);
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		lp_left->set_unique_multiplier_Mt(f);
	}
	lp_left->derive_tables();
	lp_left->print_tables(argv[10]);

	in.open(argv[11]);
	LexicalProbDeriver* lp_right = new LexicalProbDeriver(in, atoi(argv[13]));
	in.close();
	if (argc == 15) {
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		lp_right->set_unique_multiplier_MtPHwt(f);
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		lp_right->set_unique_multiplier_MtPHt(f);
		f = -1;
		lambdas >> f;
		if (f < 0) throw UnexpectedInputException("DeriveTablesMain:main()",
			"ERROR: problem in lambda file");
		lp_right->set_unique_multiplier_Mt(f);
	}
	lp_right->derive_tables();
	lp_right->print_tables(argv[12]);

	} catch (UnrecoverableException uc) {
		uc.putMessage(std::cerr);
		return -1;

	} 
	
	return 0;

}


