
import sys, subprocess, os, re, tempfile, shutil

# Input values (from runjobs)
train_vector_file = r'+train_vector_file+'
test_vector_file = r'+test_vector_file+'
output_file = r'+output_file+'
megam = r'+megam+'

DEBUG = False

###########################################################################
## Convert SERIF's vector-file format to MEGAM's input file format
###########################################################################
    
def convert_vector_to_megam(vector_file, out, tag_map, feature_map):
    """
    Writes output to `out`.  Adds any new tags to the dictionary
    `tag_map`, which maps from tag names (eg NONE, REL) to integer
    identifiers used by megam (0, 1, 2, ...).
    """
    TAG_RE = re.compile('^(\w+) ')
    def tag_repl(m):
        tag = m.group(1)
        tag_num = tag_map.setdefault(tag, len(tag_map))
        return '%s ' % tag_num

    FEATURE_RE = re.compile(r'\(([^\(\)]+)\)')
    def feature_repl(m):
        feature = m.group(1)
        feature_num = feature_map.setdefault(feature, len(feature_map))
        return 'f%s' % feature_num
        
    for lineno, line in enumerate(open(vector_file)):
        line = line.strip()
        if not line: continue
        # Strip enclosing parens.
        if not (line[0] == '(' and line[-1] == ')'):
            print 'WARNING: SKIPPING BAD LINE %d in %s!\n  %r...' % (
                lineno, vector_file, line[:20])
            continue
        line = line[1:-1]
        # Write the tag & features in a format that megam likes.
        line = TAG_RE.sub(tag_repl, line)
        line = FEATURE_RE.sub(feature_repl, line)
        # Output the transformed line.
        out.write(line+'\n')
        # And some debug blather to make sure things look reasonable.
        short_filename = os.path.split(vector_file)[1]
        if lineno == 0: print '\n'+(' %s '%short_filename).center(75, '-')
        if lineno < 3: print line
        if lineno == 3: print '...'
    print '-'*75

###########################################################################
## Run Megam.
###########################################################################
    
def run_megam(megam_input, output_file, model_type='multiclass',
              repeat=20, feature_file=None, gaussian_prior_lambda=0):
    # Use -nobias since we supply our own bias feature (called 'prior')
    options = ['-nobias']
    options += ['-maxi', str(500)]
    options += ['-lambda', str(gaussian_prior_lambda)]
    options += ['-repeat', str(repeat or 1)]
    if feature_file:
        options += ['-sforcef', feature_file]
    command = [megam] + options + [model_type, megam_input]
    out = open(output_file, 'wb')
    subprocess.check_call(command, stdout=out)
    out.close()

###########################################################################
## Main Script
###########################################################################
    
tempdir = tempfile.mkdtemp()
try:
    megam_input = os.path.join(tempdir, 'megam.txt')
    tag_map = {}
    feature_map = {}
    
    print "Converting vector file to megam's input format..."
    print "  -> %s" % megam_input
    sys.stdout.flush()
    out = open(megam_input, 'wb')
    convert_vector_to_megam(train_vector_file, out, tag_map, feature_map)
    if test_vector_file:
        out.write('TEST\n') # should this be DEV instead??
        convert_vector_to_megam(test_vector_file, out, tag_map, feature_map)
    out.close()

#     feature_file = os.path.join(tempdir, 'features.txt')
#     out = open(feature_file, 'wb')
#     for feature in feature_map:
#         print >>out, feature
#     out.close()
    
    print "Running megam..."
    sys.stdout.flush()
    if len(tag_map) == 2: model_type = 'binary'
    else: model_type = 'multiclass'
    run_megam(megam_input, output_file, model_type,
              #feature_file=feature_file
              )
    
    print "Done!"
    print "Tag map: %s" % tag_map
    print "Num features: %s" % feature_map
    print "Output: %s" % output_file
    sys.stdout.flush()

    # To do: at this point, it would be nice to convert the vector
    # file to a SERIF model file.
finally:
    if DEBUG: print 'Not deleting temp dir: %s' % tempdir
    else: shutil.rmtree(tempdir)
    

