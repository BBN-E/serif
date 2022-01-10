import sys, os, codecs
import xml.etree.ElementTree as ET

# Input data
knowledge_base_directory = "/nfs/mercury-04/u40/ldc_releases/TAC_2009_KBP_Evaluation_Reference_Knowledge_Base/data"
entity_linking_queries_2009 = "/nfs/mercury-04/u40/ldc_releases/TAC_2009_KBP_Evaluation_Entity_Linking_List/data/entity_linking_queries.xml"
entity_linking_answers_2009 = "/nfs/mercury-04/u40/ldc_releases/TAC_2009_KBP_Gold_Standard_Entity_Linking_Entity_Type_List/data/Gold_Standard_Entity_Linking_List_with_Entity_Types.tab"
entity_linking_queries_and_answers_2010 = "/nfs/mercury-04/u40/ldc_releases/TAC_2010_KBP_Training_Entity_Linking_V2.0/data/tac_2010_kbp_training_entity_linking_queries.xml";
entity_linking_queries_2011 = "/nfs/mercury-04/u40/ldc_releases/TAC_2011_KBP_Cross_Lingual_Training_Entity_Linking_V1.1/data/tac_2011_kbp_cross_lingual_training_entity_linking_queries.xml"
entity_linking_answers_2011 = "/nfs/mercury-04/u40/ldc_releases/TAC_2011_KBP_Cross_Lingual_Training_Entity_Linking_V1.1/data/tac_2011_kbp_cross_lingual_training_entity_linking_query_types.tab"
entity_linking_queries_2012 = "/nfs/mercury-04/u40/ldc_releases/TAC_2012_KBP_English_Entity_Linking_Evaluation_Annotations_V1.1/data/tac_2012_kbp_english_evaluation_entity_linking_queries.xml"
entity_linking_answers_2012 = "/nfs/mercury-04/u40/ldc_releases/TAC_2012_KBP_English_Entity_Linking_Evaluation_Annotations_V1.1/data/tac_2012_kbp_english_evaluation_entity_linking_query_types.tab"

class KBEntity:
    """A simple class for storing actors from TAC Knowledge Base"""
    def __init__(self, id, name, wiki_title, entity_type):
        self.id = id
        self.name = name
        self.wiki_title = wiki_title
        self.entity_type = entity_type

# documents: a dict which this function adds to
# queries: a file containing TAC KBP queries -- each query is an entity id, an entity name and document
# answers: a mapping between query id and a knowlege base entity
# kb_entities: the knowledge base entities that we already read in
#
# Queries can also contain their answers
def get_documents(documents, queries, answers, kb_entities):
    entity_answers = {} # a mapping between query_id and KBEntity
    if answers is not None:
        f = open(answers)
        for line in f:
            pieces = line.split('\t')
            if len(pieces) != 3:
                pieces = pieces[0:3]
            (query_id, kb_entity_id, entity_type) = pieces

            if kb_entity_id[0:2] == "E0" and kb_entity_id not in kb_entities:
                print "Could not find " + kb_entity_id + " in kb_entities!"
                sys.exit(2)

            if kb_entity_id in kb_entities and kb_entities[kb_entity_id].entity_type == "UKN":
                kb_entities[kb_entity_id].entity_type = entity_type

            if kb_entity_id not in kb_entities:
                kb_entity = KBEntity(kb_entity_id, None, None, entity_type)
                kb_entities[kb_entity_id] = kb_entity
        
            entity_answers[query_id] = kb_entities[kb_entity_id]

    parser = ET.XMLParser(encoding='utf-8')
    tree = ET.parse(queries, parser=parser)
    root = tree.getroot()
    
    for query in root.iter("query"):
        query_id = query.attrib["id"]
        docid = query.find("docid").text
        name = query.find("name").text

        if docid.find("CMN_") != -1:
            continue

        # Look for answer in query itself
        entity = query.find("entity")
        if entity is not None:
            kb_entity_id = entity.text
            if kb_entity_id[0:3] == "NIL" and kb_entity_id not in kb_entities:
                kb_entity = KBEntity(kb_entity_id, None, None, None)
                kb_entities[kb_entity_id] = kb_entity

            if kb_entity_id[0:2] == "E0" and kb_entity_id not in kb_entities:
                print "Could not find " + kb_entity_id + " in kb_entities!"
                sys.exit(2)
            entity_answers[query_id] = kb_entities[kb_entity_id]
        
        kb_entity = entity_answers[query_id]
        query_entity = (name, kb_entity, query_id)
        
        if not docid in documents:
            documents[docid] = []
        documents[docid].append(query_entity)

def print_documents(documents, outfile):
    outstream = open(outfile, 'w')

    root = ET.Element("entity_linking_annotation")
    for document, query_info_list in documents.iteritems():
        doc_elem = ET.SubElement(root, "document")
        doc_elem.attrib["docid"] = document
        for query_info in query_info_list:
            query_elem = ET.SubElement(doc_elem, "query")
            query_elem.attrib["name"] = query_info[0]
            query_elem.attrib["tac_query_id"] = query_info[2]
        
            tac_corpus_match_elem = ET.SubElement(query_elem, "tac_corpus_match")
            kb_entity = query_info[1]
            if kb_entity.wiki_title is not None:
                tac_corpus_match_elem.attrib["wiki_title"] = kb_entity.wiki_title
            if kb_entity.entity_type is not None:
                tac_corpus_match_elem.attrib["entity_type"] = kb_entity.entity_type
            tac_corpus_match_elem.attrib["tac_entity_id"] = kb_entity.id

    outstream.write(ET.tostring(root))
    outstream.close()

if len(sys.argv) != 2:
    print "Parameters: output_directory"
    sys.exit(1)

output = sys.argv[1]

if not os.path.exists(output):
    os.makedirs(output)

kb_filenames = os.listdir(knowledge_base_directory)

print "Importing entities from knowledge base..."
kb_entities = { }
for file_name in kb_filenames:
        
    file = os.path.join(knowledge_base_directory, file_name)
    print "Working on " + file_name

    tree = ET.parse(file)
    root = tree.getroot()

    for entity in root.iter("entity"):
        kb_entity = KBEntity(entity.attrib["id"], entity.attrib["name"], entity.attrib["wiki_title"], entity.attrib["type"])
        kb_entities[entity.attrib["id"]] = kb_entity

print "Done importing entities from knowledge base\n"

# Dicts keyed on document name, containing query entities
documents_2009 = {} 
documents_2010 = {}
documents_2011 = {}
documents_2012 = {}

get_documents(documents_2009, entity_linking_queries_2009, entity_linking_answers_2009, kb_entities.copy())
get_documents(documents_2010, entity_linking_queries_and_answers_2010, None, kb_entities.copy())
get_documents(documents_2011, entity_linking_queries_2011, entity_linking_answers_2011, kb_entities.copy())
get_documents(documents_2012, entity_linking_queries_2012, entity_linking_answers_2012, kb_entities.copy())

# Print documents
print_documents(documents_2009, os.path.join(output, "tac_kbp_entity_linking_annotation_2009.xml"))
print_documents(documents_2010, os.path.join(output, "tac_kbp_entity_linking_annotation_2010.xml"))
print_documents(documents_2011, os.path.join(output, "tac_kbp_entity_linking_annotation_2011.xml"))
print_documents(documents_2012, os.path.join(output, "tac_kbp_entity_linking_annotation_2012.xml"))

