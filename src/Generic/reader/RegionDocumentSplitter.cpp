// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/reader/RegionDocumentSplitter.h"
#include "Generic/reader/RegionSpanCreator.h"
#include "Generic/theories/Metadata.h"
#include "Generic/theories/Region.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapStatus.h"
#include <boost/foreach.hpp>

RegionDocumentSplitter::RegionDocumentSplitter() {
	// Read the optional threshold parameter
	_max_document_chars = ParamReader::getOptionalIntParamWithDefaultValue("document_splitter_max_size", 20000);
}

std::vector<Document*> RegionDocumentSplitter::splitDocument(Document* document) {
	// Figure out the document splits by region
	const Region* const* regions = document->getRegions();
	int nRegions = document->getNRegions();
	int regionGroupSize = 0;
	std::vector< std::vector<RegionSource> > regionGroups;
	std::vector<RegionSource> regionGroup;
	for (int r = 0; r < nRegions; r++) {
		// Split regions if necessary; may just return the original region
		std::vector<RegionSource> splitRegions;
		splitRegion(regions[r], splitRegions);

		// Group the split regions as they fit (there may just be one)
		BOOST_FOREACH(RegionSource region, splitRegions) {
			int regionSize = region.first->getEndEDTOffset().value() - region.first->getStartEDTOffset().value();
			if (regionGroupSize > 0 && regionGroupSize + regionSize > _max_document_chars) {
				// The current region would put this split document beyond the limit, so store the current grouping
				regionGroups.push_back(std::vector<RegionSource>(regionGroup));
				regionGroup.clear();
				regionGroupSize = 0;
			}

			// Add this region to the current split document grouping
			regionGroup.push_back(region);
			regionGroupSize += regionSize;
		}
	}
	regionGroups.push_back(std::vector<RegionSource>(regionGroup));

	// Construct the documents based on the new region grouping, or pass through the original doc if no split
	std::vector<Document*> split;
	if (regionGroups.size() == 1) {
		split.push_back(document);
	} else {
		// Create a new document for each group of regions, with an indexed document ID based on the original ID
		int splitIndex = 0;
		Symbol regionSpanSymbol = Symbol(L"REGION_SPAN");

		BOOST_FOREACH(regionGroup, regionGroups) {
			// Deep copy the regions, so we can free the original document
			std::vector<Region*> splitRegions;
			int regionNumber = 0;
			BOOST_FOREACH(RegionSource region, regionGroup) {
				// We renumber the regions of the new split documents so we don't get inconsistency in SerifXML
				splitRegions.push_back(_new Region(NULL, region.first->getRegionTag(), regionNumber, const_cast<LocatedString*>(region.first->getString())));
				regionNumber++;

				// Free any regions that were split (i.e. they don't exist in the original document)
				if (region.second && region.first != NULL)
					delete region.first;
			}

			// Create new Metadata from the split subset of region spans like we do in document reader
			Metadata* splitMetadata = _new Metadata();
			splitMetadata->addSpanCreator(regionSpanSymbol, _new RegionSpanCreator());
			BOOST_FOREACH(Region* splitRegion, splitRegions) {
				Symbol tag = splitRegion->getRegionTag();
				splitMetadata->newSpan(regionSpanSymbol, splitRegion->getStartEDTOffset(), splitRegion->getEndEDTOffset(), &tag);
			}

			// Create the document from the copied regions and store the original text which we need later lest we segfault
			std::wostringstream splitName;
			splitName << document->getName() << L"." << std::setfill(L'0') << std::setw(3) << splitIndex;
			splitIndex++;
			Document* splitDocument = _new Document(Symbol(splitName.str()), static_cast<int>(splitRegions.size()), &splitRegions[0]);
			splitDocument->copyOriginalText(document);
			splitDocument->setSourceType(document->getSourceType());
			splitDocument->setMetadata(splitMetadata);
			if (document->getDateTimeField() != NULL)
				splitDocument->setDateTimeField(_new LocatedString(*(document->getDateTimeField())));
			
			// Document time fields
			boost::optional<boost::posix_time::time_period> documentTimePeriod = document->getDocumentTimePeriod();
			if (documentTimePeriod)
				splitDocument->setDocumentTimePeriod(documentTimePeriod->begin(), documentTimePeriod->end());
			boost::optional<boost::local_time::posix_time_zone> documentTimeZone = document->getDocumentTimeZone();
			if (documentTimeZone)
				splitDocument->setDocumentTimeZone(*documentTimeZone);

			splitDocument->setIsDowncased(document->isDowncased());
			split.push_back(splitDocument);
		}
		
		// We made deep copies, so free the original document
		delete document;
	}
	return split;
}

