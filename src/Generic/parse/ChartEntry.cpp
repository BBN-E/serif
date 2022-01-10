// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstddef>
#include "Generic/parse/ChartEntry.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/common/SessionLogger.h"


const int ChartEntry::maxChain = CHART_ENTRY_MAX_CHAIN;
const size_t ChartEntry::blockSize = CHART_ENTRY_BLOCK_SIZE;
ChartEntry* ChartEntry::freeList = 0;
SequentialBigrams *ChartEntry::sequentialBigrams = 0;
//ParseNode* ChartEntry::name_premods = 0;

ParseNode* ChartEntry::toParseNode()
{
    if (isPreterminal) {
		if (nameType == ParserTags::nullSymbol) {
			ParseNode* wordNode = _new ParseNode(headWord, leftToken, rightToken);
			ParseNode* result = _new ParseNode(constituentCategory, leftToken, rightToken);
			result->headNode = wordNode;
			//for debugging-
			/*
			SessionLogger::dbg("ChartEntry_non_name_node") << "NonNameNode: " << result->toDebugString()
				<< "(chart_start_index = " << result->chart_start_index 
				<< ", chart_end_index = " << result->chart_end_index << ")\n";
			*/
			//end debugging
			return result;
		} else {
			ParseNode* result = _new ParseNode(nameType, leftToken, rightToken);
			result->headNode = _new ParseNode(headTag, rightToken, rightToken);
			result->headNode->headNode = _new ParseNode(headWord, rightToken, rightToken);
			//for debugging-
			/*
			ParseNode* dbg = result->headNode->headNode;
			SessionLogger::dbg("ChartEntry_name_node") << "NameNode: head: " << dbg->toDebugString()
				<<" start: "<<dbg->chart_start_index
				<<" end: "<<dbg->chart_end_index<<std::endl;
			*/
			//end debugging
			if (leftToken != rightToken) {
				result->premods = _new ParseNode(headTag, rightToken-1, rightToken-1);
				result->premods->headNode = _new ParseNode(headWord, rightToken-1, rightToken-1);
				//for debugging-
				/*
				dbg = result->premods->headNode;
				SessionLogger::dbg("ChartEntry_name_node") << "NameNode-Premod: head: " << dbg->toDebugString()
					<<" start: "<<dbg->chart_start_index
					<<" end: "<<dbg->chart_end_index<<std::endl;
				*/
				//end debugging
				ParseNode* iterator = result->premods;
				for (int i = rightToken - 2; i >= leftToken; i--) {
					iterator->next = _new ParseNode(headTag, i, i);
					iterator->next->headNode = _new ParseNode(headWord, i, i);
					//for debugging-
					/*
					dbg = iterator->next->headNode;
					SessionLogger::dbg("ChartEntry_name_node") << "NameNode-Premod: " << dbg->toDebugString()
						<<" start: "<<dbg->chart_start_index
						<<" end: "<<dbg->chart_end_index<<std::endl;
					*/
					//end debugging
					iterator = iterator->next;
				}
			}
			//for debugging-
			/*
			SessionLogger::dbg("ChartEntry_name_node") << "NameNode: " << result->toDebugString()
				<< "(chart_start_index = " << result->chart_start_index 
				<< ", chart_end_index = " << result->chart_end_index << ")\n";
			*/
			//end debugging
			result->isName = true;
			return result;
		}
	}
    if (bridgeType == BRIDGE_TYPE_KERNEL) {
        Symbol headChain = kernelOp->headChain;
        Symbol modChain = kernelOp->modifierChain;
        ParseNode* result = _new ParseNode(constituentCategory, leftToken, rightToken);
        if (kernelOp->branchingDirection == BRANCH_DIRECTION_RIGHT) {
            ParseNode* head = leftChild->toParseNode();
            result->headNode = addChain(head, headChain);
			ParseNode* mod = rightChild->toParseNode();
            result->postmods = addChain(mod, modChain);
        } else {
            ParseNode* head = rightChild->toParseNode();
            result->headNode = addChain(head, headChain);
			ParseNode* mod = leftChild->toParseNode();
            result->premods = addChain(mod, modChain);
        }
        //for debugging-
        /*
        SessionLogger::dbg("ChartEntry_bridge_type_kernel") << "BridgeTypeKernel: " << result->toDebugString() 
        	<< "(chart_start_index = " << result->chart_start_index << "/" << leftToken
        	<< ", chart_end_index = " << result->chart_end_index << "/" << rightToken << ")\n";
        */
        //end debugging
        return result;
    } else {
        Symbol modChain = extensionOp->modifierChain;
        if (extensionOp->branchingDirection == BRANCH_DIRECTION_LEFT) {
            ParseNode* mod = leftChild->toParseNode();
            mod = addChain(mod, modChain);
            ParseNode* result = rightChild->toParseNode();
            if (result->premods == 0) {
                result->premods = mod;
            } else {
                ParseNode* lastMod = result->premods;
                while (lastMod->next)
                    lastMod = lastMod->next;
                lastMod->next = mod;
            }
            result->chart_start_index = mod->chart_start_index;
            //for debugging-
            /*
            SessionLogger::dbg("ChartEntry_modChain") << "ModChain left: " << result->toDebugString()
            	<< "(chart_start_index = " << result->chart_start_index 
            	<< ", chart_end_index = " << result->chart_end_index << ")\n";
            */
            //end debugging
            return result;
        } else {
            ParseNode* mod = rightChild->toParseNode();
            mod = addChain(mod, modChain);
            ParseNode* result = leftChild->toParseNode();
            if (result->postmods == 0) {
                result->postmods = mod;
            } else {
                ParseNode* lastMod = result->postmods;
                while (lastMod->next)
                    lastMod = lastMod->next;
                lastMod->next = mod;
            }
            result->chart_end_index = mod->chart_end_index;
            //for debugging-
            /*
            SessionLogger::dbg("ChartEntry_modChain") << "ModChain right: " << result->toDebugString()
            	<< "(chart_start_index = " << result->chart_start_index 
            	<< ", chart_end_index = " << result->chart_end_index << ")\n";
            */
            //end debugging
            return result;
        }
    }
}

