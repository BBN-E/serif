import sys, os, re
import MySQLdb
import xml.etree.ElementTree as ET
import operator, codecs
from unidecode import unidecode
import warnings
from WKDB import WKDB
from actor_dictionary_import_utilities import ImportUtilities

warnings.filterwarnings('ignore')

# Sample params:
# icews-mysql-03.bbn.com azamania azamania bbn_actor_db_tac_kb_20140224 \\raid63\u10\users\azamania\wkdb_creation\WKDB_20140219_no_norm_with_old_data.db

# fact: a fact XML element
def get_fact_text(fact):
    if fact.text is not None:
        return fact.text.strip()
    link = fact.find("link")
    if link.text is not None:
        return link.text.strip()
    return None

# facts_list: a list of XML facts elements
def get_names(facts_list):
    result = []
    for facts in facts_list:
        fact_elements = facts.findall("fact")
        for fact in fact_elements:
            if fact.attrib["name"] == "name" or fact.attrib["name"] == "Name":
                name = get_fact_text(fact)
                if name is not None and len(name) > 1:
                    result.append(name)
    return result

# file: a sexp file that contains a mapping between words and entity type
def load_contraints(file):
    constraints = {}

    f = open(file)
    for line in f:
        if len(line.strip()) == 0:
            continue
        first_start = line.index('(')
        second_start = line.index('(', first_start + 1)
        first_end = line.index(')', second_start)
        second_end = line.index(')', first_end + 1)
        
        s = line[second_start + 1 : first_end].strip()
        t = line[first_end + 1: second_end].strip()
        
        constraints[s.lower()] = t
            
    f.close()
    return constraints

def remove_parentheticals(name):
    orig_name = name
    start = name.find('(')
    while start != -1:
        end = name.find(')', start)
        if end == -1:
            return name.strip()
        name = name[0:start] + name[end+1:]
        if len(name.strip()) == 0:
            return orig_name
        start = name.find('(')
    return name.strip()
    

script_location = os.path.dirname(__file__)
first_names_file = os.path.join(script_location, '..', '..', 'xdoc', 'bin', 'data', 'first_names.txt')
last_names_file = os.path.join(script_location, '..', '..', 'xdoc', 'bin', 'data', 'last_names.txt')
if not os.path.exists(first_names_file) or not os.path.exists(last_names_file):
    print "Could not find xdoc data files: " + first_names_file + " " + last_names_file
    sys.exit(1)

# Input data
knowledge_base_directory = "//mercury-04/u40/ldc_releases/TAC_2009_KBP_Evaluation_Reference_Knowledge_Base/data"
#knowledge_base_directory = "c:/temp/small-kb"

# Words that appear in <facts class="..."> that can determine entity type
wiki_class_word_types = os.path.join(script_location, "script_data_files", "wiki-class-word-types")
# Mapping between <facts class="..."> and entity type
wiki_class_types = os.path.join(script_location, "script_data_files", "wiki-class-types")

if len(sys.argv) != 6:
    print "Usage: bbn_actor_db_server bbn_actor_db_user bbn_actor_db_password bbn_actor_db_name wkdb"
    sys.exit(1)

bbn_actor_db_server, bbn_actor_db_user, bbn_actor_db_password, bbn_actor_db_name, wkdb = sys.argv[1:]

wkdb = WKDB(wkdb, first_names_file, last_names_file)

bbn_conn = MySQLdb.connect(host=bbn_actor_db_server,
                           user=bbn_actor_db_user,
                           passwd=bbn_actor_db_password,
                           db=bbn_actor_db_name,
                           charset='utf8')
bbn_cur = bbn_conn.cursor()

class_word_type_map = load_contraints(wiki_class_word_types)
class_type_map = load_contraints(wiki_class_types)

source_name = "TAC KBP Knowledge Base"
bbn_cur.execute("SELECT source_id FROM sources where source_name=%s", (source_name,))
if bbn_cur.rowcount == 0:
   bbn_cur.execute("INSERT INTO sources (source_name) VALUES (%s)", (source_name,))
   source_id = bbn_cur.lastrowid
