# Copyright 2011 by BBN Technologies Corp.
# All Rights Reserved.

from xml.dom import minidom
import codecs

class SerifXMLDocument:
    """
    A class for representing SerifXML documents and performing basic
    operations on them (e.g. inserting a new NameTheory)
    """

    ENTITY_TYPES = set(["PER",
                        "ORG",
                        "GPE",
                        "FAC",
                        "LOC",
                        "VEH",
                        "WEA"])

    VALUE_TYPES = set(["Numeric.Percent",
                       "PERCENT"
                       "Numeric.Money",
                       "MONEY",
                       "Contact-Info.Phone-Number",
                       "PHONE",
                       "Contact-Info.E-Mail",
                       "EMAIL",
                       "Contact-Info.URL",
                       "URL",
                       "TIMEX2",
                       "TIMEX2.TIME",
                       "TIME",
                       "TIMEX2.DATE",
                       "DATE",
                       "Sentence",
                       "Crime",
                       "Job-Title"])

    def __init__(self, docstr):
        self.dom = minidom.parseString(docstr)
        self.next_id = 0

    def toString(self):
        xml = self.dom.getElementsByTagName('Document')[0].toxml()
        enc_str, enc_len = codecs.getencoder('utf-8')(xml)
        return enc_str

    def getNextId(self):
        id = 'p%d' % self.next_id
        self.next_id += 1
        return id

    def createNewlineElem(self):
        return self.dom.createTextNode("\n")

    def createNameTheoryElem(self, tokensElem):
        ntElem = self.dom.createElement("NameTheory")
        ntElem.setAttribute("id", self.getNextId())
        ntElem.setAttribute("score", "0")
        ntElem.setAttribute("token_sequence_id",
                            tokensElem.getAttribute("id"))
        return ntElem

    def createValueMentionSetElem(self, tokensElem):
        vmsElem = self.dom.createElement("ValueMentionSet")
        vmsElem.setAttribute("id", self.getNextId())
        vmsElem.setAttribute("score", "0")
        vmsElem.setAttribute("token_sequence_id",
                            tokensElem.getAttribute("id"))
        return vmsElem

    def createNameElem(self, entity):
        type,start,end = entity
        offset_str = "%d:%d" % (start, end)
        nameElem = self.dom.createElement("Name")
        nameElem.setAttribute("id", self.getNextId())
        nameElem.setAttribute("score", "0")
        nameElem.setAttribute("entity_type", type)
        nameElem.setAttribute("char_offsets", offset_str)
        return nameElem

    def createValueMentionElem(self, entity):
        type,start,end = entity
        offset_str = "%d:%d" % (start, end)
        valueElem = self.dom.createElement("ValueMention")
        valueElem.setAttribute("id", self.getNextId())
        valueElem.setAttribute("score", "0")
        valueElem.setAttribute("value_type", type)
        valueElem.setAttribute("char_offsets", offset_str)
        return valueElem

    def getTokenForOffset(self, tokens, offset):
        for token in tokens.getElementsByTagName('Token'):
            offsets = token.getAttribute("char_offsets")
            start,end = offsets.split(":")
            if offset >= int(start) and offset <= int(end):
                return token
        return None

    def isWithinSentence(self, sentElem, entity):
        """
        Returns True if entity offsets are within the boundaries
        defined by sentElem's char_offsets attribute. False otherwise.
        sentElem is an XML element representing a Serif sentence.
        entity is a (type, start_offset, end_offset) tuple.
        """
        offsets = sentElem.getAttribute("char_offsets")
        start,end = offsets.split(":")
        if entity[1] >= int(start) and entity[2] <= int(end):
            return True
        return False

    def setTokenAttributes(self, elem, tokSeqElem, entity):
        """
        Returns True if tokens were sucessfully set. False otherwise.
        tokSeqElem is an XML element representing a Serif token sequence.
        entity is a (type, start_offset, end_offset) tuple.
        """
        start_tok = self.getTokenForOffset(tokSeqElem, entity[1])
        end_tok = self.getTokenForOffset(tokSeqElem, entity[2])
        # Handling of token mismatch should happen here
        if start_tok is None or end_tok is None:
            return False
        elem.setAttribute("start_token", start_tok.getAttribute("id"))
        elem.setAttribute("end_token", end_tok.getAttribute("id"))
        return True

    def createNameTheoryFromEntityList(self, sentence, entities):
        tokenSeqs = sentence.getElementsByTagName('TokenSequence')
        for tokSeqElem in tokenSeqs:
            # Create a NameTheory element
            ntElem = self.createNameTheoryElem(tokSeqElem)
            for entity in entities:
                if entity[0] not in SerifXMLDocument.ENTITY_TYPES:
                    continue
                if self.isWithinSentence(sentence, entity):
                    nameElem = self.createNameElem(entity)
                    if self.setTokenAttributes(nameElem, tokSeqElem, entity):
                        ntElem.appendChild(nameElem)
            self.insertNameTheory(sentence, tokSeqElem, ntElem)

    def createValueMentionSetFromEntityList(self, sentence, entities):
        tokenSeqs = sentence.getElementsByTagName('TokenSequence')
        for tokSeqElem in tokenSeqs:
            # Create a ValueSet element
            vmsElem = self.createValueMentionSetElem(tokSeqElem)
            for entity in entities:
                if entity[0] not in SerifXMLDocument.VALUE_TYPES:
                    continue
                if self.isWithinSentence(sentence, entity):
                    valueMentionElem = self.createValueMentionElem(entity)
                    if self.setTokenAttributes(valueMentionElem,
                                           tokSeqElem, entity):
                        vmsElem.appendChild(valueMentionElem)
            self.insertValueMentionSet(sentence, tokSeqElem, vmsElem)

    def insertNameTheory(self, sentence, tokenSequence, name_theory_elem):
        tokens_id = tokenSequence.getAttribute("id")
        sentTheoryElems = sentence.getElementsByTagName('SentenceTheory')
        for stElem in sentTheoryElems:
            if stElem.getAttribute("token_sequence_id") == tokens_id:
                # Add the NameTheory to the list of SentenceTheory attributes
                stElem.setAttribute("name_theory_id",
                                    name_theory_elem.getAttribute("id"))
                # Insert the NameTheory element into the sentence element
                sentence.insertBefore(name_theory_elem, stElem)

    def insertValueMentionSet(self, sentence, tokenSequence, value_mention_set_elem):
        tokens_id = tokenSequence.getAttribute("id")
        sentTheoryElems = sentence.getElementsByTagName('SentenceTheory')
        for stElem in sentTheoryElems:
            if stElem.getAttribute("token_sequence_id") == tokens_id:
                # Add the ValueMentionSet to the list of SentenceTheory attributes
                stElem.setAttribute("value_mention_set_id",
                                    value_mention_set_elem.getAttribute("id"))
                # Insert the NameTheory element into the sentence element
                sentence.insertBefore(value_mention_set_elem, stElem)

    def insertNamesFromEntityList(self, entity_list):
        for sentence in self.dom.getElementsByTagName('Sentence'):
            self.createNameTheoryFromEntityList(sentence, entity_list)

    def insertValuesFromEntityList(self, entity_list):
        for sentence in self.dom.getElementsByTagName('Sentence'):
            self.createValueMentionSetFromEntityList(sentence, entity_list)

