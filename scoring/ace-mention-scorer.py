from encodings import codecs
import xml.dom.minidom
import os.path
import sys

#############################################################################
##
## ACE MENTION SCORER
##
## This scorer aligns entity mentions with the same head offsets, or barring 
## an exact match, allows an alignment to a mention that begins or ends at the 
## same offset, if the entity type is the same. This latter allowance makes 
## the scorer more lenient than ACE but usually makes sense. For instance, it 
## allows the key name "U.S. army" to match "army", or "John Jones, Esq." to 
## match "John Jones", which is probably the correct thing. 
##
## The scorer reports "strict" entity mention scores, where entity types 
## must be the same to be counted as correct, and "loose" scores, where 
## entity type is ignored (although the non-exact head-offset match still 
## requires the types to match, to avoid cases like "Bank of Iran" matching 
## "Iran").
##
## Two relation mentions are considered aligned if their entity mentions 
## also align. Relation mentions in the test output are placed into one of 
## the following categories:
##    Exact: aligns to a key relation mention with the correct relation type
##    Type mismatch: aligns to a key relation mention with a 
##        different/reversed relation type
##    Spurious / entity match: does not align to a key relation mention, 
##        but a key relation mention can be found that shares the same two 
##        entities as its arguments (not entity mentions) and has the 
##        correct type. For instance, in a sentence "John married his 
##        wife", the key might only mark the relation mention 
##        Spouse(his, his wife). The relation mention Spouse(John, his wife) 
##        will not align to that relation mention when one looks at 
##        entity mentions only, but it will if one looks at the entities, 
##        since John and his are part of the same key entity. Note that 
##        this only looks at co-reference in the key.
##    Spurious / entity match / type mismatch: does not align to a key 
##        relation mention, but a key relation mention can be found that 
##        shares the same two entities as its arguments (not entity mentions) 
##        and has a different/reversed relation type
##    Spurious EM: one or more of the test relation mention arguments 
##        does not align to a key entity mention
##    Spurious: the test relation mention arguments align to key 
##        relation mention arguments, but there is no matching
##        relation in the key (at either the mention or entity level)
##
## Relations in the key output are placed into analogous categories: 
## exact, type mismatch, missing / entity match, missing, 
## missing / entity match / type mismatch, missing EM. 
## 
## The scorer presents four scores:
##    Strict RM: only exact matches are considered correct
##    Loose RM: exact matches and type mismatch matches are considered correct
##    Strict RM (entity fudge): exact and spurious  / entity match 
##        matches are considered correct
##    Loose RM (entity fudge): exact, type mismatch, spurious  / entity match, 
##        and spurious  / entity match / mismatch matches are considered 
##        correct
##
## This scorer is not heavily configured for automatic use. It could
## probably use some command-line parameters to turn on/off various
## printing. Feel free to add them. :)
##
## If you want to print additional interesting information about
## the actual alignments and missed/spurious mentions, it should
## be easy to do by iterating through em_mapped and rm_mapped. You probably 
## need to use codecs for non-English before you do so, though, 
## I haven't tried. 
##
## Liz Boschee, August 2011
##
## Copyright BBN Technologies Corp. 2011
## All rights reserved
############################################################################

class Mention:
    def __init__(self, filename, mention_id, etype, esubtype, mtype, start, end, text):
        self.filename = filename
        self.id = mention_id
        self.etype = etype
        self.esubtype = esubtype
        self.mtype = mtype
        self.start = start
        self.end = end
        self.text = text

    def matches_etype(self, etype):
        return etype == "ALL" or self.etype == etype

    def matches_mtype(self, mtype):
        return mtype == "ALL" or self.mtype == mtype

class MentionAlignment:
    EXACT = 0
    TYPE_MISMATCH = 1
    SAME_START = 2
    SAME_END = 3    
    SPURIOUS = 4
    MISSING = 5

