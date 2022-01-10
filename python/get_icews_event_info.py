#!/bin/env python

import sys, os, serifxml, codecs, re

def read_event_types(event_types_file):
    return_cache = dict()
    code_re = re.compile(r'code "(.*?)"')
    name_re = re.compile(r'name "(.*?)"')
    i = open(event_types_file, 'r')
    current_code = None
    for line in i:
        code_match = code_re.search(line)
        if code_match:
            current_code = code_match.group(1)
            continue
        name_match = name_re.search(line)
        if name_match:
            if not current_code:
                print "No current code!"
                sys.exit(1)
            return_cache[current_code] = name_match.group(1)
    return return_cache

def get_actor_name(actor_mention):
    if actor_mention.actor_name:
        return actor_mention.actor_name

    agent_name_string = ""
    if actor_mention.paired_agent_name:
        agent_name_string += actor_mention.paired_agent_name
        
    if actor_mention.paired_actor_name:
        if len(agent_name_string) > 0:
            agent_name_string += " FOR "
            
        agent_name_string += actor_mention.paired_actor_name
    else:
        if len(agent_name_string) > 0:
            agent_name_string += " FOR "
        agent_name_string += "UNKNOWN"

    return agent_name_string

if __name__=='__main__':
    if len(sys.argv) != 3:
        print "Usage: serifxml_file output_file"
        sys.exit(1)
    
    serifxml_file, output_file = sys.argv[1:]
    event_types_file = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "..", "W-ICEWS", "lib", "event_types.txt")
    event_type_map = dict() # code to name
    if os.path.exists(event_types_file):
        event_type_map = read_event_types(event_types_file)

    serif_doc = serifxml.Document(serifxml_file)
            
    o = codecs.open(output_file, 'w', encoding='utf8')
    o.write("<html>\n")

    sentence_theory_to_sentence = dict()
    for sentence in serif_doc.sentences:
        for st in sentence.sentence_theories:
            sentence_theory_to_sentence[st] = sentence

    # pattern_id to [ sentence1, sentence2, ... ]
    pattern_id_to_sentence_info = dict()

    for iem in serif_doc.icews_event_mention_set:
        event_code = iem.event_code
        pattern_id = iem.pattern_id
        time = iem.time_value_mention
        if pattern_id not in pattern_id_to_sentence_info:
            pattern_id_to_sentence_info[pattern_id] = []

        sentence_theory = None
        source = None
        target = None
        location = None
    
        for participant in iem.participants:
            if participant.actor is not None:
                if participant.role == "SOURCE":
                    source = participant.actor
                    sentence_theory = participant.actor.sentence_theory
                if participant.role == "TARGET":
                    target = participant.actor
                    sentence_theory = participant.actor.sentence_theory
                if participant.role == "LOCATION":
                    location = participant.actor
            
        pattern_id_to_sentence_info[pattern_id].append((sentence_theory, event_code, source, target, location, time,))

    seen_sentences = set()
    for pattern_id, sentence_infos in sorted(pattern_id_to_sentence_info.iteritems(), key=lambda x:x[0]):
        o.write("<p><font size=\"5\">" + pattern_id + "</font></p>\n")
        for sentence_info in sorted(sentence_infos, key=lambda x: sentence_theory_to_sentence[x[0]].sent_no):
            sentence_theory = sentence_info[0]
            event_code = sentence_info[1]
            source = sentence_info[2]
            target = sentence_info[3]
            location = sentence_info[4]
            time = sentence_info[5]

            sentence = sentence_theory_to_sentence[sentence_theory]
            seen_sentences.add(sentence)
            if event_code in event_type_map:
                o.write("<b>" + event_type_map[event_code] + " (" + str(event_code) + ")</b><br>\n")
            o.write(str(sentence.sent_no) + ". ")
            for token in sentence_theory.token_sequence:
                if source is not None and source.mention.syn_node.start_token == token:
                    o.write("<font color=\"blue\"><b>")
                if target is not None and target.mention.syn_node.start_token == token:
                    o.write("<font color=\"green\"><b>")
                o.write(token.text + " ")
                if source is not None and source.mention.syn_node.end_token == token:
                    o.write("</b></font>")
                if target is not None and target.mention.syn_node.end_token == token:
                    o.write("</b></font>")
                
            if location:
                print "Writing location: " + location.mention.text
                o.write("\n(<font color=\"orange\">" + location.mention.text + "</font>)\n")
        
            if time:
                o.write("\n(<font color=\"red\">" + time.text + "</font>)\n")
        
            o.write("<br>")

            if source is not None:
                o.write("<br><font color=\"blue\"><b>" + get_actor_name(source) + "</b></font>")
            if target is not None:
                o.write("<br><font color=\"green\"><b>" + get_actor_name(target) + "</b></font><br>")
            if source is not None or target is not None:
                o.write("<br>\n")
            
            o.write("<br>")


    o.write("<p><font size=\"5\">Unmatched Sentences</font></p>\n")
    for sentence in serif_doc.sentences:
        if sentence not in seen_sentences:
            o.write(str(sentence.sent_no) + ". ")
            for token in sentence.token_sequence:
                o.write(token.text + " ")
            o.write("<br><br>")
    o.write("</html>")

    o.close()