if __name__ == '__main__':

    import argparse
    import os.path
    from SerifHTTPClient import SerifHTTPClient

    parser = argparse.ArgumentParser(description="Test SerifXMLModifier module")
    parser.add_argument('home', metavar='SERIF_DATA',
                        help='SERIF data directory')
    parser.add_argument('--lang', metavar='language',
                        default='english',
                        choices = ('english', 'chinese', 'arabic'),
                        help='language to use for test')
    args = parser.parse_args()

    if args.lang == 'english':
        filename = os.path.join(args.home, 'english', 'test', 'sample-batch', 'source', 'AFP_ENG_20030304.0250.sgm')
        entity_list = [('GPE', 257, 261), ('TIMEX2', 277, 283)]
    elif args.lang == 'chinese':
        filename = os.path.join(args.home, 'chinese', 'test', 'sample-batch', 'source', 'CBS20001001.1000.0041.sgm')
        entity_list = [('GPE', 199, 200), ('TIMEX2', 204, 207)]
    elif args.lang == 'arabic':
        filename = os.path.join(args.home, 'arabic', 'test', 'sample-batch', 'source', 'AFA20001014.1400.0153.sgm')
        entity_list = [('GPE', 168, 169), ('PER', 192, 194)]

    client = SerifHTTPClient(args.lang, 'localhost', 8000)
    basename = os.path.basename(filename)

    print "Processing %s from SERIF stage 'START' to 'part-of-speech'" % basename
    result = client.process_file(filename, 'part-of-speech', 'serifxml')
    doc = SerifXMLDocument(result)

    print "Inserting external name and value list"
    doc.insertNamesFromEntityList(entity_list)
    doc.insertValuesFromEntityList(entity_list)

    print "Processing %s from SERIF stage 'parser' to 'output'" % basename
    result = client.process_xml(doc.toString(), 'parse', 'output', 'serifxml')
    doc = SerifXMLDocument(result)

    print doc.toString()