class RelMention:
    def __init__(self, filename, mention_id, rtype, arg1, arg2):
        self.filename = filename
        self.id = mention_id
        self.rtype = rtype
        self.arg1 = arg1
        self.arg2 = arg2

    def matches_rtype(self, rtype):
        return rtype == "ALL" or self.rtype == rtype

    def is_symmetric(self):
        return self.rtype.startswith("PER-SOC") or self.rtype == "PHYS.Near"

class RelMentionAlignment:
    EXACT = 0
    TYPE_MISMATCH = 1
    SPURIOUS = 2
    SPURIOUS_EM = 3
    SPURIOUS_ENTITY_MATCH = 4
    SPURIOUS_ENTITY_TYPE_MISMATCH = 5
    MISSING = 6
    MISSING_EM = 7
    MISSING_ENTITY_MATCH = 8
    MISSING_ENTITY_TYPE_MISMATCH = 9

def is_spurious_rma(rma):
    return rma == RelMentionAlignment.SPURIOUS or rma == RelMentionAlignment.SPURIOUS_EM or rma == RelMentionAlignment.SPURIOUS_ENTITY_MATCH or rma == RelMentionAlignment.SPURIOUS_ENTITY_TYPE_MISMATCH

def is_missing_rma(rma):
    return rma == RelMentionAlignment.MISSING or rma == RelMentionAlignment.MISSING_EM or rma == RelMentionAlignment.MISSING_ENTITY_MATCH or rma == RelMentionAlignment.MISSING_ENTITY_TYPE_MISMATCH

class Mapping:
    # Either key or test can be None
    def __init__(self, key, test, alignment):
        self.key = key
        self.test = test
        self.alignment = alignment

def get_entities(filename, root, entities, entity_mentions, entity_map, local_em_list):
    for entity in root.getElementsByTagName("entity"):
        entity_id = entity.getAttribute("ID")
        entity_type = entity.getAttribute("TYPE")
        entity_subtype = entity.getAttribute("SUBTYPE")
        entities[entity_id] = []
        for mention in entity.getElementsByTagName("entity_mention"):
            mention_id = mention.getAttribute("ID")
            mention_type = mention.getAttribute("TYPE")
            head_charseq = mention.getElementsByTagName("head")[0].getElementsByTagName("charseq")[0]
            start = int(head_charseq.getAttribute("START"))
            end = int(head_charseq.getAttribute("END"))
            text = head_charseq.firstChild.data
            text = text.replace("\n", " ")
            entity_map[mention_id] = entity_id
            entities[entity_id].append(mention_id)
            entity_mentions[mention_id] = Mention(filename, mention_id, entity_type, entity_subtype, mention_type, start, end, text)
            local_em_list.append(entity_mentions[mention_id])

def get_relations(filename, root, relations, relation_mentions, relation_map, local_rm_list):
    for relation in root.getElementsByTagName("relation"):
        relation_id = relation.getAttribute("ID")
        relation_type = relation.getAttribute("TYPE") + "."+  relation.getAttribute("SUBTYPE")
        relations[relation_id] = []
        for mention in relation.getElementsByTagName("relation_mention"):
            mention_id = mention.getAttribute("ID")
            arg1 = None
            arg2 = None
            for elt in mention.getElementsByTagName("relation_mention_argument"):
                if elt.getAttribute("ROLE") == "Arg-1":
                    arg1 = elt.getAttribute("REFID")
                elif elt.getAttribute("ROLE") == "Arg-2":
                    arg2 = elt.getAttribute("REFID")
            relation_map[mention_id] = relation_id
            relations[relation_id].append(mention_id)
            relation_mentions[mention_id] = RelMention(filename, mention_id, relation_type, arg1, arg2)
            local_rm_list.append(relation_mentions[mention_id])


if len(sys.argv) != 3:
  print "Usage: key_dir test_dir\n";
  sys.exit()

keydir = sys.argv[1]
testdir = sys.argv[2]

em_mapped = []
rm_mapped = []

key_entities = {}
key_entity_mentions = {}
key_entity_map = {}
key_relations = {}
key_relation_mentions = {}
key_relation_map = {}

test_entities = {}
test_entity_mentions = {}
test_entity_map = {}
test_relations = {}
test_relation_mentions = {}
test_relation_map = {}