ParseNode* ChartEntry::addChain(ParseNode* base, const Symbol &chainSym)
{
    Symbol chain[CHART_ENTRY_MAX_CHAIN];
    int chainLength = 0;
    const wchar_t* p = chainSym.to_string();
    p = wcschr(p, L'=');
    if (p == 0) {
        return base;
    }
    const wchar_t* p1 = chainSym.to_string();
    const wchar_t* p2;
    wchar_t buffer[50];
    size_t len;
    while ((p2 = wcschr(p1, '=')) != 0) {
        len = p2 - p1;
        wcsncpy(buffer, p1, len);
        buffer[len] = '\0';
        chain[chainLength++] = Symbol(buffer);
        p1 = p2 + 1;
    }
    ParseNode* p3 = base;
    for (int i = (chainLength - 1); i >= 0; --i) {
        if (p3 == base && base->isName &&
			LanguageSpecificFunctions::isCoreNPLabel(chain[i])) {
			// do nothing
		} else {
			ParseNode* p4 = _new ParseNode(chain[i], p3->chart_start_index, p3->chart_end_index);
			p4->headNode = p3;
			p3 = p4;
			//for debugging-
			/*
			SessionLogger::dbg("ChartEntry_addChain") << "Chain result: " << p3->toDebugString()
				<< "(chart_start_index = " << p3->chart_start_index 
				<< ", chart_end_index = " << p3->chart_end_index << ")\n";
			*/
			//end debugging
		}	
    }
    return p3;
}

void* ChartEntry::operator new(size_t)
{
    ChartEntry* p = freeList;
    if (p) {
        freeList = p->next;
    } else {
		//std::cerr << "allocating new ChartEntry block" << std::endl;
        ChartEntry* newBlock = static_cast<ChartEntry*>(::operator new(
            blockSize * sizeof(ChartEntry)));
        for (size_t i = 1; i < (blockSize - 1); i++)
            newBlock[i].next = &newBlock[i + 1];
        newBlock[blockSize - 1].next = 0;
        p = newBlock;
        freeList = &newBlock[1];
    }
    return p;
}

void ChartEntry::operator delete(void* object)
{
    ChartEntry* p = static_cast<ChartEntry*>(object);
    p->next = freeList;
    freeList = p;
}



