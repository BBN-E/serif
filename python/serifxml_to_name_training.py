import os, sys, pdb, codecs

sys.path.append('t:/Projects/SERIF/python')
import serifxml


indir = sys.argv[1]
OUT = codecs.open(sys.argv[2], 'w', 'utf-8')

for filename in os.listdir(indir):
    try:
        doc = serifxml.Document(os.path.join(indir, filename))
    except:
        print "Not a serifxml document:", filename
    for s in doc.sentences:
        toks = []
        tok_map = {}
        for i,t in enumerate(s.token_sequence):
            toks.append( [t.text, None] )
            tok_map[t] = i
        for n in s.name_theory:
            st = tok_map[n.start_token]
            et = tok_map[n.end_token]
            toks[st][1] = "%s-ST" % n.entity_type
            for i in range(st+1, et+1):
                toks[i][1] = "%s-CO" % n.entity_type
        last_tag = "START"
        OUT.write("(")
        for x in toks:
            this_tag = x[1]
            if this_tag == None:
                if last_tag.find("NONE") == 0:
                    this_tag = "NONE-CO"
                else:
                    this_tag = "NONE-ST"
            OUT.write("(%s %s) " % (x[0], this_tag))
            last_tag = this_tag
        OUT.write(")\n")

OUT.close()