for subdir in os.listdir(keydir):
    
    print "Processing", subdir

    for filename in os.listdir(os.path.join(keydir, subdir, "key")):
        if not filename.endswith("xml"):
            continue
        f = filename[0:-8]
        
        local_key_ems = []
        local_test_ems = []
        local_key_rms = []
        local_test_rms = []

        # Read information from key
        key_root = xml.dom.minidom.parse(os.path.join(keydir, subdir, "key", f + ".apf.xml"))
        get_entities(f, key_root, key_entities, key_entity_mentions, key_entity_map, local_key_ems)
        get_relations(f, key_root, key_relations, key_relation_mentions, key_relation_map, local_key_rms)

        test_filename = os.path.join(testdir, subdir, "output", f + ".sgm.apf")
        if os.path.exists(test_filename):
            # Read information from test
            test_root = xml.dom.minidom.parse(test_filename)
            get_entities(f, test_root, test_entities, test_entity_mentions, test_entity_map, local_test_ems)
            get_relations(f, test_root, test_relations, test_relation_mentions, test_relation_map, local_test_rms)

        ##############################
        ### ENTITY MENTION ALIGNMENT
        ##############################

        # Easy storage of mention alignments from key<->test
        test_key_em_mapping = {}
        key_test_em_mapping = {}

        # First, align matches whose head offsets match exactly
        for test_em in local_test_ems:
            for key_em in local_key_ems:
                if test_em.start == key_em.start and test_em.end == key_em.end:
                    if test_em.etype == key_em.etype:
                        em_mapped.append(Mapping(key_em, test_em, MentionAlignment.EXACT))
                    else:
                        em_mapped.append(Mapping(key_em, test_em, MentionAlignment.TYPE_MISMATCH))
                    test_key_em_mapping[test_em.id] = key_em.id
                    key_test_em_mapping[key_em.id] = test_em.id
                    break

        ## Second, align matches that end at the same place and have the same entity type
        ## For instance, "U.S. army" ~ "army", but not "Bank of Iran" ~ "Iran"
        for test_em in local_test_ems:
            for key_em in local_key_ems:
                # Exclude key mentions that have already mapped to something
                if key_em.id in key_test_em_mapping:
                    continue
                if test_em.etype == key_em.etype and test_em.start >= key_em.start and test_em.end == key_em.end:
                    em_mapped.append(Mapping(key_em, test_em, MentionAlignment.SAME_END))
                    test_key_em_mapping[test_em.id] = key_em.id
                    key_test_em_mapping[key_em.id] = test_em.id
                    break

        ## Third, align matches that begin at the same place and have the same entity type
        ## For instance, "James Judd" ~ "James Judd, Esq." but not "American Airlines" ~ "American"
        for test_em in local_test_ems:
            for key_em in local_key_ems:
                # Exclude key mentions that have already mapped to something
                if key_em.id in key_test_em_mapping:
                    continue
                if test_em.etype == key_em.etype and test_em.start == key_em.start and test_em.end <= key_em.end:
                    em_mapped.append(Mapping(key_em, test_em, MentionAlignment.SAME_START))
                    test_key_em_mapping[test_em.id] = key_em.id
                    key_test_em_mapping[key_em.id] = test_em.id
                    break

        ## The remainder of test mentions are spurious
        for test_em in local_test_ems:
            if test_em.id not in test_key_em_mapping:
                em_mapped.append(Mapping(None, test_em, MentionAlignment.SPURIOUS))
                test_key_em_mapping[test_em.id] = None

        ## The remainder of key mentions are missing
        for key_em in local_key_ems:
            if key_em.id not in key_test_em_mapping:
                em_mapped.append(Mapping(key_em, None, MentionAlignment.MISSING))
                key_test_em_mapping[key_em.id] = None

        ################################
        ### RELATION MENTION ALIGNMENT
        ################################

        entity_mapped_key_rms = []
        entity_mapped_mismatch_key_rms = []
        for test_rm in local_test_rms:
            # Map the test mentions to the key mentions
            mapped_arg1 = test_key_em_mapping[test_rm.arg1]
            mapped_arg2 = test_key_em_mapping[test_rm.arg2]

            # If they can't be mapped, this is definitely spurious
            if not mapped_arg1 or not mapped_arg2:
                rm_mapped.append(Mapping(None, test_rm, RelMentionAlignment.SPURIOUS_EM))
                continue
            
            # First look for an exact match, i.e. a key relation mention with the same two arguments
            # It's called a "mismatch" if there is a reversal error or a relation type error
            mapped = None
            mapped_mismatch = None
            for key_rm in local_key_rms:
                key_arg1_ent = key_entity_map[key_rm.arg1]
                key_arg2_ent = key_entity_map[key_rm.arg2]
                if mapped_arg1 == key_rm.arg1 and mapped_arg2 == key_rm.arg2:
                    if test_rm.rtype == key_rm.rtype:
                        mapped = key_rm
                    else:
                        mapped_mismatch = key_rm
                elif mapped_arg1 == key_rm.arg2 and mapped_arg2 == key_rm.arg1:
                    if test_rm.rtype == key_rm.rtype and key_rm.is_symmetric():
                        mapped = key_rm
                    else:
                        mapped_mismatch = key_rm
            if mapped:
                rm_mapped.append(Mapping(mapped, test_rm, RelMentionAlignment.EXACT))
            elif mapped_mismatch:
                rm_mapped.append(Mapping(mapped_mismatch, test_rm, RelMentionAlignment.TYPE_MISMATCH))

            ## Find key relation mentions that match this in terms of entities
            ## Store them in entity_mapped_key_rms and entity_mapped_mismatch_key_rms

            # Get key entities for test relation
            mapped_arg1_ent = key_entity_map[mapped_arg1]
            mapped_arg2_ent = key_entity_map[mapped_arg2]
                
            entity_match = False
            entity_match_mismatch = False
            for key_rm in local_key_rms:
                # Get key entities for key relation
                key_arg1_ent = key_entity_map[key_rm.arg1]
                key_arg2_ent = key_entity_map[key_rm.arg2]
                if mapped_arg1_ent == key_arg1_ent and mapped_arg2_ent == key_arg2_ent:
                    if key_rm.rtype == test_rm.rtype:
                        entity_mapped_key_rms.append(key_rm)
                        entity_match = True
                    else:
                        entity_mapped_mismatch_key_rms.append(key_rm)
                        entity_match_mismatch = True                            
                elif mapped_arg1_ent == key_arg2_ent and mapped_arg2_ent == key_arg1_ent:
                    if test_rm.rtype == key_rm.rtype and key_rm.is_symmetric():
                        entity_mapped_key_rms.append(key_rm)
                        found_entity_match = True                            
                    else:
                        entity_mapped_mismatch_key_rms.append(key_rm)
                        found_reversed_entity_match = True

            # Store alignment for test_rm if it doesn't already have one
            # We don't align to any particular key_rm based on entities, 
            #   because it could be one of many
            if mapped or mapped_mismatch:
                pass
            elif entity_match:                   
                rm_mapped.append(Mapping(None, test_rm, RelMentionAlignment.SPURIOUS_ENTITY_MATCH))
            elif entity_match_mismatch:                   
                rm_mapped.append(Mapping(None, test_rm, RelMentionAlignment.SPURIOUS_ENTITY_TYPE_MISMATCH))
            else:
                rm_mapped.append(Mapping(None, test_rm, RelMentionAlignment.SPURIOUS))

        # Deal with missing relation mentions
        for key_rm in local_key_rms:
            if key_rm not in [x.key for x in rm_mapped]:
                mapped_arg1 = key_test_em_mapping[key_rm.arg1]
                mapped_arg2 = key_test_em_mapping[key_rm.arg2]
                if not mapped_arg1 or not mapped_arg2:
                    rm_mapped.append(Mapping(key_rm, None, RelMentionAlignment.MISSING_EM))
                elif key_rm in entity_mapped_key_rms:
                    rm_mapped.append(Mapping(key_rm, None, RelMentionAlignment.MISSING_ENTITY_MATCH))
                elif key_rm in entity_mapped_mismatch_key_rms:
                    rm_mapped.append(Mapping(key_rm, None, RelMentionAlignment.MISSING_ENTITY_TYPE_MISMATCH))
                else:
                    rm_mapped.append(Mapping(key_rm, None, RelMentionAlignment.MISSING))
             

