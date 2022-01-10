#!/bin/env python

# Copyright 2011 by BBN Technologies Corp.
# All Rights Reserved.


import socket, sys, re, xml.sax.saxutils

class SerifHTTPClient:
    """
    A simple client that can be used to communicate with a
    SerifHTTPServer that is already running.
    """

    # Template used to send request messages.
    REQUEST_TEMPLATE = (
        '<SerifXMLRequest%(session_id)s>\n'
        '  <ProcessDocument%(options)s>\n'
        '    %(document)s\n'
        '  </ProcessDocument>\n'
        '</SerifXMLRequest>\n')

    # Template used to initialize document.
    DOCUMENT_TEMPLATE = (
        '<Document docid="testdoc" language="%(language)s">\n'
        '  <OriginalText><contents>%(text)s</contents></OriginalText>\n'
        '</Document>\n')

    def __init__(self, language, host, port):
        self.language = language
        self.host = host
        self.port = port

    def process_file(self, my_file, end_stage="output",
                     output_format='serifxml',
                     session_id=None, **opts):
        f = open(my_file, 'r')
        text = f.read()
        f.close()
        return self.process_text(text, end_stage, output_format,
                                 session_id, **opts)

    def process_xml(self, document, start_stage, end_stage="output",
                    output_format='serifxml', session_id=None, **opts):
        request = self.create_request(document, session_id,
                                      start_stage=start_stage,
                                      end_stage=end_stage,
                                      output_format=output_format,
                                      **opts)
        return self.process_request(request)

    def process_text(self, text, end_stage="output",
                     output_format='serifxml',
                     session_id=None, **opts):
        document = SerifHTTPClient.DOCUMENT_TEMPLATE % \
            dict(text=xml.sax.saxutils.escape(text),
                 language=self.language)
        request = self.create_request(document,
                                      session_id,
                                      start_stage="START",
                                      end_stage=end_stage,
                                      output_format=output_format,
                                      **opts)
        return self.process_request(request)

    def create_request(self, document, session_id=None, **opts):
        opt_str = ""
        id_str = ""
        if session_id is not None:
            id_str = " session_id='%s'" % session_id
        for key, val in opts.items():
            opt_str += " " + key + "=\"" + val + "\""
        return SerifHTTPClient.REQUEST_TEMPLATE % \
            dict(session_id=id_str, document=document, options=opt_str)

    def process_request(self, request):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((self.host,self.port))
        except socket.error, msg:
            raise ValueError('Could not connect to server: %s' % msg)
        try:
            print 'Sending request to Serif%s at %s:%s' % (self.language,
                                                           self.host,
                                                           self.port)
            # Send our request
            s.send("POST SerifXMLRequest HTTP/1.0\r\n")
            s.send("content-length: %d\r\n\r\n" % len(request))
            s.send(request)
            # Read the response.
            response = ''
            while 1:
                received = s.recv(1024)
                if not received: break
                response += received
            s.close()
            # Parse the response
            m = re.match("HTTP/1.0 (?P<status>.*?)\r?\n"
                         "(?P<header>[ \t]*\S.*\r?\n)*\r?\n(?P<body>[\s\S]*)",
                         response)
            if not m:
                raise ValueError("Bad response from Serif!")
            if m.group("status") != "200 OK":
                raise ValueError(m.group("status"))
            return m.group("body")
        except socket.error, msg:
            s.close()
            raise ValueError(msg)

    def shutdown_server(self):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((self.host, self.port))
        except socket.error, msg:
            raise ValueError('Could not connect to server: %s' % msg)
        try:
            s.send("POST Shutdown HTTP/1.0\r\n")
            s.send("content-length: 0\r\n\r\n")
            # Read the response.
            response = ''
            while 1:
                received = s.recv(1024)
                if not received: break
                response += received
            s.close()
            # Parse the response
            m = re.match("HTTP/1.0 (?P<status>.*?)\r?\n"
                         "(?P<header>[ \t]*\S.*\r?\n)*\r?\n(?P<body>[\s\S]*)",
                         response)
            if not m:
                raise ValueError("Bad response from Serif!")
            if m.group("status") != "200 OK":
                raise ValueError(m.group("status"))
            return m.group("body")
        except socket.error, msg:
            s.close()
            raise ValueError(msg)

if __name__ == '__main__':

    client = SerifHTTPClient('English', 'localhost', 8000)

    # Construct the test document.
    test_doc = 'Evan saw a man on the hill with a telescope.  He was tall.'

    # Send it to the server.
    results = [test_doc]
    results.append(client.process_text(test_doc, output_format='serifxml'))

    # Print results
    for document in results:
        print '-'*75
        print document
