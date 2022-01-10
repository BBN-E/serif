# Sample parameter: /nfs/raid63/u11/users/azamania/ACCENT/accent-2016-04-17/expts/Serif/icews/lib/event_models

import sys, os

if len(sys.argv) != 2:
    print "Usage: event-models-directory"
    sys.exit(1)

def obfuscate_file(file):
    os.rename(file, file + ".bak")
    o = open(file, 'w')
    i = open(file + ".bak", 'r')

    contents = ""
    for line in i:
        line = line.strip()
        if line.startswith("#"):
            continue
        pos = line.find('#')
        if pos > -1:
            line = line[0:pos]
        contents += line + " "
    i.close()
    contents = contents.replace("  ", " ")
    o.write(contents)
    o.close()
    os.remove(file + ".bak")

def obfuscate_directory(dir):
    filenames = os.listdir(dir)
    for filename in filenames:
        file = os.path.join(dir, filename)
        if os.path.isfile(file) and not file.endswith(".bak"):
            print filename
            obfuscate_file(file)
        elif os.path.isdir(file):
            obfuscate_directory(file)

obfuscate_directory(sys.argv[1])

