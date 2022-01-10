#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include <stdexcept>
#include <sstream>
#include <sys/stat.h>
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/LocatedString.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/ValueMentionSet.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/tokens/Tokenizer.h"
#include "MainUtilities.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include "boost/algorithm/string/split.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

// Studio is over-paranoid about bounds checking for the boost string 
// classification module, so we wrap its import statement with pragmas
// that tell studio to not display warnings about it.  For more info:
// <http://msdn.microsoft.com/en-us/library/ttcz0bys.aspx>
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4996)
# include "boost/algorithm/string/classification.hpp"
# pragma warning(pop)
#else
# include "boost/algorithm/string/classification.hpp"
#endif

std::wstring MainUtilities::normalizeString(const std::wstring& input_string) {

	// Replace several non-ASCII characters with close approximants -- e.g., 
	// remove accents from letters.  This really only makes sense to do if
	// we're handling English text, so skip it otherwise.
#ifdef ENGLISH_LANGUAGE
	static Tokenizer* static_tokenizer = Tokenizer::build();
	LocatedString located_string(input_string.c_str());
	static_tokenizer->standardizeNonASCII(&located_string);
	std::wstring normalized_string = located_string.toWString();
#else
	std::wstring normalized_string = input_string;	
#endif

	std::transform(normalized_string.begin(), normalized_string.end(), normalized_string.begin(), ::tolower); // Lowercase
	size_t index = normalized_string.find(L',');
	while(index != std::wstring::npos) {  // Remove commas
		normalized_string.replace(index, 1, L"");
		index = normalized_string.find(L',');
	}
	index = normalized_string.find(L'.');
	while(index != std::wstring::npos) {  // Remove periods
		normalized_string.replace(index, 1, L"");
		index = normalized_string.find(L'.');
	}
	index = normalized_string.find(L'\t');
	while(index != std::wstring::npos) {
		normalized_string.replace(index, 1, L" ");  // Tabs to spaces
		index = normalized_string.find(L'\t');
	}
	index = normalized_string.find(L"  ");
	while (index != std::wstring::npos) { // Remove duplicate whitespace
		normalized_string.replace(index, 2, L" ");
		index = normalized_string.find(L"  ");
	}
	while(normalized_string.length() > 0 && normalized_string[0] == L' ') {  // Strip leading whitespace
		normalized_string = normalized_string.substr(1);
	}
	while(normalized_string.length() > 0 && normalized_string[normalized_string.length()-1] == L' ') {  // Strip trailing whitespace
		normalized_string = normalized_string.substr(0, normalized_string.length()-1);
	}
#ifdef ENGLISH_LANGUAGE
	if (normalized_string.find(L"a ") == 0) {
		normalized_string = normalized_string.substr(2);	
	} else if (normalized_string.find(L"an ") == 0) {
		normalized_string = normalized_string.substr(3);
	} else if (normalized_string.find(L"the ") == 0) {
		normalized_string = normalized_string.substr(4);
	}
#endif
	return normalized_string;
}

std::string MainUtilities::normalizeString(const std::string& input_string){
	std::wstring winput_string = UnicodeUtil::toUTF16StdString(input_string);
	std::wstring wnorm_string = normalizeString(winput_string);
	std::string norm_string = UnicodeUtil::toUTF8StdString(wnorm_string);
	return norm_string;
}
std::wstring MainUtilities::encodeForXML(std::wstring const &s) {
	std::wostringstream result;

	for( std::wstring::const_iterator iter = s.begin(); iter!=s.end(); iter++ )
    {
		wchar_t c = *iter;

		switch( c )
		{
			case '&': result << "&amp;"; break;
			case '<': result << "&lt;"; break;
			case '>': result << "&gt;"; break;
			case '"': result << "&quot;"; break;
			case '\'': result << "&apos;"; break;
			case '\n': result << '\n'; break;
			default:
				if ( c<32 || c>127 )
					result << "&#" << (unsigned int)c << ";";
				else
					result << c;
         }
    }

    return result.str();
}

