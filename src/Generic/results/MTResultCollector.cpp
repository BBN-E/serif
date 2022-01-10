// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.
#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/results/MTResultCollector.h"
#include "Generic/theories/DocTheory.h"

#include "Generic/common/limits.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/NameTheory.h"
#include "Generic/theories/NPChunkTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/PartOfSpeech.h"
#include "Generic/theories/PartOfSpeechSequence.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueType.h"
#include "Generic/common/Segment.h"
#include "Generic/common/memory_ext.h"
#include "Generic/reader/MTDocumentReader.h"

#include <boost/lexical_cast.hpp>
#include <string>
#include <sstream>


using boost::lexical_cast;


void MTResultCollector::loadDocTheory(DocTheory* docTheory) {
	_docTheory = docTheory;
}


void MTResultCollector::produceOutput( const wchar_t * output_dir, const wchar_t * document_filename ) {
	
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
	
	Document * document = _docTheory->getDocument();
	int n_sentences     = _docTheory->getNSentences();
	int n_segments      = document->getNSegments();
	
	// Figure out what field we extracted to construct the document, so our output can refer to it
	// Or, if we're creating a new segment file, we'll write the sentence located strings into it
	std::string ref_source_narrow = ParamReader::getRequiredParam("segment_input_field");
	bool write_tokens = ParamReader::getOptionalTrueFalseParamWithDefaultVal("mr_replace_tokens", false);
	std::wstring ref_source( ref_source_narrow.begin(), ref_source_narrow.end() );
	
	// Local copy of segments to be extended w/ Serif state
	std::vector<WSegment> segments = document->getSegments();
	
	// Detect any modifications in segmentation
	bool preserve_segments = (n_segments == n_sentences);
	{	
		// extract segment boundary offsets
		std::vector< EDTOffset > seg_offsets;
		MTDocumentReader().constructLocatedStringsFromField( segments, ref_source, NULL, & seg_offsets );
		
		// ensure tokens of corresponding sentences fall within original
		//  segment offsets-- violation indicates a segmentation change
		for( size_t i = 0; preserve_segments && i != (size_t)n_segments; i++ ){
			TokenSequence * tseq = _docTheory->getSentenceTheory((int)i)->getTokenSequence();

			// Check for something that shouldn't happen
			if (tseq == 0) {
				throw UnrecoverableException("MTResultCollector::produceOutput", "TokenSequence is null.  Have you run the tokens stage?");
			}

			if( tseq->getNTokens() == 0 ) continue;
			
			if( tseq->getToken(0)->getStartEDTOffset() < seg_offsets[i] )
				preserve_segments = false;
			if( tseq->getToken( tseq->getNTokens() - 1 )->getEndEDTOffset() > seg_offsets[i+1] )
				preserve_segments = false;
		}
	}
	
	if( preserve_segments ){
		
		//wcerr << "MTResultCollector: No re-segmentation occured, extending ";
		//wcerr << "input document Segments for document" << document->getName().to_string() << endl;
		
	} else {
		
		//wcerr << "MTResultCollector: Creating default sentence-break ";
		//wcerr << "segments for document " << document->getName().to_string() << endl;
		
		segments.clear(); segments.resize( n_sentences );
		
		for( int i = 0; i < n_sentences; i++ ){
			
			// segment header info
			segments[i].segment_attributes()[L"doc-id"]        = document->getName().to_string();
			segments[i].segment_attributes()[L"sentence-no"]   = lexical_cast<std::wstring>(i);
			
            if (document->getDateTimeField()) {
				segments[i].segment_attributes()[L"date-time"] = document->getDateTimeField()->toString();
            }
            if (document->getSourceType() != Symbol(L"") && document->getSourceType() != Symbol(L"UNKNOWN")) {
                segments[i].segment_attributes()[L"genre"] = document->getSourceType().to_string();
            }
			const LocatedString * sent_lstr = _docTheory->getSentence(i)->getString();
			
			// write sentence text offsets into the original source
			segments[i].segment_attributes()[L"start-offset"] = lexical_cast<std::wstring>( sent_lstr->firstStartOffsetStartingAt<EDTOffset>(0) );
			segments[i].segment_attributes()[L"end-offset"]   = lexical_cast<std::wstring>( sent_lstr->lastEndOffsetEndingAt<EDTOffset>(sent_lstr->length()-1) );
			
			// if our midpoint offset is covered by any metadata spans, list those as attributes of the segment
			EDTOffset middle((sent_lstr->originalStart<EDTOffset>().value() + sent_lstr->originalEnd<EDTOffset>().value()) / 2);
			std::auto_ptr<Metadata::SpanList> spanlist( document->getMetadata()->getCoveringSpans(middle) );
			
			bool metadata_found = false;
			for( int m = 0, mm = 0; m < spanlist->length(); m++ ){
				Span * sp = (*spanlist)[m];
				if(sp->getSpanType() == Symbol(L"TEXT") || sp->getSpanType() == Symbol(L"SEG")) { continue; }

				if (metadata_found) 
					SessionLogger::warn("mt_result_collector") << L"Discarding segment metadata: " << segments[i].segment_attributes()[L"meta"].c_str() << L"\n";

				segments[i].segment_attributes()[L"meta"] = sp->getSpanType().to_string();
				metadata_found = true;
			}
			
			// generate a unique sentence GUID for the MT folks
			std::wstringstream sent_guid;
			sent_guid << "[" << document->getName().to_string() << "][" << i << "]";
			segments[i].add_field( L"guid", sent_guid.str() );
			
			// and add the sentence content
			segments[i].add_field( ref_source, sent_lstr->toString() );
		}
	}
	
	std::wstring doc_out_fname = std::wstring(output_dir) + LSERIF_PATH_SEP + std::wstring(document_filename) + L".segments";
	
	SessionLogger::info("SERIF") << "Writing to segment-format file " << doc_out_fname.c_str() << std::endl;
	UTF8OutputStream dout( doc_out_fname.c_str() );
	if( dout.fail() ) {
		std::string doc_out_fname_as_string = std::string(doc_out_fname.begin(), doc_out_fname.end());	
		throw UnrecoverableException( "MTResultCollector::produceOutput",
			(std::string("Unable to open ") + doc_out_fname_as_string + " for reading.").c_str() );
	}

	// expand segments with Serif structures, and write to the doc stream
	for( int cur_sent_no = 0; cur_sent_no < n_sentences; cur_sent_no++ ){
		
		const Sentence * sent        = _docTheory->getSentence(cur_sent_no);
		SentenceTheory * sent_theory = _docTheory->getSentenceTheory(cur_sent_no);
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
			fe.attributes[L"refers"] = ref_source;
			segments[cur_sent_no][L"names"].push_back( fe );
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
			fe.attributes[L"refers"] = ref_source;
			segments[cur_sent_no][L"values"].push_back( fe );
		}
		
		// Get the full parse
		Parse * fullParse = sent_theory->getFullParse();
		if (fullParse && fullParse->getRoot()->getChild(0)->getTag() != Symbol(L"-empty-")) {
			
			std::wstringstream parse_val;
			std::wstringstream heads_val;
			
			int numNodes = 0, heads[4096], nodeStarts[4096], nodeEnds[4096]; Symbol tags[4096];
			fullParse->getRoot()->getAllNonterminalNodesWithHeads( heads, nodeStarts, nodeEnds, tags, numNodes, 4096 );
			
			for( int i = 0; i < numNodes; i++ ){
                int start = sent_lstr->positionOfStartOffset(sent_tokens->getToken(nodeStarts[i])->getStartEDTOffset());
                int end   = sent_lstr->positionOfEndOffset(sent_tokens->getToken(nodeEnds[i])->getEndEDTOffset());
				parse_val << tags[i].to_string() << ":" << start << ":" << end << " ";
				heads_val << heads[i] << " ";
			}
			
			// add a parse field to the segment
			WSegment::field_entry_t fe;
			fe.value = parse_val.str();
			fe.attributes[L"refers"] = ref_source;
			segments[cur_sent_no][L"parse"].push_back( fe );

			if (ParamReader::isParamTrue("mt_result_parse_output_heads")){
				// add a heads field to the segment
				WSegment::field_entry_t fe;
				fe.value = heads_val.str();
				fe.attributes[L"refers"] = ref_source;
				segments[cur_sent_no][L"heads"].push_back( fe );
			}
		}
		
		// Get the np chunks parse
		Parse * npChunkParse = sent_theory->getNPChunkParse();
		if (npChunkParse) {
			
			std::wstringstream npchunk_val;
			
			int numNodes = 0, nodeStarts[4096], nodeEnds[4096]; Symbol tags[4096];
			npChunkParse->getRoot()->getAllNonterminalNodes( nodeStarts, nodeEnds, tags, numNodes, 4096);
			
			for (int i = 0; i < numNodes; i++) {
				int start = sent_lstr->positionOfStartOffset(sent_tokens->getToken(nodeStarts[i])->getStartEDTOffset());
				int end   = sent_lstr->positionOfEndOffset(sent_tokens->getToken(nodeEnds[i])->getEndEDTOffset());
                npchunk_val << tags[i].to_string() << ":" << start << ":" << end << " ";
			}
			
			// add a parse field to the segment
			WSegment::field_entry_t fe;
			fe.value = npchunk_val.str();
			fe.attributes[L"refers"] = ref_source;
			segments[cur_sent_no][L"np-chunks"].push_back( fe );
		}
		
		// Get the mentions (descriptors)
		MentionSet* mentionSet = sent_theory->getMentionSet();
		if (mentionSet) {
			
			std::wstringstream ment_val;
			
			int nMentions = mentionSet->getNMentions();
			for (int i = 0; i < nMentions; i++) {
				Mention* mention = mentionSet->getMention(i);
				
				// build up the mention string representation
				ment_val << mention->getTypeString(mention->getMentionType()) << "/" << mention->getEntityType().getName().to_string();
				
				if( mention->hasIntendedType() )
					ment_val << "." << mention->getIntendedType().getName().to_string();
				if( mention->hasRoleType() )
					ment_val << "." << mention->getRoleType().getName().to_string();
				
				ment_val << "/" << mention->getEntitySubtype().getName().to_string();
				ment_val << ":" << sent_lstr->positionOfStartOffset(sent_tokens->getToken(mention->getNode()->getStartToken())->getStartEDTOffset());
				ment_val << ":" << sent_lstr->positionOfEndOffset(sent_tokens->getToken(mention->getNode()->getEndToken())->getEndEDTOffset()) << " ";
			}
			
			// add a mentions field to the segment
			WSegment::field_entry_t fe;
			fe.value = ment_val.str();
			fe.attributes[L"refers"] = ref_source;
			segments[cur_sent_no][L"mentions"].push_back( fe );
		}
		// Get the tokens for this sentence
		
		if (sent_tokens && write_tokens) {
			
			std::wstringstream toks_val;
			
			int nSentTokens = sent_tokens->getNTokens();
			for (int i = 0; i < nSentTokens; i++) {
				const Token* token = sent_tokens->getToken(i);
				
				// build minimal token string representation
				
				toks_val << token->getStartEDTOffset() << " ";
				toks_val << token->getEndEDTOffset() << " ";
				toks_val << token->getSymbol().to_string() << "  ";
			}
			
			// add a tokens field to the segment
			WSegment::field_entry_t fet;
			fet.value = toks_val.str();
			fet.attributes[L"refers"] = ref_source;
			segments[cur_sent_no][L"tokens"].push_back( fet );
		}		
		// write to the output stream!
		dout << segments[cur_sent_no];
	}
}



