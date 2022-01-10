// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/results/MTResultSaver.h"
#include "Generic/theories/DocTheory.h"

#include "Generic/common/limits.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/PartOfSpeech.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueType.h"
#include "Generic/common/Segment.h"
#include "Generic/common/memory_ext.h"
#include "Generic/reader/MTDocumentReader.h"
#include "Generic/theories/SentenceTheoryBeam.h"

#include <boost/lexical_cast.hpp>
#include <string>
#include <sstream>


using boost::lexical_cast;


void MTResultSaver::resetForNewDocument(DocTheory* docTheory, const wchar_t * output_dir) {
	/* Note:
		A fundamental assumption of the Segment format is that fields involved in defining document
		content (ie, segment-input-field) are immutable once created. This allows the use of Segments
		in an add-and-passthrough processing pipeline, where components work with a portion of a
		Segment's content, add their output, and pass through all other Segment state. Because
		arbitrary and application-specific content can be stored with Segments, it's not feasible to
		write a single mapping for Segment attributes and fields when re-segmentation is performed.
		However, there are applications where re-segmentation is needed. Therefor, the collector
		explicitly checks for indications of re-segmentation, and will generate entirely new 
		Segments if it has occured. It's up to later application stages to determine what state
		from source Segments should be mapped, and how that mapping should occur.
	*/
	
	_docTheory = docTheory;
	Document * document = _docTheory->getDocument();
	int n_segments      = document->getNSegments();
	
	// Figure out what field we extracted to construct the document, so our output can refer to it
	// Or, if we're creating a new segment file, we'll write the sentence located strings into it
	std::string ref_source_narrow = ParamReader::getRequiredParam("segment_input_field");
	_ref_source = std::wstring( ref_source_narrow.begin(), ref_source_narrow.end());
	
	// Local copy of segments to be extended w/ Serif state
	_segments = document->getSegments();
	

	std::string docPath = _docTheory->getDocument()->getName().to_debug_string();
	InputUtil::normalizePathSeparators(docPath);
	boost::filesystem::path docPathBoost(docPath);
	std::string doc_filename = BOOST_FILESYSTEM_PATH_GET_FILENAME(docPathBoost);
	std::stringstream doc_out_fname;
	doc_out_fname << output_dir << SERIF_PATH_SEP << doc_filename << ".segments";
	
	std::cerr << "Writing to segment-format file " << doc_out_fname.str() << std::endl;
	_dout = _new UTF8OutputStream( doc_out_fname.str().c_str() );
	if( _dout->fail() )
		throw UnrecoverableException( "MTResultSaver::resetForNewDocument",
		(std::string("Unable to open ") + doc_out_fname.str() + " for reading.").c_str() );

}

void MTResultSaver::cleanUpAfterDocument() {
	_dout->close();
	delete _dout;
}

void MTResultSaver::produceSentenceOutput(int cur_sent_no, SentenceTheoryBeam* sent_theory_beam) {
	// expand segments with Serif structures, and write to the doc stream
		
		const Sentence * sent        = _docTheory->getSentence(cur_sent_no);
		SentenceTheory * sent_theory = sent_theory_beam->getTheory(0);
		TokenSequence  * sent_tokens = sent_theory->getTokenSequence();
		const LocatedString * sent_lstr = _docTheory->getSentence(cur_sent_no)->getString();
		
		// Get the names
		NameTheory* nameTheory = sent_theory->getNameTheory();
		if (nameTheory) {
			
			std::wstringstream names_val;
			for (int i = 0; i < nameTheory->getNNameSpans(); i++) {
				NameSpan * span = nameTheory->getNameSpan(i);
				names_val << span->type.getName().to_string() << ":";
				names_val << sent_lstr->positionOfStartOffset(sent_tokens->getToken(span->start)->getStartEDTOffset()) << ":";
				names_val << sent_lstr->positionOfEndOffset(sent_tokens->getToken(span->end)->getEndEDTOffset()) << " ";
			}
			
			// add a names field to the segment
			WSegment::field_entry_t fe;
			fe.value = names_val.str();
			fe.attributes[L"refers"] = _ref_source;
			_segments[cur_sent_no][L"names"].push_back( fe );
		}
		
		// Get the values
		ValueMentionSet* valueMentionSet = sent_theory->getValueMentionSet();
		if (valueMentionSet) {
			
			std::wstringstream values_val;
			for (int i = 0; i < valueMentionSet->getNValueMentions(); i++) {
				ValueMention * ment = valueMentionSet->getValueMention(i);
				values_val << ment->getFullType().getNameSymbol().to_string() << ":";
				values_val << sent_lstr->positionOfStartOffset(sent_tokens->getToken(ment->getStartToken())->getStartEDTOffset()) << ":";
				values_val << sent_lstr->positionOfEndOffset(sent_tokens->getToken(ment->getEndToken())->getEndEDTOffset()) << " ";
			}
			
			// add a values field to the segment
			WSegment::field_entry_t fe;
			fe.value = values_val.str();
			fe.attributes[L"refers"] = _ref_source;
			_segments[cur_sent_no][L"values"].push_back( fe );
		}

		// Get the N best parses
		int n_theories = sent_theory_beam->getNTheories();
		for (int i = 0; i < n_theories; i++){
			SentenceTheory * cur_sent_theory = sent_theory_beam->getTheory(i);
			Parse * parse = cur_sent_theory->getFullParse();
			if (parse && parse->getRoot()->getChild(0)->getTag() != Symbol(L"-empty-")){

				std::wstringstream parse_val;
				std::wstringstream heads_val;
				
				int numNodes = 0, heads[4096], nodeStarts[4096], nodeEnds[4096]; Symbol tags[4096];
				parse->getRoot()->getAllNonterminalNodesWithHeads( heads, nodeStarts, nodeEnds, tags, numNodes, 4096 );
			
				for( int j = 0; j < numNodes; j++ ){
					int start = sent_lstr->positionOfStartOffset(sent_tokens->getToken(nodeStarts[j])->getStartEDTOffset());
				    int end   = sent_lstr->positionOfEndOffset(sent_tokens->getToken(nodeEnds[j])->getEndEDTOffset());
					if(tags[j] == Symbol(L":"))
						tags[j] = Symbol(L"-COLON-");
					parse_val << tags[j].to_string() << ":" << start << ":" << end << " ";
					heads_val << heads[j] << " ";
				}
			
				// add a parse field to the segment
				WSegment::field_entry_t fe;
				fe.value = parse_val.str();
				fe.attributes[L"refers"] = _ref_source;
				char buf[5];
				std::string str = _itoa(i, buf, 10);
				std::wstring wstr(str.length(), L' ');
				copy(str.begin(), str.end(), wstr.begin());
				fe.attributes[L"i"] = wstr;
				_segments[cur_sent_no][L"parse"].push_back( fe );
	
				if (ParamReader::isParamTrue("mt_result_parse_output_heads")) {
					// add a heads field to the segment
					WSegment::field_entry_t fe;
					fe.value = heads_val.str();
					fe.attributes[L"refers"] = _ref_source;
					fe.attributes[L"i"] = wstr;
					_segments[cur_sent_no][L"heads"].push_back( fe );
				}
			}
		}

		// write to the output stream!
		*_dout << _segments[cur_sent_no];
}
