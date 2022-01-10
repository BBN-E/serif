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
  <title>%(name)s</title>
<style type="text/css">
%(css)s
</style>
</head>
<body class="dtd">
<div class="body">
<h1>%(name)s</h1>
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
    
    s = s.replace('&', '&amp;')
    s = s.replace('<', '&lt;').replace('>', '&gt;')
    for target in targets:
        s = re.sub(r'(&lt;%s&gt;)' % target,
                      r'<a href="#%s">\1</a>' % target, s)
        s = re.sub(r'(%%%s;)' % target,
                      r'<a href="#%s">\1</a>' % target, s)
    return '<pre>%s</pre>\n' % s

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
    out.write(HEADER % dict(name=infile.replace('_', ': '),
                            css=open(cssfile).read()))
    out.write(page)
    out.write(FOOTER % time.ctime())
    out.close()

def usage(error='', exitcode=-1):
    print "%s dtdfile htmlfile" % sys.argv[0]
    if error: print error
    sys.exit(exitcode)

if len(sys.argv) == 3:
    if not os.path.exists(sys.argv[1]):
        usage('%s not found' % sys.argv[1])
    if os.path.exists(sys.argv[2]):
        usage('%s already exists' % sys.argv[2])
    main(sys.argv[1], sys.argv[2])
    sys.exit(0)
else:
    usage()
 
