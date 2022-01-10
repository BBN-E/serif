// Copyright 2016 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MSA_RESULT_COLLECTOR_H
#define MSA_RESULT_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/common/OutputStream.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/actors/Gazetteer.h"
#include "Generic/theories/ActorEntity.h"

class DocTheory;

class MSAResultCollector : public ResultCollector {
public:
	MSAResultCollector();
	virtual ~MSAResultCollector() {}
	virtual void finalize() {}

	virtual void loadDocTheory(DocTheory* docTheory);
	
	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t* document_filename);
	virtual void produceOutput(std::wstring *results);

private:
	DocTheory *_docTheory;
	ActorInfo_ptr _actorInfo;
	std::wstring produceOutputHelper();

	double _min_gpe_confidence;
	double _min_per_confidence;
	double _min_org_confidence;

	Gazetteer::GeoResolution_ptr getGeoResolutionForActorEntity(ActorEntity_ptr ae);

};

#endif