################################
### HELPER PRINT FUNCTIONS
################################

# Get printable precision, recall, F scores
def get_prf(correct, missing, spurious):
    r = None
    p = None
    r_str = "n/a"
    p_str = "n/a"
    f_str = "n/a"
    if correct + missing != 0:
        r = 100 * float(correct) / (correct + missing)
        r_str = "%6.2f" % r
    if correct + spurious != 0:
        p = 100 * float(correct) / (correct + spurious)
        p_str = "%6.2f" % p
    if r > 0 or p > 0:
        f_str = "%6.2f" % (2 * r * p / (r + p))
    elif r != None or p != None:
        f_str = "%6.2f" % 0.0
    return "%6s %6s %6s" % (r_str, p_str, f_str)

def print_em_by_etype(etype):
    ## We consider samestart and sameend to be strict matches
    exact      = len([x for x in em_mapped if x.alignment == MentionAlignment.EXACT         and x.key.matches_etype(etype)])
    same_start = len([x for x in em_mapped if x.alignment == MentionAlignment.SAME_START    and x.key.matches_etype(etype)])
    same_end   = len([x for x in em_mapped if x.alignment == MentionAlignment.SAME_END      and x.key.matches_etype(etype)])
    mismatch   = len([x for x in em_mapped if x.alignment == MentionAlignment.TYPE_MISMATCH and x.key.matches_etype(etype)])
    spurious   = len([x for x in em_mapped if x.alignment == MentionAlignment.SPURIOUS      and x.test.matches_etype(etype)])
    missing    = len([x for x in em_mapped if x.alignment == MentionAlignment.MISSING       and x.key.matches_etype(etype)])
    print_em(etype, exact + same_start + same_end, mismatch, missing, spurious)

