/**
 * Class that provides a convenience constructor for
 * a common rename operation: generating RDF triples
 * from punned ElfRelations.
 *
 * @file MakeTripleOperator.h
 * @author nward@bbn.com
 * @date 2010.10.27
 **/

#pragma once

#include "Generic/common/Sexp.h"
#include "RenameOperator.h"
#include "boost/shared_ptr.hpp"

/**
 * Syntactic sugar convenience subclass that performs
 * a specialized rename operation. Works just like a
 * RenameOperator, but reads from a compact predicate,
 * subject, object definition.
 *
 * @author nward@bbn.com
 * @date 2010.10.27
 **/
class MakeTripleOperator : public RenameOperator {
public:
	MakeTripleOperator(Sexp* sexp);

	Symbol get_operator_symbol() const { return ReadingMacroOperator::MAKE_TRIPLE_SYM; }

	// Uses the version of retrieve_predicates_generated() defined by RenameOperator.
};

/**
 * Shared pointer for use in STL containers.
 *
 * @author nward@bbn.com
 * @date 2010.10.27
 **/
typedef boost::shared_ptr<MakeTripleOperator> MakeTripleOperator_ptr;
