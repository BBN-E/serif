# Takes a directory of serifxml run through the tokens stage and
# a spannotator directory of annotation on those files and augments
# the serifxml with annotated names. This augmented serifxml should
# be suitable for JSerif name model training. 

# Sample params: /nfs/raid66/u14/psychic/data/name_annotation/serif/tokens/output /nfs/raid63/u11/users/azamania/PSYCHIC/spannotator_databases/initial_vulnerability_db/tasks/initial_practice_az/annotation /nfs/raid63/u11/users/azamania/PSYCHIC/spannotator_databases/initial_vulnerability_db/tasks/initial_practice_az/augmented_serifxml

import sys, os, serifxml

def add_name_theories(serif_doc):
    for sentence in serif_doc.sentences:
        sentence.name_theory = serifxml.NameTheory(owner=sentence)
        sentence.name_theory.token_sequence = sentence.token_sequence

def find_name_theory(serif_doc, start, end):
    for sentence in serif_doc.sentences:
        name_theory = sentence.name_theory
        if start >= name_theory.token_sequence[0].start_char and start < name_theory.token_sequence[-1].end_char:
            return name_theory
    return None

def find_tokens(name_theory, start, end):
    start_token = None
    end_token = None
    for token in name_theory.token_sequence:
        #print str(token.start_char) + " " + str(token.end_char)
        if start >= token.start_char and start <= token.end_char:
            start_token = token
        if end >= token.start_char and end <= token.end_char + 1: # +1 because we might have annotated the space after the token
            end_token = token

    return start_token, end_token
        
if len(sys.argv) != 4:
    print "Usage: " + sys.argv[0] + " serifxml-dir spannotator-dir output-dir"
    sys.exit(1)

serifxml_input, spannotator_input, output = sys.argv[1:]

if not os.path.isdir(output):
    os.makedirs(output)


serifxml_filenames = os.listdir(serifxml_input)
spannotator_filenames = os.listdir(spannotator_input)

intersection_filenames = [fn for fn in serifxml_filenames if fn in spannotator_filenames]

for filename in intersection_filenames:
    print filename
    serifxml_file = os.path.join(serifxml_input, filename)
    spannotator_file = os.path.join(spannotator_input, filename)
    output_file = os.path.join(output, filename)
    
    serif_doc = serifxml.Document(serifxml_file)
    add_name_theories(serif_doc)

    sf = open(spannotator_file)
    for line in sf:
        line = line.strip()
        pieces = line.split('\t')
        if len(pieces) != 5:
            print "Bad line: " + line
            sys.exit(1)
        span_type, start, end, timestamp, annotator = pieces
        start = int(start)
        end = int(end)
        name_theory = find_name_theory(serif_doc, start, end)
        if name_theory is None:
            print "Warning: Could not find name_theory for: " + str(start) + " " + str(end)
            continue
        
        start_token, end_token = find_tokens(name_theory, start, end)
        if start_token is None or end_token is None:
            print "Error: Could not find tokens for: " + str(start) + " " + str(end)
            sys.exit(1)
            
        name = serifxml.Name(owner=name_theory)
        name.entity_type = span_type
        name.start_token = start_token
        name.end_token = end_token
        name_theory._children.append(name)
        
    sf.close()
    serif_doc.save(output_file)
