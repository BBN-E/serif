// Copyright 2016 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/results/MSAResultCollector.h"
#include "Generic/theories/ActorEntitySet.h"
#include "Generic/actors/AWAKEActorInfo.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

namespace {
	Symbol PER = Symbol(L"PER");
	Symbol ORG = Symbol(L"ORG");
	Symbol GPE = Symbol(L"GPE");
}

MSAResultCollector::MSAResultCollector() {
	// 0.45 so if two countries get put corefereneced together in the same entity, we will report them both
	_min_gpe_confidence = ParamReader::getOptionalFloatParamWithDefaultValue("msa_xml_min_gpe_confidence", 0.45); 
	_min_per_confidence = ParamReader::getOptionalFloatParamWithDefaultValue("msa_xml_min_per_confidence", 0.95);
	_min_org_confidence = ParamReader::getOptionalFloatParamWithDefaultValue("msa_xml_min_org_confidence", 0.95);

	_actorInfo = AWAKEActorInfo::getAWAKEActorInfo();
}

void MSAResultCollector::loadDocTheory(DocTheory* theory) {
	_docTheory = theory;
}

std::wstring MSAResultCollector::produceOutputHelper() {
	std::wstringstream wss;
	wss << L"<MSAXML>\n";
	std::vector<ActorEntity_ptr> actorEntities = _docTheory->getActorEntitySet()->getAll();
	BOOST_FOREACH(ActorEntity_ptr ae, actorEntities) {
		ActorId aid = ae->getActorId();
		if (aid.isNull()) continue;
		Symbol entityTypeSym = _actorInfo->getEntityTypeForActor(ae->getActorId());

		ActorId countryActorId;
		Symbol isoCode;
		std::vector<ActorId> associatedCountries = _actorInfo->getAssociatedCountryActorIds(ae->getActorId());
		if (associatedCountries.size() == 1) {
			countryActorId = associatedCountries[0];
			isoCode = _actorInfo->getIsoCodeForActor(countryActorId);
		}
		if (entityTypeSym == PER && ae->getConfidence() >= _min_per_confidence) {
			wss << "    <Person actor_id=\"" << aid.getId() << "\"";
			if (!countryActorId.isNull()) {
				wss << " country_actor_id=\"" << countryActorId.getId()
					<< "\" country_name=\"" << OutputUtil::escapeXML(_actorInfo->getActorName(countryActorId))
					<< "\" country_iso_code=\"" << isoCode << "\"";
			}
			wss << ">";
			wss << OutputUtil::escapeXML(_actorInfo->getActorName(aid)) << "</Person>\n";
		}
		if (entityTypeSym == ORG && ae->getConfidence() >= _min_org_confidence) {
			wss << "    <Organization actor_id=\"" << aid.getId() << "\"";
			if (!countryActorId.isNull()) {
				wss << " country_actor_id=\"" << countryActorId.getId()
					<< "\" country_name=\"" << OutputUtil::escapeXML(_actorInfo->getActorName(countryActorId))
					<< "\" country_iso_code=\"" << isoCode << "\"";
			}
			wss << ">";
			wss << OutputUtil::escapeXML(_actorInfo->getActorName(aid)) << "</Organization>\n";
		}
		// countries
		if (entityTypeSym == GPE && ae->getConfidence() >= _min_gpe_confidence && _actorInfo->isACountry(aid)) {
			Symbol isoCode = _actorInfo->getIsoCodeForActor(aid);
			if (!isoCode.is_null()) {
				wss << "    <Country actor_id=\"" << aid.getId() << "\" iso_code=\"" 
					<< isoCode << "\">" << OutputUtil::escapeXML(_actorInfo->getActorName(aid)) << "</Country>\n";
			}

		} 
		// non-country geonames
		if (entityTypeSym == GPE && ae->getConfidence() >= _min_gpe_confidence && !_actorInfo->isACountry(aid)) {
			Gazetteer::GeoResolution_ptr georesolution = getGeoResolutionForActorEntity(ae);
			if (georesolution && georesolution->countryInfo) {
				ActorId countryActorId = georesolution->countryInfo->actorId;
				Symbol isoCode = _actorInfo->getIsoCodeForActor(countryActorId);
				if (!isoCode.is_null()) {
					wss << "    <Geoname actor_id=\"" << aid.getId() 
						<< "\" country_actor_id=\"" << countryActorId.getId() 
						<< "\" country_name=\"" << OutputUtil::escapeXML(_actorInfo->getActorName(countryActorId))
						<< "\" country_iso_code=\"" << isoCode
						<< "\">" << OutputUtil::escapeXML(_actorInfo->getActorName(aid)) << "</Geoname>\n";
				}
			}
		}
	}

	wss << L"</MSAXML>\n";
	return wss.str();

}

void MSAResultCollector::produceOutput(const wchar_t *output_dir,
									   const wchar_t *doc_filename)
{
	// File name
	std::wstring output_file = std::wstring(output_dir) + LSERIF_PATH_SEP + std::wstring(doc_filename);
	if (!boost::algorithm::ends_with(output_file, L".txt")) {
		output_file += L".msa.xml";
	} else {
		output_file = boost::algorithm::replace_last_copy(output_file, ".txt", ".msa.xml");
	}

	// Open output stream
	UTF8OutputStream stream;
	stream.open(output_file.c_str());

	std::wstring results = produceOutputHelper();

	// Write out
	stream << results;

	stream.close();
}

void MSAResultCollector::produceOutput(std::wstring *results) { *results = produceOutputHelper(); }

Gazetteer::GeoResolution_ptr MSAResultCollector::getGeoResolutionForActorEntity(ActorEntity_ptr ae) {
	BOOST_FOREACH(ProperNounActorMention_ptr pnam, ae->getActorMentions()) {
		if (pnam->isResolvedGeo())
			return pnam->getGeoResolution();
	}
	return Gazetteer::GeoResolution_ptr();
}
