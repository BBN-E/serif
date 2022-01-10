#!/usr/bin/env python
# Copyright 2015 by BBN Technologies Corp.
# All Rights Reserved.

import os
import re
import codecs
import argparse
import serifxml

def text_to_serifxml(text, docid='anonymous', language='English'):
    """
    Takes an arbitrary string and constructs an input SerifXML document.
    """

    # Construct a SerifXML document from the text
    document = serifxml.Document()
    document.docid = docid
    document.language = language
    document.original_text = serifxml.OriginalText(owner=document,
                                                   contents=text)

    # Done
    return document

def path_to_docid(input_path):
    """
    Generates a document ID based on a file path.
    """

    filename = os.path.basename(input_path)
    docid = filename.decode('ascii', 'ignore')
    docid = re.sub(r'\s+', '_', docid, flags=re.UNICODE)
    return docid

def path_to_serifxml(input_path, output_dir, language='English'):
    """
    Converts text files to SerifXML in the specified directory
    """

    # Handle files or directories
    if os.path.isdir(input_path):
        # Loop over the directory recursively
        for filename in os.listdir(input_path):
            sub_path = os.path.join(input_path, filename)
            path_to_serifxml(sub_path, output_dir, language)
    else:
        # Load the text and generate a document ID
        text = codecs.open(input_path, 'r', 'utf-8').read()
        docid = path_to_docid(input_path)

        # Convert to SerifXML
        xml = text_to_serifxml(text, docid, language)

        # Write SerifXML
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        output_path = os.path.join(output_dir, "%s.xml" % docid)
        xml.save(output_path)

def main():
    """
    """

    # Check command-line arguments
    parser = argparse.ArgumentParser(description="Converts plain text documents to SerifXML input")
    parser.add_argument("input", help="an input text file or a directory containing them")
    parser.add_argument("output", help="an output directory that will contain SerifXML")
    parser.add_argument("--language", help="the natural language of the input text", default="English")
    args = parser.parse_args()

    # Run extraction
    path_to_serifxml(args.input, args.output, args.language)

if __name__ == '__main__':
    main()
