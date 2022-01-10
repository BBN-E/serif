// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef RELATION_MODEL_H
#define RELATION_MODEL_H

#include <boost/shared_ptr.hpp>


class PotentialRelationInstance;

class RelationModel {
public:
	/** Create and return a new RelationModel. */
	static RelationModel *build() { return _factory()->build(); }
	static RelationModel *build(const char *file_prefix) { return _factory()->build(file_prefix); }
	/** Hook for registering new RelationModel factories */
	struct Factory { 
		virtual RelationModel *build() = 0; 
		virtual RelationModel *build(const char *file_prefix) = 0; 
	};
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual int findBestRelationType(PotentialRelationInstance *instance) = 0;
	virtual void train(char *vector_file, char* output_file_prefix) = 0;
	void generateVectorsAndTrain(char *training_file, char* vector_file, char* output_file_prefix) {};
	void test(char *test_file) {};
	void trainFromStateFileList(char *training_file, char* output_file_prefix) { }

	virtual ~RelationModel() {}

protected:
	RelationModel() {}

private:
	static boost::shared_ptr<Factory> &_factory();

};

// language-specific includes determine which implementation is used
//#ifdef ENGLISH_LANGUAGE
//	#include "English/relations/en_RelationModel.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/relations/ch_RelationModel.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/relations/ar_RelationModel.h"
//#else	
//	#include "Generic/relations/xx_RelationModel.h"
//#endif


#endif
