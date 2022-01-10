// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/common/en_AdeptWordConstants.h"
#include "Generic/common/Symbol.h"

Symbol EnglishAdeptWordConstants::OF = Symbol(L"of");
Symbol EnglishAdeptWordConstants::IN = Symbol(L"in");
Symbol EnglishAdeptWordConstants::FOR = Symbol(L"for");
Symbol EnglishAdeptWordConstants::ON = Symbol(L"on");
Symbol EnglishAdeptWordConstants::AT = Symbol(L"at");
Symbol EnglishAdeptWordConstants::WITH = Symbol(L"with");
Symbol EnglishAdeptWordConstants::BY = Symbol(L"by");
Symbol EnglishAdeptWordConstants::AS = Symbol(L"as");
Symbol EnglishAdeptWordConstants::FROM = Symbol(L"from");
Symbol EnglishAdeptWordConstants::ABOUT = Symbol(L"about");
Symbol EnglishAdeptWordConstants::INTO = Symbol(L"into");
Symbol EnglishAdeptWordConstants::AFTER = Symbol(L"after");
Symbol EnglishAdeptWordConstants::OVER = Symbol(L"over");
Symbol EnglishAdeptWordConstants::SINCE = Symbol(L"since");
Symbol EnglishAdeptWordConstants::UNDER = Symbol(L"under");
Symbol EnglishAdeptWordConstants::LIKE = Symbol(L"like");
Symbol EnglishAdeptWordConstants::BEFORE = Symbol(L"before");
Symbol EnglishAdeptWordConstants::UNTIL = Symbol(L"until");
Symbol EnglishAdeptWordConstants::DURING = Symbol(L"during");
Symbol EnglishAdeptWordConstants::THROUGH = Symbol(L"through");
Symbol EnglishAdeptWordConstants::AGAINST = Symbol(L"against");
Symbol EnglishAdeptWordConstants::BETWEEN = Symbol(L"between");
Symbol EnglishAdeptWordConstants::WITHOUT = Symbol(L"without");
Symbol EnglishAdeptWordConstants::BELOW = Symbol(L"below");

bool EnglishAdeptWordConstants::isADEPTReportedPreposition(Symbol word) {
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