void RegionDocumentSplitter::splitRegion(const Region* region, std::vector<RegionSource> &regions) {
	// If the region is small enough, just pass it through without splitting it further
	int regionSize = region->getEndEDTOffset().value() - region->getStartEDTOffset().value();
	if (regionSize < _max_document_chars) {
		regions.push_back(RegionSource(const_cast<Region*>(region), false));
	} else {
		// Find region boundaries
		std::vector<RegionBound> regionSubstringBounds;
		int currentStartPos = 0;
		int currentEndPos = _max_document_chars;
		while (currentStartPos >= 0 && currentEndPos >= 0 && currentStartPos < region->getString()->length() && currentEndPos < region->getString()->length()) {
			bool foundBreak = false;

			// 1. Look for periods followed by newline-containing whitespace
			for (int p = currentEndPos; p >= currentStartPos; p--) {
				if (p >= 0 && p < region->getString()->length() && region->getString()->charAt(p) == L'.') {
					// Search forward from period for whitespace
					bool foundNewline = false;
					int lastWhitespace = p + 1;
					for (int w = p + 1; w <= currentEndPos; w++) {
						if (w >= 0 && w < region->getString()->length()) {
							if (!iswspace(region->getString()->charAt(w))) {
								lastWhitespace = w - 1;
								break;
							}
							if (region->getString()->charAt(w) == L'\n')
								foundNewline = true;
						}
					}
					if (foundNewline) {
						regionSubstringBounds.push_back(RegionBound(currentStartPos, p));
						currentStartPos = lastWhitespace + 1;
						currentEndPos = currentStartPos + _max_document_chars;
						foundBreak = true;
						break;
					}
				}
			}
			if (foundBreak)
				continue;

			// 2. Look for periods followed by a space
			for (int p = currentEndPos; p >= currentStartPos; p--) {
				if (p >= 0 && p < region->getString()->length() - 1 && region->getString()->charAt(p) == L'.' && iswspace(region->getString()->charAt(p + 1))) {
					regionSubstringBounds.push_back(RegionBound(currentStartPos, p));
					currentStartPos = p + 2;
					currentEndPos = currentStartPos + _max_document_chars;
					foundBreak = true;
					break;
				}
			}
			if (foundBreak)
				continue;

			// 3. Break on any whitespace
			for (int w = currentEndPos; w >= currentStartPos; w--) {
				if (w >= 0 && w < region->getString()->length() && iswspace(region->getString()->charAt(w))) {
					regionSubstringBounds.push_back(RegionBound(currentStartPos, w));
					currentStartPos = w + 1;
					currentEndPos = currentStartPos + _max_document_chars;
					foundBreak = true;
					break;
				}
			}
			if (foundBreak)
				continue;

			// 4. Just break at the limit
			regionSubstringBounds.push_back(RegionBound(currentStartPos, currentEndPos));
			currentStartPos = currentEndPos + 1;
			currentEndPos = currentStartPos + _max_document_chars;
		}
		if (currentStartPos < region->getString()->length() - 2)
			regionSubstringBounds.push_back(RegionBound(currentStartPos, region->getString()->length() - 1));

		// Build new subregions based on split heuristics
		BOOST_FOREACH(RegionBound regionSubstringBound, regionSubstringBounds) {
			// Region numbers don't matter here because we're renumbering when we build the split documents
			//   We need to free the substring, since the Region constructor makes its own copy
			LocatedString* regionSubstring = region->getString()->substring(regionSubstringBound.first, regionSubstringBound.second);
			regions.push_back(RegionSource(_new Region(NULL, region->getRegionTag(), 0, regionSubstring), true));
			delete regionSubstring;
		}
	}
}

DocTheory* RegionDocumentSplitter::mergeDocTheories(std::vector<DocTheory*> splitDocTheories) {
	if (splitDocTheories.size() == 0)
		throw UnrecoverableException("RegionDocumentSplitter::mergeDocuments", "Expected at least one document for merge");
	if (splitDocTheories.size() == 1)
		// Pass through
		return splitDocTheories[0];
	else {
		DocTheory* mergedDocTheory = _new DocTheory(splitDocTheories);
		HeapStatus heapStatus;
		SessionLogger::dbg("kba-stream-corpus-item") << "[memory] merge peak: " << heapStatus.getHeapSize();
		BOOST_FOREACH(DocTheory* splitDocTheory, splitDocTheories) {
			delete splitDocTheory->getDocument();
			delete splitDocTheory;
		}
		return mergedDocTheory;
	}
}
