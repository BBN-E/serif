// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENAMEX_SPAN_H
#define ENAMEX_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/IdfSpan.h"
#include "Generic/preprocessors/Attributes.h"

namespace DataPreprocessor {


	class EnamexSpan : public IdfSpan {
	public:
		EnamexSpan()
			: IdfSpan() {}
		EnamexSpan(EDTOffset start_offset, EDTOffset end_offset, const Attributes *attributes)
			: IdfSpan(start_offset, end_offset, 
			          attributes->type, attributes->role, attributes->coref_id) {}
		~EnamexSpan();
		virtual Symbol getSpanType() const {
			return Symbol(L"Enamex"); }
	};

} // namespace DataPreprocessor

#endif
