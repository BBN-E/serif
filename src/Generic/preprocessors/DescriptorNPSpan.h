// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCRIPTOR_NP_SPAN_H
#define DESCRIPTOR_NP_SPAN_H

#include "Generic/common/Symbol.h"
#include "Generic/preprocessors/IdfSpan.h"
#include "Generic/preprocessors/Attributes.h"

namespace DataPreprocessor {


	class DescriptorNPSpan : public IdfSpan {
	public:
		DescriptorNPSpan()
			: IdfSpan() {}
		DescriptorNPSpan(EDTOffset start_offset, EDTOffset end_offset, const Attributes *attributes)
			: IdfSpan(start_offset, end_offset, 
			          attributes->type, attributes->role, attributes->coref_id) {}
		~DescriptorNPSpan();
		virtual Symbol getSpanType() const {
			return Symbol(L"Descriptor"); }

	};

	static std::wstring XML_CLASS_ID;

} // namespace DataPreprocessor

#endif