/*
	Deprecated hack checking for strange token offsets (this belongs closer to the tokenization stage should it be required again).
		
        int previousStart = -1;
        int previousEnd = -1;
        for (int i = 0; i < sent_tokens->getNTokens(); i++) {
//            printf("%d %s %d %d\n", i, sent_tokens->getToken(i)->getSymbol().to_debug_string(), 
//                sent_tokens->getToken(i)->getStartEDTOffset(), sent_tokens->getToken(i)->getEndEDTOffset());
            int start = sent_tokens->getToken(i)->getStartEDTOffset();
            int end = sent_tokens->getToken(i)->getEndEDTOffset();
            if (start == previousStart && end == previousEnd) {  // Look for signs that our tokenization has gone crazy
                wstringstream wss;
                UTF8OutputStream os((doc_out_fname.str() + string(".bad.txt")).c_str());
                previousStart = -1;
                previousEnd = -1;
                bool foundBad = false;
                for (int j = 0; j < sent_tokens->getNTokens(); j++) {
                    start = sent_tokens->getToken(j)->getStartEDTOffset();
                    end = sent_tokens->getToken(j)->getEndEDTOffset();
                    wss << j << " " << sent_tokens->getToken(j)->getSymbol().to_debug_string() << " " << start << " " << end << "\n";
                    if (!foundBad && previousEnd == end) {
                        os << L" (BAD) " << sent_tokens->getToken(j)->getSymbol().to_string() << " ";
                        foundBad = true;
                    } else {
                        os << sent_tokens->getToken(j)->getSymbol().to_string() << " ";
                    }
                    previousStart = start;
                    previousEnd = end;
                }
                os << L"\n";
                os.close();
                wstring ws = wss.str();
                string s(ws.begin(), ws.end());
                char message[4096];
                sprintf(message, "Document %s sentence %d with %d tokens has suspicious offsets:\n%s\n", document->getName().to_debug_string(), cur_sent_no, sent_tokens->getNTokens(), s.c_str());
                throw UnrecoverableException( "MTResultCollector::produceOutput", message);
            }
            previousStart = start;
            previousEnd = end;
        }

*/
