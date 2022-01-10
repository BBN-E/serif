// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/ParamReader.h"
#include "Generic/edt/LexEntity.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/edt/NameLinkFunctions.h"
#include "Generic/theories/Entity.h"
#include "Generic/theories/EntityType.h"
#include <math.h>

double LexEntity::LEX_ENT_LOG_OF_ZERO = -10000;
double LexEntity::unseen_weights[];

DebugStream &LexEntity::_debugOut = DebugStream::referenceResolverStream;

LexEntity::LexEntity(EntityType type) : 
	_totalCount(0), _type(type), 
	_use_edit_distance(false), _edit_threshold(3) 
{
	_use_edit_distance = ParamReader::isParamTrue("namelink_use_edit_distance");

	if (_use_edit_distance)
		_edit_threshold = ParamReader::getRequiredIntParam("namelink_edit_distance_threshold");

	_debugOut << "\n ......Creating LexEntity of type "
			  << type.getName().to_debug_string();
}
LexEntity::LexEntity(const LexEntity &other) : _totalCount(other._totalCount), _type(other._type),
	_use_edit_distance(other._use_edit_distance), _edit_threshold(other._edit_threshold),
	_wordCounts(other._wordCounts) { }

double LexEntity::estimate(Symbol words[], int nWords) {
	CountsTable tempCounts = CountsTable();
	int i, tempTotalCount;
	double tempCount;
	double acc = 0.0;
	double unseen_weight = unseen_weights[_type.getNumber()];
	NameLinkFunctions::recomputeCounts(_wordCounts, tempCounts, tempTotalCount);
	//tempCounts = CountsTable(_wordCounts);
	//tempTotalCount = _totalCount;

	if(_totalCount == 0)
		return LEX_ENT_LOG_OF_ZERO;

	for(i=0; i<nWords; i++) {
		tempCount = (double)tempCounts[words[i]];
		if (_use_edit_distance) {
			if (tempCount == 0 && wcslen(words[i].to_string()) >= (size_t)_edit_threshold) {
				for (CountsTable::iterator iter = tempCounts.begin(); iter != tempCounts.end(); ++iter) {
					int dist = editDistance(words[i], iter.value().first);
					if (wcslen(iter.value().first.to_string()) >= (size_t)_edit_threshold) {
						if (dist == 1) {
							tempCount += (0.75 * iter.value().second);
						}
						//else if (dist == 2) {
						//	tempCount += (0.5 * iter.value().second);
						//}
					}
				}
			}
		}
		acc += log (
			(1-unseen_weight)*(tempCount / tempTotalCount) +
			unseen_weight * (1.0/10000));
		_debugOut << " [" << words[i].to_debug_string() << ", " 
			<< tempCount << " / " << tempTotalCount << "] ";
	}

	_debugOut << " <";
	for(CountsTable::iterator iter = tempCounts.begin(); iter != tempCounts.end(); ++iter) {
		if(iter!=tempCounts.begin())
			_debugOut << "  ";
		_debugOut << iter.value().first.to_debug_string() << ":" << iter.value().second;
	}
	_debugOut << "> ";

	return acc;
}

void LexEntity::learn(Symbol words[], int nWords) {
	int i;

	CountsTable oldCounts = _wordCounts;
	NameLinkFunctions::recomputeCounts(oldCounts, _wordCounts, _totalCount);

	_debugOut << "\nLearning " << _totalCount;
	for(i=0; i<nWords; i++) {
		_wordCounts.add(words[i]);
		_totalCount++;
	}
	_debugOut << " " <<_totalCount;
}

int LexEntity::editDistance(Symbol sym1, Symbol sym2) const {
	int cost_del = 1;
	int cost_ins = 1;
	int cost_sub = 1;
	int i,j;

	const wchar_t *s1 = sym1.to_string();
	const wchar_t *s2 = sym2.to_string();
	int n1 = static_cast<int>(wcslen(s1));
	int n2 = static_cast<int>(wcslen(s2));

	int *dist = _new int[(n1+1)*(n2+1)];

	dist[0] = 0;

	for (i = 1; i <= n1; i++) 
		dist[i*(n2+1)] = dist[(i-1)*(n2+1)] + cost_del;

	for (j = 1; j <= n2; j++) 
		dist[j] = dist[j-1] + cost_ins;

	for (i = 1; i <= n1; i++) {
		for (j = 1; j <= n2; j++) {
			int dist_del = dist[(i-1)*(n2+1)+j] + cost_del;
			int dist_ins = dist[i*(n2+1)+(j-1)] + cost_ins;
			int dist_sub = dist[(i-1)*(n2+1)+(j-1)] + 
							(s1[i-1] == s2[j-1] ? 0 : cost_sub);
			dist[i*(n2+1)+j] = std::min(std::min(dist_del, dist_ins), dist_sub);
		}
	}
	int tmp = dist[n1*(n2+1)+n2];
	delete [] dist;

	return tmp;
}
