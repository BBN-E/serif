// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SPECIFIC_RELATION_VECTOR_MODELS_H
#define SPECIFIC_RELATION_VECTOR_MODELS_H

// unifier
#include "Generic/relations/RelationStats.h"

// probability models
#include "Generic/relations/BackoffProbModel.h"
#include "Generic/relations/BackoffToPriorProbModel.h"

class TypeFeatureVectorModel: public RelationStats {
public:
	/** Create and return a new TypeFeatureVectorModel. */
	static TypeFeatureVectorModel *build(UTF8InputStream &stream) { 
		return dynamic_cast<TypeFeatureVectorModel*>(_factory()->build(stream)); }
	// for training - construct empty structures waiting to be filled in
	static TypeFeatureVectorModel *build() { 
		return dynamic_cast<TypeFeatureVectorModel*>(_factory()->build()); }

	/** Hook for specifying the probablility model and filter classes
	 * that should be used when building new RelationStats instances. */
	template <typename ProbModel, typename Filter>
	static void setModelAndFilter() {
		_factory().reset(_new FactoryFor<TypeFeatureVectorModel, ProbModel, Filter>()); }

private:
	static boost::shared_ptr<Factory> &_factory(); 	
	TypeFeatureVectorModel(RelationProbModel *probModel, RelationFilter *filter) 
		: RelationStats(probModel, filter) {}
	template <typename A, typename B, typename C>
	friend RelationStats *FactoryFor<A, B, C>::build();
	template <typename A, typename B, typename C>
	friend RelationStats *FactoryFor<A, B, C>::build(UTF8InputStream &stream);
};

class TypeB2PFeatureVectorModel: public RelationStats {
public:
	/** Create and return a new TypeFeatureVectorModel. */
	static TypeB2PFeatureVectorModel *build(UTF8InputStream &stream) { 
		return dynamic_cast<TypeB2PFeatureVectorModel*>(_factory()->build(stream)); }
	// for training - construct empty structures waiting to be filled in
	static TypeB2PFeatureVectorModel *build() { 
		return dynamic_cast<TypeB2PFeatureVectorModel*>(_factory()->build()); }

	/** Hook for specifying the probablility model and filter classes
	 * that should be used when building new RelationStats instances. */
	template <typename ProbModel, typename Filter>
	static void setModelAndFilter() {
		_factory().reset(_new FactoryFor<TypeB2PFeatureVectorModel, ProbModel, Filter>()); }

private:
	static boost::shared_ptr<Factory> &_factory(); 	
	TypeB2PFeatureVectorModel(RelationProbModel *probModel, RelationFilter *filter) 
		: RelationStats(probModel, filter) {}
	template <typename A, typename B, typename C>
	friend RelationStats *FactoryFor<A, B, C>::build();
	template <typename A, typename B, typename C>
	friend RelationStats *FactoryFor<A, B, C>::build(UTF8InputStream &stream);
};

// For backwards compatibility:
typedef TypeFeatureVectorModel type_feature_vector_model_t;
typedef TypeB2PFeatureVectorModel type_b2p_feature_vector_model_t;



// // filters
// #ifdef ENGLISH_LANGUAGE
// 	#include "English/relations/en_FeatureVectorFilter.h"
// 	#include "English/relations/en_ExpFeatureVectorFilter.h"
// #elif defined(ARABIC_LANGUAGE)	
// 	#include "Arabic/relations/ar_ExpFeatureVectorFilter.h"
// 	#include "Generic/relations/xx_FeatureVectorFilter.h"
// #elif defined(CHINESE_LANGUAGE)
// 	#include "Chinese/relations/ch_ExpFeatureVectorFilter.h"
// 	#include "Generic/relations/xx_FeatureVectorFilter.h"
// #else	
// 	#include "Generic/relations/xx_FeatureVectorFilter.h"
// #endif

// #ifdef ENGLISH_LANGUAGE
// 	typedef RelationStats<BackoffProbModel<2>,FeatureVectorFilter> type_feature_vector_model_t;
// 	typedef RelationStats<BackoffProbModel<3>,ExpFeatureVectorFilter> type_b2p_feature_vector_model_t;
// #elif defined(ARABIC_LANGUAGE)
// 	typedef RelationStats<BackoffProbModel<2>,FeatureVectorFilter> type_feature_vector_model_t;
// 	typedef RelationStats<BackoffProbModel<2>,ExpFeatureVectorFilter> type_b2p_feature_vector_model_t;
// #else	
// 	typedef RelationStats<BackoffProbModel<2>,FeatureVectorFilter> type_feature_vector_model_t;
// 	typedef RelationStats<BackoffProbModel<2>,FeatureVectorFilter> type_b2p_feature_vector_model_t;
// #endif

#endif
