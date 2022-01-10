import sys
import os
import codecs
import serifxml

def node_pos_formatter(node):
    """
    Returns a terminal node's original token.
    """

    return node.text

def node_offsets_formatter(node):
    """
    Returns a terminal node's original character offsets.
    """

    return "%d:%d" % (node.start_token.start_char, node.end_token.end_char)

def get_node_string(node_formatter, node):
    """
    Returns a string for the specified node and its children, converted
    with the specified formatter. Based on
    serifxml.SerifSynNodeTheory._treebank_str.
    """

    # Terminal nodes just get formatted
    if len(node) == 0:
        return node_formatter(node)

    # Non-terminal nodes get their tag and their formatted children
    node_string = "(%s" % node.tag
    node_string += "".join([" %s" % get_node_string(node_formatter, child) for child in node])
    node_string += ")"
    return node_string

def print_node(output_f, node_formatter, node, indent = '', width = 75):
    """
    Prints the specified node and its children to the specified open
    file using the specified formatter, with indentation. Based on
    serifxml.SerifSynNodeTheory._pprint_treebank_str.
    """

    # Try putting this tree on one line.
    node_string = get_node_string(node_formatter, node)
    if len(node_string) + len(indent) < width:
        output_f.write("%s%s" % (indent, node_string))
        return

    # Otherwise, put each child on a separate line.
    output_f.write("%s(%s" % (indent, node.tag))
    for child in node:
        output_f.write("\n")
        print_node(output_f, node_formatter, child, indent + '  ', width)
    output_f.write(")")

def extract_parse_trees(mode, input_file, output_dir):
    """
    Extracts the parse trees for the specified SerifXML file in the
    specified format, 'parse' or 'parseoffsets' (character
    offsets into original text).
    """

    # Determine which printer we're going to use
    node_formatter = None
    if mode == "parse":
        node_formatter = node_pos_formatter
    elif mode == "parseoffsets":
        node_formatter = node_offsets_formatter
    else:
        raise Exception("Unknown mode '%s'" % mode)

    # Set up the input and output
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
    try:
        input_doc = serifxml.Document(input_file)
    except Exception as e:
        # Malformed document, probably due to an illegal character
        print "Could not read %s: %s" % (os.path.basename(input_file), e)
        return
    output_file = os.path.join(output_dir, os.path.basename(input_file).replace("xml", mode))
    output_f = codecs.open(output_file, "w", "utf-8")

    # Dump the parse of each sentence
    for sentence in input_doc.sentences:
        if sentence.parse.root is None:
            output_f.write("(X^ -empty-)")
        else:
            print_node(output_f, node_formatter, sentence.parse.root)
        output_f.write("\n")

def extract_parse_trees_dir(mode, input_dir, output_dir):
    """
    Extracts the parse trees for the SerifXML files in the specified
    directory.
    """

    # Handle files or directories
    if os.path.isdir(input_dir):
        # Loop over the directory, checking for SerifXML
        for filename in os.listdir(input_dir):
            if filename.endswith(".xml"):
                input_file = os.path.join(input_dir, filename)
                extract_parse_trees(mode, input_file, output_dir)
    else:
        extract_parse_trees(mode, input_dir, output_dir)

def main():
    """
    Checks arguments and passes them to extract_parse_trees.
    """

    # Check command-line arguments
    usage = "usage: %s <parse|parseoffsets> <serifxml file|dir> <output dir>" % sys.argv[0]
    if len(sys.argv) != 4:
        sys.stderr.write("%s\n" % usage)
        sys.exit(2)
    extract_parse_trees_dir(*sys.argv[1:])

if __name__ == "__main__":
    main()
