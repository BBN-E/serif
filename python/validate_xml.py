import sys
import os
import codecs
from lxml import etree

def validate(dtd, input, suffix = None):
    """
    Extracts the parse trees for the SerifXML files in the specified
    directory.
    """

    # Load the DTD if it isn't already
    if isinstance(dtd, str):
        loaded_dtd = etree.DTD(codecs.open(dtd, "r", "utf-8"))
    else:
        loaded_dtd = dtd

    # Handle files or directories
    if os.path.isdir(input):
        # Loop over the directory, optionally
        for filename in os.listdir(input):
            if suffix is None or filename.endswith(suffix):
                input_file = os.path.join(input, filename)
                validate(loaded_dtd, input_file)
    else:
        # Read and validate the input
        print os.path.basename(input)
        try:
            root = etree.parse(input)
        except Exception as e:
            print "  parse failed: %s" % e
            return
        valid = loaded_dtd.validate(root)
        if valid:
            print "  valid"
        else:
            print "  invalid"
            for error in loaded_dtd.error_log.filter_from_errors():
                print "  %s" % error

def main():
    """
    Checks arguments and passes them to validate.
    """

    # Check command-line arguments
    usage = "usage: %s <dtd file> <xml file|dir> [<xml file suffix>]" % sys.argv[0]
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        sys.stderr.write("%s\n" % usage)
        sys.exit(2)
    validate(*sys.argv[1:])

if __name__ == "__main__":
    main()
