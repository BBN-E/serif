/**
 * Composite collection types that are unlikely to change.
 *
 * @file ContainerTypes.h
 * @author afrankel@bbn.com
 * @date 2011.06.08
 **/

#pragma once
#include "Generic/common/bsp_declare.h"
#include "boost/make_shared.hpp"
#include "PredFinder/common/ElfTemplate.h"
#include <map>
#include <set>
#include <string>
#include <vector>
class ElfIndividual;
class ElfRelation;
class ElfRelationArg;
BSP_DECLARE(ElfIndividual);
BSP_DECLARE(ElfRelation);
BSP_DECLARE(ElfRelationArg);

typedef std::pair<std::wstring, std::wstring> PairOfStrings;
typedef std::map<std::wstring, std::wstring> StringReplacementMap;
typedef std::set<std::wstring> SetOfStrings;
typedef std::set<int> SetOfInts;
typedef std::map<std::pair<std::wstring, std::wstring>, SetOfStrings> StringPairToSetOfStringsMap;
typedef std::map<std::wstring, SetOfStrings> StringToSetOfStringsMap;
typedef std::map<std::wstring, SetOfInts> StringToSetOfIntsMap;
typedef std::map<int, SetOfStrings> IntToSetOfStringsMap;

/**
 * Comparison callable so we can be sure of getting a stable sort.
 *
 * @author afrankel@bbn.com
 * @date 2011.04.21
 **/
typedef ptr_less_than<ElfIndividual> ElfIndividual_less_than;
typedef ptr_less_than<ElfRelationArg> ElfRelationArg_less_than;

/**
 * Stores ElfIndividuals, sorting them. Intended for use
 * in ElfDocument::to_xml() for collecting the set of all found
 * individuals. Note: ElfIndividual itself is defined in
 * ElfIndividual.h.
 *
 * @author nward@bbn.com
 * @date 2011.06.06
 **/
typedef std::set<ElfIndividual_ptr, ElfIndividual_less_than> ElfIndividualSortedSet;
typedef std::set<ElfRelationArg_ptr, ElfRelationArg_less_than> ElfRelationArgSortedSet;

/**
 * Stores ElfIndividuals. Intended for use in an ElfDocument.
 *
 * @author nward@bbn.com
 * @date 2011.07.07
 **/
typedef std::set<ElfIndividual_ptr> ElfIndividualSet;

/**
 * Defines a simple string to ElfRelation dictionary
 * of lists, intended for use in an ElfIndividualClusterMember
 * for collecting the relations relevant to a particular
 * individual. Keys should be the name of the relation.
 *
 * @author nward@bbn.com
 * @date 2010.08.24
 **/
typedef std::map<std::wstring, std::vector<ElfRelation_ptr> > ElfRelationMap;

/**
 * Container for storing pair-wise mappings between individual URIs.
 *
 * @author nward@bbn.com
 * @date 2011.06.08
 **/
typedef std::map<std::wstring, std::wstring> ElfIndividualUriMap;

/**
 * Defines a dictionary (key = string; value = vector of ElfRelationArg elements)
 * intended for use in an ElfIndividualClusterMember
 * for collecting the arguments relevant to a particular
 * individual. Role name is used as a key. Generally, a role name 
 * will point to a single arg. However, the pattern may yield multiple 
 * args for a given role name, all of which are retained until we
 * split the original relation into multiple relations, each one
 * containing a unique instance of a particular arg. Thus, the value
 * pointed to by the key is a vector of args rather than a single
 * arg, though the size of the vector is usually 1 even in earlier stages,
 * and is always 1 after the uniquify step has been performed.
 * Identical roles from different relations shouldn't be mixed.
 *
 * @author nward@bbn.com
 * @date 2010.08.25
 **/
typedef std::map<std::wstring, std::vector<ElfRelationArg_ptr> > ElfRelationArgMap;