def print_em_by_mtype(mtype):
    ## We consider samestart and sameend to be strict matches
    exact      = len([x for x in em_mapped if x.alignment == MentionAlignment.EXACT         and x.key.matches_mtype(mtype)])
    same_start = len([x for x in em_mapped if x.alignment == MentionAlignment.SAME_START    and x.key.matches_mtype(mtype)])
    same_end   = len([x for x in em_mapped if x.alignment == MentionAlignment.SAME_END      and x.key.matches_mtype(mtype)])
    mismatch   = len([x for x in em_mapped if x.alignment == MentionAlignment.TYPE_MISMATCH and x.key.matches_mtype(mtype)])
    spurious   = len([x for x in em_mapped if x.alignment == MentionAlignment.SPURIOUS      and x.test.matches_mtype(mtype)])
    missing    = len([x for x in em_mapped if x.alignment == MentionAlignment.MISSING       and x.key.matches_mtype(mtype)])
    print_em(mtype, exact + same_start + same_end, mismatch, missing, spurious)

def print_em(criterion, exact, mismatch, missing, spurious):
    strict_prf = get_prf(exact, missing+mismatch, spurious+mismatch)
    loose_prf = get_prf(exact + mismatch, missing, spurious)
    print "%5s  %5d %5d %5d %5d  %s  %s" % (criterion, exact, mismatch, missing, spurious, strict_prf, loose_prf)

def print_prf(criterion, correct, missing, spurious):
    print "%25s %5d %5d %5d  %s" % (criterion, correct, missing, spurious, get_prf(correct, missing, spurious))

