// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/common/en_CBRNEWordConstants.h"
#include "Generic/common/Symbol.h"

#include <iostream>

Symbol EnglishCBRNEWordConstants::OF = Symbol(L"of");
Symbol EnglishCBRNEWordConstants::IN = Symbol(L"in");
Symbol EnglishCBRNEWordConstants::FOR = Symbol(L"for");
Symbol EnglishCBRNEWordConstants::ON = Symbol(L"on");
Symbol EnglishCBRNEWordConstants::AT = Symbol(L"at");
Symbol EnglishCBRNEWordConstants::WITH = Symbol(L"with");
Symbol EnglishCBRNEWordConstants::BY = Symbol(L"by");
Symbol EnglishCBRNEWordConstants::AS = Symbol(L"as");
Symbol EnglishCBRNEWordConstants::FROM = Symbol(L"from");
Symbol EnglishCBRNEWordConstants::ABOUT = Symbol(L"about");
Symbol EnglishCBRNEWordConstants::INTO = Symbol(L"into");
Symbol EnglishCBRNEWordConstants::AFTER = Symbol(L"after");
Symbol EnglishCBRNEWordConstants::OVER = Symbol(L"over");
Symbol EnglishCBRNEWordConstants::SINCE = Symbol(L"since");
Symbol EnglishCBRNEWordConstants::UNDER = Symbol(L"under");
Symbol EnglishCBRNEWordConstants::LIKE = Symbol(L"like");
Symbol EnglishCBRNEWordConstants::BEFORE = Symbol(L"before");
Symbol EnglishCBRNEWordConstants::UNTIL = Symbol(L"until");
Symbol EnglishCBRNEWordConstants::DURING = Symbol(L"during");
Symbol EnglishCBRNEWordConstants::THROUGH = Symbol(L"through");
Symbol EnglishCBRNEWordConstants::AGAINST = Symbol(L"against");
Symbol EnglishCBRNEWordConstants::BETWEEN = Symbol(L"between");
Symbol EnglishCBRNEWordConstants::WITHOUT = Symbol(L"without");
Symbol EnglishCBRNEWordConstants::BELOW = Symbol(L"below");

bool EnglishCBRNEWordConstants::isCBRNEReportedPreposition(Symbol word) {
	std::cout << "HELLO THERE" << std::endl;
	return (word == OF ||           
		word == IN ||           
		word == FOR ||          
		word == ON ||           
		word == AT ||           
		word == WITH ||         
		word == BY ||           
		word == AS ||           
		word == FROM ||         
		word == ABOUT ||        
		word == INTO ||         
		word == AFTER ||        
		word == OVER ||         
		word == SINCE ||        
		word == UNDER ||        
		word == LIKE ||         
		word == BEFORE ||       
		word == UNTIL ||        
		word == DURING ||       
		word == THROUGH ||      
		word == AGAINST ||      
		word == BETWEEN ||      
		word == WITHOUT ||      
		word == BELOW);
}


