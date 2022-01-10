// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/parse/ch_WordFeatures.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


const unsigned ChineseWordFeatures::numDerivationalFeatures = 
    WORD_FEATURES_NUM_DERIV_FEATURES;

ChineseWordFeatures::ChineseWordFeatures()
{
		derivationalFeatures[0] = L'\x4ebf';
		derivationalFeatures[1] = L'\x4e07';
        derivationalFeatures[2] = L'\x5e74';
        derivationalFeatures[3] = L'\x591a';
        derivationalFeatures[4] = L'\x65e5';
        derivationalFeatures[5] = L'\x6027';
        derivationalFeatures[6] = L'\x4f1a';
        derivationalFeatures[7] = L'\x4e1a';
        derivationalFeatures[8] = L'\x56fd';
        derivationalFeatures[9] = L'\x4e94';
        derivationalFeatures[10] = L'\x4e00';
        derivationalFeatures[11] = L'\x56db';
        derivationalFeatures[12] = L'\x90e8';
        derivationalFeatures[13] = L'\x5316';
        derivationalFeatures[14] = L'\x516d';
        derivationalFeatures[15] = L'\x4e8c';
        derivationalFeatures[16] = L'\x4e09';
        derivationalFeatures[17] = L'\x5e02';
        derivationalFeatures[18] = L'\x957f';
        derivationalFeatures[19] = L'\x4eba';
        derivationalFeatures[20] = L'\x4e5d';
        derivationalFeatures[21] = L'\x533a';
        derivationalFeatures[22] = L'\x516b';
        derivationalFeatures[23] = L'\x4e03';
        derivationalFeatures[24] = L'\x51fa';
        derivationalFeatures[25] = L'\x529b';
        derivationalFeatures[26] = L'\x4e8e';
        derivationalFeatures[27] = L'\xff10';
        derivationalFeatures[28] = L'\x5341';
        derivationalFeatures[29] = L'\x961f';
        derivationalFeatures[30] = L'\x6cd5';
        derivationalFeatures[31] = L'\x673a';
        derivationalFeatures[32] = L'\x5382';
        derivationalFeatures[33] = L'\x5730';
        derivationalFeatures[34] = L'\x91cf';
        derivationalFeatures[35] = L'\x5230';
        derivationalFeatures[36] = L'\x5c14';
        derivationalFeatures[37] = L'\x8005';
        derivationalFeatures[38] = L'\x989d';
        derivationalFeatures[39] = L'\x5c40';
        derivationalFeatures[40] = L'\x6e2f';
        derivationalFeatures[41] = L'\x5165';
        derivationalFeatures[42] = L'\x5bb6';
        derivationalFeatures[43] = L'\x5236';
        derivationalFeatures[44] = L'\x54c1';
        derivationalFeatures[45] = L'\x7701';
        derivationalFeatures[46] = L'\x70b9';
        derivationalFeatures[47] = L'\x4e9a';
        derivationalFeatures[48] = L'\x6708';
        derivationalFeatures[49] = L'\x578b';
        derivationalFeatures[50] = L'\x5ea6';
        derivationalFeatures[51] = L'\x9762';
        derivationalFeatures[52] = L'\x65b9';
        derivationalFeatures[53] = L'\x7387';
        derivationalFeatures[54] = L'\x65af';
        derivationalFeatures[55] = L'\x754c';
        derivationalFeatures[56] = L'\x5458';
        derivationalFeatures[57] = L'\x519b';
        derivationalFeatures[58] = L'\x5c71';
        derivationalFeatures[59] = L'\x4e0a';
        derivationalFeatures[60] = L'\x8d44';
        derivationalFeatures[61] = L'\x5934';
        derivationalFeatures[62] = L'\x6743';
        derivationalFeatures[63] = L'\x5dde';
        derivationalFeatures[64] = L'\x751f';
        derivationalFeatures[65] = L'\x7ebf';
        derivationalFeatures[66] = L'\x8d5b';
        derivationalFeatures[67] = L'\x4eec';
        derivationalFeatures[68] = L'\x5de5';
        derivationalFeatures[69] = L'\x6d77';
        derivationalFeatures[70] = L'\x514b';
        derivationalFeatures[71] = L'\x62a5';
        derivationalFeatures[72] = L'\x5357';
        derivationalFeatures[73] = L'\x4f59';
        derivationalFeatures[74] = L'\x767e';
        derivationalFeatures[75] = L'\x5e73';
        derivationalFeatures[76] = L'\x5b50';
        derivationalFeatures[77] = L'\x4ea7';
        derivationalFeatures[78] = L'\x5b66';
        derivationalFeatures[79] = L'\x91d1';
        derivationalFeatures[80] = L'\x592b';
        derivationalFeatures[81] = L'\x534e';
        derivationalFeatures[82] = L'\x5149';
        derivationalFeatures[83] = L'\x5546';
        derivationalFeatures[84] = L'\x8fbe';
        derivationalFeatures[85] = L'\x56e2';
        derivationalFeatures[86] = L'\x7ea7';
        derivationalFeatures[87] = L'\x6b3e';
        derivationalFeatures[88] = L'\x573a';
        derivationalFeatures[89] = L'\x503c';
        derivationalFeatures[90] = L'\x8f66';
        derivationalFeatures[91] = L'\x52a1';
        derivationalFeatures[92] = L'\x58eb';
        derivationalFeatures[93] = L'\x4e49';
        derivationalFeatures[94] = L'\x53e3';
        derivationalFeatures[95] = L'\x5904';
        derivationalFeatures[96] = L'\x5343';
        derivationalFeatures[97] = L'\x7acb';
        derivationalFeatures[98] = L'\x4f5c';
        derivationalFeatures[99] = L'\x4e8b';
        derivationalFeatures[100] = L'\xff12';
        derivationalFeatures[101] = L'\x7269';
        derivationalFeatures[102] = L'\x8def';
        derivationalFeatures[103] = L'\x4ef6';
        derivationalFeatures[104] = L'\x6570';
        derivationalFeatures[105] = L'\x6c5f';
        derivationalFeatures[106] = L'\x59d4';
        derivationalFeatures[107] = L'\x5fb7';
        derivationalFeatures[108] = L'\x5173';
        derivationalFeatures[109] = L'\x4e1c';
        derivationalFeatures[110] = L'\x5170';
        derivationalFeatures[111] = L'\x8d77';
        derivationalFeatures[112] = L'\x56ed';
		derivationalFeatures[113] = L'\x624b';
        derivationalFeatures[114] = L'\x7ad9';
        derivationalFeatures[115] = L'\x4efd';
        derivationalFeatures[116] = L'\x5f97';
        derivationalFeatures[117] = L'\x529e';
        derivationalFeatures[118] = L'\x5728';
        derivationalFeatures[119] = L'\xff15';
        derivationalFeatures[120] = L'\x5f00';
        derivationalFeatures[121] = L'\x6c14';
        derivationalFeatures[122] = L'\x79d1';
        derivationalFeatures[123] = L'\x5408';
        derivationalFeatures[124] = L'\x8fdb';
        derivationalFeatures[125] = L'\x8fd0';
        derivationalFeatures[126] = L'\x7a0e';
        derivationalFeatures[127] = L'\x4efb';
        derivationalFeatures[128] = L'\x80fd';
        derivationalFeatures[129] = L'\x7535';
        derivationalFeatures[130] = L'\x8bc1';
        derivationalFeatures[131] = L'\x4f4d';
        derivationalFeatures[132] = L'\x73b0';
        derivationalFeatures[133] = L'\x6d41';
        derivationalFeatures[134] = L'\x6218';
        derivationalFeatures[135] = L'\x80a1';
        derivationalFeatures[136] = L'\x5f0f';
        derivationalFeatures[137] = L'\x8272';
        derivationalFeatures[138] = L'\x793e';
        derivationalFeatures[139] = L'\xff11';
        derivationalFeatures[140] = L'\x4ee5';
        derivationalFeatures[141] = L'\x53d6';
        derivationalFeatures[142] = L'\x8bba';
        derivationalFeatures[143] = L'\x5f80';
        derivationalFeatures[144] = L'\x8d39';
        derivationalFeatures[145] = L'\x6599';
        derivationalFeatures[146] = L'\x5206';
        derivationalFeatures[147] = L'\x5b89';
        derivationalFeatures[148] = L'\x5c9b';
        derivationalFeatures[149] = L'\x6587';
        derivationalFeatures[150] = L'\x5e08';
        derivationalFeatures[151] = L'\x7279';
        derivationalFeatures[152] = L'\x897f';
        derivationalFeatures[153] = L'\x7ed9';
        derivationalFeatures[154] = L'\x6240';
        derivationalFeatures[155] = L'\x9662';
        derivationalFeatures[156] = L'\x7403';
        derivationalFeatures[157] = L'\x5c5e';
        derivationalFeatures[158] = L'\x5317';
        derivationalFeatures[159] = L'\x6728';
        derivationalFeatures[160] = L'\x6625';
        derivationalFeatures[161] = L'\x6c34';
        derivationalFeatures[162] = L'\x671b';
        derivationalFeatures[163] = L'\x57ce';
        derivationalFeatures[164] = L'\x672c';
        derivationalFeatures[165] = L'\x76ee';
        derivationalFeatures[166] = L'\x6b65';
        derivationalFeatures[167] = L'\x6d0b';
        derivationalFeatures[168] = L'\x8282';
        derivationalFeatures[169] = L'\x9a6c';
        derivationalFeatures[170] = L'\x5883';
        derivationalFeatures[171] = L'\x53d8';
        derivationalFeatures[172] = L'\xff16';
        derivationalFeatures[173] = L'\x4e66';
        derivationalFeatures[174] = L'\x5e26';
        derivationalFeatures[175] = L'\x7a7a';
        derivationalFeatures[176] = L'\x4e16';
        derivationalFeatures[177] = L'\x5385';
        derivationalFeatures[178] = L'\x4ef7';
        derivationalFeatures[179] = L'\x826f';
        derivationalFeatures[180] = L'\x6536';
        derivationalFeatures[181] = L'\x95f4';
        derivationalFeatures[182] = L'\x662f';
        derivationalFeatures[183] = L'\x6c47';
        derivationalFeatures[184] = L'\x8ba1';
        derivationalFeatures[185] = L'\x53d7';
        derivationalFeatures[186] = L'\x75c5';
        derivationalFeatures[187] = L'\x8425';
        derivationalFeatures[188] = L'\x9500';
        derivationalFeatures[189] = L'\x53bb';

}