bool MainUtilities::getSerifStartEndTokenFromCharacterOffsets(const DocTheory * docTheory, EDTOffset c_start, EDTOffset c_end, int& sent_num, int& tok_start, int& tok_end)
{
	sent_num = -1;
	tok_start = -1;
	tok_end = -1;
	for(int i =0; i< docTheory->getNSentences(); i++){
		const TokenSequence* toks = docTheory->getSentenceTheory(i)->getTokenSequence();
		EDTOffset sent_start = toks->getToken(0)->getStartEDTOffset();
		EDTOffset sent_end = toks->getToken(toks->getNTokens()-1)->getEndEDTOffset();
		if(c_start >= sent_start && c_end <= sent_end){
			//found the right sentence
			sent_num = i;
			for(int j = 0;  j< toks->getNTokens(); j++){
				//this is a loose alignment (doesn't require that you match the beginning/ending of a token exactly)
				const Token* t = toks->getToken(j);
				if(c_start >= t->getStartEDTOffset() && c_start <= t->getEndEDTOffset()){
					tok_start = j;
				}
				if(c_end >= t->getStartEDTOffset() && c_end <= t->getEndEDTOffset()){
					tok_end = j;
				}
			}
			if(tok_start == -1 || tok_end == -1){
				/*SessionLogger::warn("LEARNIT")<<"DistillUtilities::getSerifStartEndTokenFromCharacterOffsets(): Skipping alignment for: "
					<<docTheory->getDocument()->getName()<<"offsets ("<<c_start<<", "<<c_end
					<<") found sentence: "<<i<< " but could not align start/end token: "<<tok_start<<", "<<tok_end<<std::endl;
					*/
				return false;
			}
			else{
				return true;
			}
		}
		else if(c_start <= sent_start && c_end >= sent_end){
			/*SessionLogger::warn("LEARNIT")<<"DistillUtilities::getSerifStartEndTokenFromCharacterOffsets(): Skipping alignment for: "
				<<docTheory->getDocument()->getName()<<"offsets ("<<c_start<<", "<<c_end
				<<") cross boundary of sentence: "<<i<< "sentence offsets: "<<sent_start<<", "<<sent_end<<std::endl;
				*/
			return false;
		}
	}
	//SessionLogger::warn("LEARNIT")<<"DistillUtilities::getSerifStartEndTokenFromCharacterOffsets(): Couldn't find alignment for: "
	//			<<docTheory->getDocument()->getName()<<"offsets ("<<c_start<<", "<<c_end<<")"<<std::endl;
	return false;
}
const SynNode* MainUtilities::getParseNodeFromTokenOffsets(const SentenceTheory* sentTheory, int start_token, int end_token){
	const SynNode* root = sentTheory->getFullParse()->getRoot();
	const SynNode* best_node = root->getNodeByTokenSpan(start_token, end_token);
	return best_node;
}
const Mention* MainUtilities::getMentionFromTokenOffsets(const SentenceTheory* sentTheory, int start_token, int end_token){
	const MentionSet* ment_set = sentTheory->getMentionSet();
	const Mention* matched_mention = 0;
	const Mention* matched_head = 0;
	for(int i =0; i< ment_set->getNMentions(); i++){
		const Mention* m = ment_set->getMention(i);
		if((start_token == m->getNode()->getStartToken()) && (end_token == m->getNode()->getEndToken())){
			matched_mention = m;
			break;
		}
		else if((m->getHead() != 0) && 
			(start_token ==m->getHead()->getStartToken()) && (end_token ==m->getHead()->getEndToken())) 
		{
			matched_head = m;
		}
	}
	if(matched_mention == 0){
		matched_mention = matched_head;
	}
	if(matched_mention == 0){
		return 0;
	}
	if(matched_mention->getMentionType() != Mention::NONE || matched_mention->getHead() == 0){
		return matched_mention;
	}
	//we have a 'NONE' mention, see if there is a non-none mention that shares the same head
	for(int i =0; i< ment_set->getNMentions(); i++){
		const Mention* m = ment_set->getMention(i);
		if(m->getMentionType() != Mention::NONE && m->getHead() == matched_mention->getHead()){
			return m;
		}
	}
	return matched_mention;


}
const ValueMention* MainUtilities::getValueMentionFromTokenOffsets(const SentenceTheory* sentTheory, int start_token, int end_token){
	const ValueMentionSet* vms = sentTheory->getValueMentionSet();
	for(int i =0; i< vms->getNValueMentions(); i++){
		const ValueMention* vm = vms->getValueMention(i);
		if(vm->getStartToken() == start_token && vm->getEndToken() == end_token)
			return vm;
	}
	return 0;

}
LocatedString* MainUtilities::substringFromEdtOffsets(const LocatedString* origStr, const EDTOffset begin_edt_index, const EDTOffset end_edt_index) {
	return origStr->substring(origStr->positionOfStartOffset<EDTOffset>(begin_edt_index), origStr->positionOfStartOffset<EDTOffset>(end_edt_index) + 1);
}
