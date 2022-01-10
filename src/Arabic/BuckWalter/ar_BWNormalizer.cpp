// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Arabic/BuckWalter/ar_BWNormalizer.h"
#include "Arabic/common/ar_WordConstants.h"
#include "Arabic/common/ar_ArabicSymbol.h"

void BWNormalizer::normalize(const wchar_t* input, wchar_t* output){
	size_t input_length = wcslen(input);
	size_t output_offset = 0;
	for(size_t char_num = 0; char_num < input_length; char_num++){
		if(!(ArabicWordConstants::isNonKeyCharacter(input[char_num]))){
            	output[output_offset] = input[char_num];
				output_offset++;
		}
	}
	output[output_offset] = L'\0';
	return;
}
void BWNormalizer::normalize(const wchar_t* v_input, const wchar_t* nv_input, wchar_t* output){
	size_t input_length = wcslen(v_input);
	size_t nvlength = wcslen(nv_input);
	size_t output_offset = 0;
	for(size_t char_num = 0; char_num < input_length; char_num++){
		if((output_offset < nvlength) &&
			ArabicWordConstants::isEquivalentChar(v_input[char_num], nv_input[output_offset])){
			output[output_offset] = nv_input[output_offset];
			output_offset++;
		}
		else if(!(ArabicWordConstants::isNonKeyCharacter(v_input[char_num]))){
				//skip final F as well
			if((char_num < (input_length-1)) || 
				(v_input[char_num] != ArabicSymbol::ArabicChar('F'))){
            	output[output_offset] = v_input[char_num];
				output_offset++;
				}
		}
	}
	output[output_offset] = L'\0';
	return;
}
Symbol BWNormalizer::normalize(Symbol input, Symbol nvinput) throw (UnrecoverableException){
	const wchar_t* in = input.to_string();
	int len = static_cast<int>(wcslen(in));
	if(wcslen(in) > 500){
		char message[500];
		sprintf(message,"%s, %d: %s","Input greater than 500 characters, num characters",
			len, input.to_debug_string());
		throw UnrecoverableException("BWNormalizer::normalize()", message);
	}
	wchar_t output[500];
	normalize(in, nvinput.to_string(), output);
	return Symbol(output);
}
Symbol BWNormalizer::normalize(Symbol input) throw (UnrecoverableException){
	const wchar_t* in = input.to_string();
	int len = static_cast<int>(wcslen(in));
	if(wcslen(in) > 500){
		char message[500];
		sprintf(message,"%s, %d: %s","Input greater than 500 characters, num characters",
			len, input.to_debug_string());
		throw UnrecoverableException("BWNormalizer::normalize()", message);
	}
	wchar_t output[500];
	normalize(in, output);
	return Symbol(output);
}