bool ChineseWordFeatures::isChinese(const std::wstring& word) const
{
	int i;
	for (i = 0; word[i]; i++)
		if (word[i] > 0xff)
			return 1;
	return 0;
}

		
std::wstring ChineseWordFeatures::derivationalFeature(const std::wstring& word) const
{
    size_t length = word.length();

    // skip single char chinese words
	if (length > 1) {
		for (size_t i = 0; i < numDerivationalFeatures; ++i) {
			size_t j;
			wchar_t feature = derivationalFeatures[i];
			if (((j = word.rfind(feature)) != std::wstring::npos) &&
				(j == (word.length() - 1))) {
					wchar_t cstr[5];
 					swprintf(cstr, 5, L"D%u", i+1);
					return std::wstring(cstr);
			}
		}
    }
    return L"D0";
}

bool ChineseWordFeatures::isChineseDigit(wchar_t c_char) const
{
	if((c_char == 0xff10) ||
       (c_char == 0xff11) ||
       (c_char == 0xff12) ||
       (c_char == 0xff13) ||
       (c_char == 0xff14) ||
       (c_char == 0xff15) ||
       (c_char == 0xff16) ||
       (c_char == 0xff17) ||
       (c_char == 0xff18) ||
       (c_char == 0xff19) ||
       (c_char == 0x4ebf) ||
       (c_char == 0x4e07) ||
       (c_char == 0x5343) ||
       (c_char == 0x767e) ||
       (c_char == 0x5341) ||
       (c_char == 0x4e00) ||
       (c_char == 0x4e8c) ||
       (c_char == 0x4e09) ||
       (c_char == 0x56db) ||
       (c_char == 0x4e94) ||
       (c_char == 0x516d) ||
       (c_char == 0x4e03) ||
       (c_char == 0x516b) ||
       (c_char == 0x4e5d))
		return true;
	return false;
}