def print_rm_by_rtype(rtype):
    exact_rm = len([x for x in rm_mapped if x.alignment == RelMentionAlignment.EXACT and x.key.matches_rtype(rtype)])
    loose_rm = len([x for x in rm_mapped if x.alignment == RelMentionAlignment.TYPE_MISMATCH and x.key.matches_rtype(rtype)])
    spurious = len([x for x in rm_mapped if is_spurious_rma(x.alignment) and x.test.matches_rtype(rtype)])
    missing  = len([x for x in rm_mapped if is_missing_rma(x.alignment)  and x.key.matches_rtype(rtype)])
    exact_entity_rm_test = len([x for x in rm_mapped if x.alignment == RelMentionAlignment.SPURIOUS_ENTITY_MATCH and x.test.matches_rtype(rtype)])
    exact_entity_rm_key  = len([x for x in rm_mapped if x.alignment == RelMentionAlignment.MISSING_ENTITY_MATCH and x.key.matches_rtype(rtype)])
    loose_entity_rm_test = len([x for x in rm_mapped if x.alignment == RelMentionAlignment.SPURIOUS_ENTITY_TYPE_MISMATCH and x.test.matches_rtype(rtype)])
    loose_entity_rm_key  = len([x for x in rm_mapped if x.alignment == RelMentionAlignment.MISSING_ENTITY_TYPE_MISMATCH and x.key.matches_rtype(rtype)])
    
    if len(rtype) > 20:
        rtype = rtype[0:20]

    print
    print "%25s %5s %5s %5s  %6s %6s %6s" % (rtype, "CORR", "MISS", "SPUR", "R", "P", "F")
    print_prf("strict rm", exact_rm, missing + loose_rm, spurious + loose_rm)
    print_prf("loose rm", exact_rm + loose_rm, missing, spurious)    
    print_prf("strict rm (entity fudge)", exact_rm + exact_entity_rm_test, missing + loose_rm - exact_entity_rm_key, spurious + loose_rm  - exact_entity_rm_test)    
    print_prf("loose rm (entity fudge)", exact_rm + exact_entity_rm_test + loose_rm + loose_entity_rm_test, missing - loose_entity_rm_key - exact_entity_rm_key, spurious - loose_entity_rm_test - exact_entity_rm_test)
    print


################################
### PRINT ENTITY MENTIONS
################################
print
print "ENTITY MENTION SCORES:"
print
print "%5s  %5s %5s %5s %5s  %6s %6s %6s  %6s %6s %6s" % ("KEY", "TYPE", "TYPE", "", "", "", "STRICT", "", "", "LOOSE", "")
print "%5s  %5s %5s %5s %5s  %6s %6s %6s  %6s %6s %6s" % ("ETYPE", "CORR", "INCO", "MISS", "SPUR", "R", "P", "F", "R", "P", "F")
for etype in ["PER", "ORG", "GPE", "LOC", "FAC", "WEA", "VEH", "ALL"]:
    print_em_by_etype(etype)
    
print
print "%5s  %5s %5s %5s %5s  %6s %6s %6s  %6s %6s %6s" % ("KEY", "TYPE", "TYPE", "", "", "", "STRICT", "", "", "LOOSE", "")
print "%5s  %5s %5s %5s %5s  %6s %6s %6s  %6s %6s %6s" % ("MTYPE", "CORR", "INCO", "MISS", "SPUR", "R", "P", "F", "R", "P", "F")
for mtype in ["NAM", "NOM", "PRO", "ALL"]:
    print_em_by_mtype(mtype)

print

# To print mappings, just iterate through em_mapped, for instance:
#for mapping in em_mapped:
#    if mapping.alignment == MentionAlignment.TYPE_MISMATCH:
#        print mapping.key.text, mapping.key.etype, mapping.test.text, mapping.test.etype

################################
### PRINT RELATION MENTIONS
################################

relation_types = []
breakdown_by_rtype = False
if breakdown_by_rtype:
    for rm in key_relation_mentions.values():
        if rm.rtype not in relation_types:
            relation_types.append(rm.rtype)
    for rm in test_relation_mentions.values():
        if rm.rtype not in relation_types:
            relation_types.append(rm.rtype)

print
print "RELATION MENTION SCORES:"
relation_types.sort()
for rtype in relation_types:
    print_rm_by_rtype(rtype)
print_rm_by_rtype("ALL")
    