else:
   source_id = bbn_cur.fetchone()[0]

kb_filenames = os.listdir(knowledge_base_directory)

unknown_classes = {}
unknown_classes_filenames = {}

for file_name in kb_filenames:
    file = os.path.join(knowledge_base_directory, file_name)
    print "Working on " + file_name

    parser = ET.XMLParser(encoding='utf-8')
    tree = ET.parse(file, parser=parser)
    root = tree.getroot()

    for entity in root.iter("entity"):
        kb_entity_id = entity.attrib["id"]
        canonical_name = entity.attrib["name"]

        all_names = [remove_parentheticals(canonical_name)]

        wiki_title = entity.attrib["wiki_title"]
        entity_type = entity.attrib["type"]

        facts_list = entity.findall("facts")
        ascii_names = []
        for name in all_names:
            ascii_name = unidecode(name)
            
            if ascii_name != name:
                ascii_names.append(ascii_name)
            
        if len(ascii_names) != 0:
            #print "New ascii names: " + str(ascii_names)
            all_names = all_names + ascii_names

        if entity_type == "UKN" and len(facts_list) > 0:
            entity_class = facts_list[0].attrib["class"]
            if entity_class is not None:
                entity_class = entity_class.lower()
                
                if entity_class.startswith("infobox ") or entity_class.startswith("infobox_"):
                    entity_class = entity_class[8:]

                if entity_class in class_type_map:
                    entity_type = class_type_map[entity_class]
                else:
                    for w, et in class_word_type_map.iteritems():
                        if re.search('\\b' + w + '\\b', entity_class):
                            entity_type = et

                if entity_type == "UKN":
                    if entity_class not in unknown_classes:
                        unknown_classes[entity_class] = 0
                    unknown_classes[entity_class] += 1
                    unknown_classes_filenames[entity_class] = file_name

        if entity_type == "NONE":
            continue

        if entity_type == "UKN":
            continue

        wca = wkdb.getWikipediaConservativeAliases(all_names, [])
        if (len(wca) != 0):
            #print "Got WCA for " + unidecode(canonical_name)
            for a in wca:
                #print unidecode(a)
                if a not in all_names:
                    all_names.append(a)

        #print "Inserting: " + unidecode(canonical_name)
        # Actors and their source element mapping
        bbn_cur.execute("INSERT INTO actors (canonical_name, ace_entity_type, iso_code) VALUES (%s, %s, %s)", (canonical_name, entity_type, None))
        bbn_actor_id = bbn_cur.lastrowid

        bbn_cur.execute("INSERT INTO actor_sources (actor_id, source_id, original_source_element, match_confidence) VALUES (%s, %s, %s, %s)", (bbn_actor_id, source_id, kb_entity_id + ":" + wiki_title + ":" + file_name, 1.0,))
        
        inserted_names = []
        # Actor strings
        for n in all_names:
            if not ImportUtilities.valid_string(a):
                continue
            is_acronym = False
            if ImportUtilities.acronym_test(a, wca):
                is_acronym = True

            name_to_insert = n.upper()
            if name_to_insert not in inserted_names:
                bbn_cur.execute("INSERT INTO actor_strings (actor_id, string, confidence, source_id, acronym) VALUES (%s, %s, %s, %s, %s)", (bbn_actor_id, name_to_insert, 0.9, source_id, is_acronym))
                inserted_names.append(name_to_insert)

sorted_classes = sorted(unknown_classes.iteritems(), key=operator.itemgetter(1), reverse=True)

# Uncomment to output KB entities of type UKN that we are ignoring
#out = codecs.open("C:/temp/unknown-classes", 'w', 'utf-8')
#for c, count in sorted_classes:
#    fn = unknown_classes_filenames[c]
#    out.write(c + " " + str(count) + " (" + fn + ")\n")
#out.close()

bbn_conn.commit()
bbn_conn.close()