bool ChineseWordFeatures::isChineseNumChar(wchar_t c_char) const
{
	if ((c_char == 0x96f6) ||
        (c_char == 0x4e24) ||
        (c_char == 0x70b9) ||
        (c_char == 0x591a) ||
        (c_char == 0x4f59) ||
        (c_char == 0x30fb) ||
        (c_char == 0x5206) ||
        (c_char == 0x4e4b))
			return true;
	return false;
}
			

bool ChineseWordFeatures::isChineseNumber(const std::wstring& word) const {
	size_t i = 0;
	size_t length = word.length();
	bool found_number = false;
	
	for ( ; i < length; ++i) {
		if (isChineseDigit(word[i])) {
			found_number = true;
			continue;
		}
		if (isChineseNumChar(word[i])) 
			continue;
		return false;
	}
	if (found_number) {
		return true;
	} else return false;
}

bool ChineseWordFeatures::isEnglishNumber(const std::wstring& word) const {
	size_t i = 0;
	size_t length = word.length();
	bool found_number = false;

	if ( (word[0] == '+') || (word[0] == '-') )
		i++;
	for ( ; i < length; ++i) {
		if (isascii(word[i]) && isdigit(word[i])) {
			found_number = true;
			continue;
		}
		if (word[i] == '.')
			continue;
		if ( (tolower(word[i]) == 'e') &&
			(i > 0) && isascii(word[i-1]) &&
			isdigit(word[i-1]) &&
			(i < (length - 1)) )
			continue;
		if ( (tolower(word[i]) == 'd') &&
			(i == (length - 1)) )
			continue;
		return false;
	}
	if (found_number) {
		return true;
	} else return false;
}



Symbol ChineseWordFeatures::features(Symbol wordSym, bool firstWord) const
{
	std::wstring wordStr = wordSym.to_string();
	std::wstring returnStr;

    if (isEnglishNumber(wordStr) || isChineseNumber(wordStr)) {
        returnStr = L"XXWFNUM";
    }
	else if (isChinese(wordStr))
	{
		returnStr = L"XXWF";
	    returnStr += derivationalFeature(wordStr);
	}
	else
	{
		returnStr = L"XXWFEN";
	}
    return Symbol(returnStr.c_str());
}


Symbol ChineseWordFeatures::reducedFeatures(Symbol wordSym, bool firstWord) const
{
	std::wstring wordStr = wordSym.to_string();
	std::wstring returnStr;

	if (isEnglishNumber(wordStr) || isChineseNumber(wordStr)) {
        returnStr = L"XXWFNUM";
    }
	else if (isChinese(wordStr))
	{
		returnStr = L"XXWFD0";
	}
	else 
	{ 
		returnStr = L"XXWFEN";
	}
    return Symbol(returnStr.c_str());
}
