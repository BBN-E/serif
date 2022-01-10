import re, sys, os, time

"""
This is a fairly hack-ish script that converts the dtd to a more
readable webpage.  It tries to display the comments in a readable way,
and creates symlinks between elements.  It's fairly specialized to the
actual formatting used in the DTD.
"""

HEADER ='''
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html>
<head>
  <title>SerifXML DTD</title>
<style type="text/css">
%s
body { 
    background: inherit; foreground: inherit;
}
</style>
</head>
<body class="dtd">
<div class="body">
'''
FOOTER = '''
</div>
<p class="timestamp">Automatically generated from the DTD on %s</p>
</body>
</html>
'''

TARGET_RE = re.compile(r'(<!(?:ELEMENT |ENTITY +%) *([\w\.]+))')

def convert_code(s, targets):
    target_anchors = ''.join('<a name="%s"/>\n' % m.group(2)
                             for m in TARGET_RE.finditer(s))
    
    s = s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

    # Add hyperlinks:
    for target in targets:
        s = re.sub(r'(%%?\b(?<!name=")%s\b;?)' % target,
                   r'<a href="#%s">\1</a>' % target, s)
    #s = re.sub(r'name="<a href="#(\w+)">\1</a>"', r'name="\1"', s)
    
    # Add color:
    s = re.sub(r'(?s)((?:<a name="[^"]+"/>)?&lt;!(ENTITY|ELEMENT|ATTLIST).*?&gt;)',
               r'<span class="\2">\1</span>', s)

    s = re.sub(r'(\n<span class="ELEMENT">)',
               r'<hr class="ELEMENT"/>\1', s)
    #s = re.sub(r'</div>\s*<div', r'</div><div', s)

    return '%s<pre class="dtd">\n%s</pre>\n' % (target_anchors, s)

def convert_comment(s, targets):
    # Strip the comment marker
    assert s.startswith('<!--') and s.endswith('-->')
    s = s[4:-3].strip()
    
    # Split on blank lines.
    paragraphs = re.split('\n *\n', s)
    result = ''
    for para in paragraphs:
        para = para.replace('&', '&amp;')
        para = para.replace('<', '&lt;').replace('>', '&gt;')
        for target in targets:
            para = re.sub(r'(&lt;%s&gt;)' % target,
                          r'<a href="#%s">\1</a>' % target, para)
            para = re.sub(r'(%%%s;)' % target,
                          r'<a href="#%s">\1</a>' % target, para)

        # List items
        if para.lstrip().startswith('*'):
            indent = ' '*(len(para) - len(para.lstrip()))
            items = re.findall(r'\*.*(?:\n *[^\* ].*)*', para)
            para = '\n'.join('%s<ul>\n%s  <li>%s</li>\n%s</ul>' %
                             (indent, indent, item[1:].lstrip(), indent)
                             for item in items)

        # Headers
        para = re.sub(r'\A\s*###+ *\n *# *(.*)#\n\s*###+ *\Z', r'<h1>\1</h1>', para)
        para = re.sub(r'\A\s*###+ *\n *# *(.*)\n\s*###+ *\Z', r'<h2>\1</h2>', para)
        para = re.sub(r'\A\s*___+ *(.*?) *___+\s*\Z', r'<h3>\1</h3>', para)

        # Hack: parse example
        para = re.sub(r'(?s)\A(\s*&lt;Parse.*)', r'<pre>\1</pre>', para)
        para = re.sub(r'&lt;parse_id&gt;', r'<i>parse_id</i>', para)

        # Paragraphs.
        para = re.sub(r'(?s)\A\s*([^\*\<\s].*)\s*\Z', r'<p>\1</p>', para)

        # Bold
        para = re.sub(r'\*\*(\S.*?)\*\*', r'<b>\1</b>', para)
        
        result += para + '\n\n'

    # Remove duplicate <ul>s
    result = re.sub(r'(?m)^( *)</ul>\n*\1<ul>\n', '', result)
    result = re.sub(r'(?m)^( *)</ul>\n*\1<ul>\n', '', result)
    
    return result

def main(infile='serifxml.dtd', outfile='../http_server/serifxml_dtd.html',
         cssfile='../http_server/serif.css'):
    print 'reading dtd (%s)...' % infile
    s = open(infile).read()
    s = s.expandtabs()

    # Get a list of all targets.
    print 'finding targets...'
    targets = [m.group(2) for m in TARGET_RE.finditer(s)]
    
    # Split the dtd into comments (odd pieces) and code (even pieces)
    pieces = re.split('(?s)(<!--.*?-->)', s)

    # Process each piece appropriately.
    print 'processing...'
    page = ''
    for i, piece in enumerate(pieces):
        piece = piece.strip()
        if not piece: continue
        if i%2 == 0:
            page += convert_code(piece, targets)+'\n'
        else:
            page += convert_comment(piece, targets)+'\n'
        sys.stdout.write('.')
        sys.stdout.flush()
    print
    
    # Add div's for items.
    page = re.sub(r'(?s)(<h3>.*?(?=<h[123]>|\Z))',
                  r'<div class="dtd_item">\n\1</div>\n', page)

    # Generate the output file.
    print 'writing output html...'
    print '  -> %s' % outfile
    out = open(outfile, 'wb')
    out.write(HEADER % open(cssfile).read())
    out.write(page)
    out.write(FOOTER % time.ctime())
    out.close()

def usage(error='', exitcode=-1):
    print "%s [dtdfile htmlfile]" % sys.argv[0]
    if error: print error
    sys.exit(exitcode)

if len(sys.argv) == 1:
    main()
    sys.exit(0)
elif len(sys.argv) == 3:
    if not os.path.exists(sys.argv[1]):
        usage('%s not found' % sys.argv[1])
    if os.path.exists(sys.argv[2]):
        usage('%s already exists' % sys.argv[2])
    main(sys.argv[1], sys.argv[2])
else:
    usage()
 
