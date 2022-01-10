// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// Copyright (c) 2007 by BBN Technologies, Inc.
// All Rights Reserved.

// General socket server for connecting to Serif

#include "Generic/common/leak_detection.h" //This must be the first #include

#include "Generic/common/version.h"

#include "Generic/driver/DocumentDriver.h"
#include "Generic/driver/SessionProgram.h"
#include "Generic/common/ParamReader.h"
#include "Generic/apf/APFResultCollector.h"
#include "Generic/apf/APF4ResultCollector.h"
#include "Generic/eeml/EEMLResultCollector.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/state/ObjectIDTable.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <exception>

#if defined(WIN32) || defined(WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <tchar.h>
#elif defined(UNIX)
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

using namespace std;

#define OK 1
#define ERROR 0

#if defined(UNIX)
#define SOCKET int
#define SD_SEND SHUT_WR
#define INVALID_SOCKET -1
#endif

size_t get_command (char *buffer, char *command);
bool hasDocId(wchar_t* document, wstring *docId);
bool hasTextSection(wchar_t* document);
void stringPrintEmptyResults(wstring &results, wstring &docId);
wchar_t getNextUTF8Char(char * text, size_t & pos, size_t max_pos);
void makeUTF8CharArray(const wchar_t* input, char* output, int max_len);

#if defined(WIN32) || defined(WIN64)
void initializeWinsock();
#endif

void initializeListenSocket(SOCKET& sock, const char *port);
void acceptSocket(SOCKET& serv_sock, SOCKET& client_sock);
void closeSocket(SOCKET& sock);
int readCommandSize(SOCKET& sock, char *buf, int max_length);
int readFromSocket(SOCKET& sock, char *buf, int num_characters);

void usage ()
{
	cerr << "Usage: SerifSocketServer.exe <paramfile> <port>\n"
		<< "\tparamfile - Serif parameters file\n"
		<< "\tport - port number for server\n";
}

// This is the entry point for this application
int main(int argc, char **argv)
{ 
	char *buffer = 0;
	wchar_t *wbuffer = 0;
	char *doc;
	char command[256];
	char error_string[1000];
	char out_bytes[1000];
	wstring results;

	SessionProgram *sessionProgram;
	ResultCollector *resultCollector;
	DocumentDriver *documentDriver;

	cout << "This is Serif Socket Server, version 1.0\n"
		 << "Copyright (c) 2008 by BBN Technologies, Inc.\n"
		 << "Serif version: " << SerifVersion::getVersionString() << "\n"
		 << "Serif Language Module: " << SerifVersion::getSerifLanguage().toString() << "\n"
		 << "\n" << flush;

	if (argc != 3)
	{ 
		usage ();
		return -1;
	} 

#if defined(WIN32) || defined(WIN64)
	initializeWinsock();
#endif

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	initializeListenSocket(ListenSocket, argv[2]);

	try {
		ParamReader::readParamFile (argv[1]);
		sessionProgram = new SessionProgram;
		char outputFormat[500];
		ParamReader::getRequiredParam("output_format",outputFormat, 500);
		if (!strcmp(outputFormat, "EEML"))
			resultCollector = new EEMLResultCollector();
		else if (!strcmp(outputFormat, "APF"))
			resultCollector = new APFResultCollector();
		else if (!strcmp(outputFormat, "APF4"))
			resultCollector = new APF4ResultCollector(APF4ResultCollector::APF2004);
		else if (!strcmp(outputFormat, "APF5"))
			resultCollector = new APF4ResultCollector(APF4ResultCollector::APF2005);
		else if (!strcmp(outputFormat, "APF7"))
			resultCollector = new APF4ResultCollector(APF4ResultCollector::APF2007);
		else if (!strcmp(outputFormat, "APF8"))
			resultCollector = new APF4ResultCollector(APF4ResultCollector::APF2008);
		else
			throw UnexpectedInputException("socket-server::main()",
										   "Invalid output-format");

		documentDriver = new DocumentDriver (sessionProgram, resultCollector);
	} catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage () << "\n";
		return -1;
	} 
	catch (exception &e) {
		cerr << "\nError: " << e.what() << " in SerifSocketServer::main()\n";		
		return -1;
	}

	while (1) {

		printf ("Waiting for a connection...\n");

		acceptSocket(ListenSocket, ClientSocket);

		// grab everything up to the first space, this is the length of the command
		char command_size[10];
		int rv = readCommandSize(ClientSocket, command_size, 9);
		if (rv) {
			printf ("Bad command size.\n\n");
			strcpy_s(error_string, "Could not read command size from beginning of stream");
			sprintf_s(out_bytes, "%d\n%d\n%s", ERROR, strlen(error_string), error_string);
			send(ClientSocket, out_bytes, (int)strlen(out_bytes), 0);
			continue;
		}

		delete [] buffer;
		buffer = new char[atoi(command_size) + 1];
		readFromSocket(ClientSocket, buffer, atoi(command_size));

		size_t offset = get_command(buffer, command);

		if (!strcmp(command, "SHUTDOWN")) {
			printf ("Shutdown by client.\n\n");
			sprintf_s(out_bytes, "%d\n0\n", OK);
			send(ClientSocket, out_bytes, (int)strlen(out_bytes), 0);
			break;
		}
		
		if (!strcmp(command, "DISCONNECT")) {
			printf ("Disconnect command received.\n\n");
			sprintf_s(out_bytes, "%d\n0\n", OK);
			send(ClientSocket, out_bytes, (int)strlen(out_bytes), 0);		
			closeSocket(ClientSocket);
			continue;
		}
		
		if (strcmp(command, "ANALYZE")) {
			printf ("Unknown command: %s\n\n", command);
			sprintf_s(error_string, "Unknown command: %s", command);
			sprintf_s(out_bytes, "%d\n%d\n%s", ERROR, strlen(error_string), error_string);
			send(ClientSocket, out_bytes, (int)strlen(out_bytes), 0);
			continue;
		}

		// we have an ANALYZE command
		doc = buffer + offset + 1;
		size_t array_size = strlen(doc) + 1;

		delete [] wbuffer;
		wbuffer = new wchar_t[array_size];

		size_t pos = 0;
		int utf8_pointer = 0;
		while (pos < array_size - 1) {
			wbuffer[utf8_pointer] = getNextUTF8Char(doc, pos, array_size - 1);
			utf8_pointer++;
		}
		wbuffer[utf8_pointer] = L'\0';
		
		results = L"";
		try
		{ 
			std::wstring docId(L"NONE_FOUND");

			if (!hasDocId(wbuffer, &docId)) {
				printf("Error - Could not find well-formed DOCID section\n");
				sprintf_s(error_string, "SERIF ERROR: No DOCID found\n", command);
				sprintf_s(out_bytes, "%d\n%d\n%s", ERROR, strlen(error_string), error_string);
				send(ClientSocket, out_bytes, (int)strlen(out_bytes), 0);
				continue;
			} else if (!hasTextSection(wbuffer)) {
				printf("Warning - Could not find TEXT section: %s\n", 
					OutputUtil::convertToChar(docId.data()));
				stringPrintEmptyResults(results, docId);
			} else {
				documentDriver->runOnString((const wchar_t*) wbuffer, &results);
			}
		}
		catch (UnrecoverableException &e)
		{ //GG
			cerr << "\n" << e.getMessage () << "\n";
			sprintf_s(error_string, "SERIF ERROR", ERROR);
			sprintf_s(out_bytes, "%d\n%d\n%s", ERROR, strlen(error_string), error_string);
			send(ClientSocket, out_bytes, (int)strlen(out_bytes), 0);
			continue;
		}
		catch (exception &e) {
			cerr << "\nError: " << e.what() << " in SerifSocketServer::main()\n";
			sprintf_s(error_string, "SERIF ERROR: %s", e.what());
			sprintf_s(out_bytes, "%d\n%d\n%s", ERROR, strlen(error_string), error_string);
			send(ClientSocket, out_bytes, (int)strlen(out_bytes), 0);
			continue;
		}

		int wchar_length = ((int)results.length() + 1) * 3;
		char *buffer = new char[wchar_length];
		makeUTF8CharArray(results.c_str(), buffer, wchar_length);

		int size = (int)strlen(buffer);
		sprintf_s(out_bytes, "%d\n%d\n", OK, size);
		send(ClientSocket, out_bytes, (int)strlen(out_bytes), 0);
		send(ClientSocket, buffer, size, 0);

		closeSocket(ClientSocket);
		
	}

	// cleanup
	shutdown(ClientSocket, SD_SEND);
	closeSocket(ClientSocket);
	closeSocket(ListenSocket);
#if defined(WIN32) || defined(WIN64)
	WSACleanup();
#endif
}

size_t get_command (char *buffer, char *command)
{
	size_t index, i;
	size_t size;

	size = strlen (buffer);

	index = 0;

	while ((buffer[index] != ' ') &&
		(index < size))
		index++;

	for (i = 0; i < index; i++)
		command[i] = buffer[i];

	command[index] = '\0';

	return index;
}

bool hasTextSection(wchar_t* document)
{
	std::wstring str(document);
	size_t start_id = str.find(L"<TEXT>");
	size_t end_id = str.find(L"</TEXT>");
    
	if (start_id == str.npos || end_id == str.npos || end_id <= start_id) 
		return false;
	return true;
}

bool hasDocId(wchar_t* document, wstring *docId)
{
	std::wstring str(document);
	size_t start_id = str.find(L"<DOCID>");
	size_t end_id = str.find(L"</DOCID>");
    
	if (start_id == str.npos || end_id == str.npos || end_id <= start_id) 
		return false;

	(*docId) = str.substr(start_id + 7, end_id - start_id - 7).data();
	return true;
}

void stringPrintEmptyResults(wstring &results, wstring &docId)
{
	results.append(L"<?xml version=\"1.0\"?>\n");
	results.append(L"<!DOCTYPE source_file SYSTEM \"atea.v1.1.dtd\">\n");
	results.append(L"<source_file SOURCE=\"UNKNOWN\" TYPE=\"text\" VERSION=\"2.0\" URI=\"");
	results.append(docId.data());
	results.append(L"\">\n");
	results.append(L"  <document DOCID=\"");
	results.append(docId.data());
	results.append(L"\">\n\n");
	results.append(L"  </document>\n</source_file>"); 
}

#if defined(WIN32) || defined(WIN64)
void initializeWinsock() 
{
	WSADATA wsaData;
	// Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        exit(ERROR);
    }
}
#endif

#if defined(WIN32) || defined(WIN64)
void initializeListenSocket(SOCKET &sock, const char *port)
{
	struct addrinfo *result = NULL,
                    hints;
   
    int iResult;
 
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, port, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        exit(ERROR);
    }

    // Create a SOCKET for connecting to server
    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        printf("socket failed: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        exit(ERROR);
    }

    // Setup the TCP listening socket
    iResult = bind( sock, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(sock);
        WSACleanup();
        exit(ERROR);
    }

    freeaddrinfo(result);

    iResult = listen(sock, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        exit(ERROR);
    }
}
#elif defined(UNIX)
void initializeListenSocket(SOCKET &sock, const char *port)
{
	struct sockaddr_in serv_addr;
	int portno = atoi(port);
   
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("socket open failed.\n\n");
		exit(ERROR);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR on binding.\n\n");
		exit(ERROR);
	}

	listen(sock, 5);
}
#endif 


#if defined(WIN32) || defined(WIN64)
void acceptSocket(SOCKET& serv_sock, SOCKET& client_sock) 
{
	client_sock = accept(serv_sock, NULL, NULL);
}
#elif defined (UNIX)
void acceptSocket(SOCKET& serv_sock, SOCKET& client_sock) 
{
	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);
	client_sock = accept(serv_sock, (sockaddr *) &cli_addr, (socklen_t *) &clilen);
	if (client_sock < 0) {
		printf("ERROR on accept.\n\n");
		exit(ERROR);
	}
}
#endif

#if defined(WIN32) || defined(WIN64)	
void closeSocket(SOCKET& sock) { closesocket(sock); }
#elif defined(UNIX)
void closeSocket(SOCKET& sock) { close(sock); }
#endif


// read from the socket up to the first space
int readCommandSize(SOCKET &sock, char *buf, int max_length) {
	int i = 0;
	char recvbuf[2];
	int iResult;
	char c;

	do {
		// grab one character at a time until you see a space
		iResult = recv(sock, recvbuf, 1, 0);

		c = recvbuf[0];
		if (i >= max_length) {
			return 1;
		}
	
		buf[i++] = c;

	} while (c != ' ');
	// write over the final space with a terminator
	buf[i-1] = '\0';

	if (atoi(buf) == 0) return 1;
	return 0;
}

int readFromSocket(SOCKET &sock, char *buf, int num_characters)
{
	int i;
	char recvbuf[2];
	int iResult;
	char c;

	for (i = 0; i < num_characters; i++) {
		iResult = recv(sock, recvbuf, 1, 0);
		if (iResult != 1) break;
		c = recvbuf[0];
		buf[i] = c;
	}
	buf[i] = '\0';
	return 0;
}

wchar_t getNextUTF8Char(char * text, size_t & pos, size_t max_pos) {
	unsigned char c[4];
	unsigned char headMask1 = 0x1f;
	unsigned char headMask2 = 0x0f;
	unsigned char mask = 0x3f;
	if (pos == max_pos)
		return 0x0000;
	c[0] = text[pos];
	pos++;
	int readSize = 0;
	//std::cout << "Default readSize = 0\n";
	// check the lead byte to see how many more we  should read in
	if (c[0] >= 0xe0) {
		//std::cout << "readSize = 2\n";
		readSize = 2;
	}
	else if (c[0] >= 0xc0) {
		//std::cout << "readSize = 1\n";
		readSize = 1;
	}

	if (readSize > 0) {
		if (pos == max_pos) {
			char message[130];
			strcpy_s(message, "Unexpected EOL in char stream! ");
			return 0;
			//throw UnexpectedInputException("SocketsClient::getNextUTF8Char()", message);
		}
		for (int i = 0; i < readSize; i++) {
			c[i+1] = text[pos];
			pos++;
		}
	}
	wchar_t wc = 0x0000;
	// 0xxxxxxx
	if (readSize == 0) {
		wc = c[0];
	}
	// 110xxxxx 10xxxxxx
	else if (readSize == 1)
		wc = ((headMask1 & c[0]) << 6) | (mask & c[1]);
	// 1110xxxx 10xxxxxx 10xxxxxx
	else if (readSize == 2)
		wc = ((headMask2 & c[0]) << 12) | ((mask & c[1]) << 6) | (mask & c[2]);

	return wc;
}

void makeUTF8CharArray(const wchar_t* input, char* output, int max_len)
{
	size_t len = wcslen(input);
	size_t i;
	int pos = 0; 
	unsigned char c[4];
	for(i = 0; i< len; i++){
		// < 7f => 0xxxxxxx
		unsigned char baseMask = 0x80;
		unsigned char lowMask = 0x3f;
		wchar_t ch = input[i];
		if (ch <= 0x007f) {
			c[0] = (char)ch;
			output[pos++] = c[0];
		}
		// < 7ff => 110xxxxx 10xxxxxx
		else if (ch <= 0x07ff) {
			c[1] = (baseMask | (lowMask&((char) ch)));
			c[0] = (0xc0 | ((char)(ch >> 6)));
			output[pos++] = c[0];
			output[pos++] = c[1];

		}
		// < ffff => 1110xxxx 10xxxxxx 10xxxxxx
		else if (ch <= 0xffff) {
			c[2] = (baseMask | (lowMask&((char) ch)));
			c[1] = (baseMask | (lowMask&((char) (ch >> 6))));
			c[0] = (0xe0 | ((char)(ch >> 12)));
			output[pos++] = c[0];
			output[pos++] = c[1];
			output[pos++] = c[2];
		}
		if((pos + 4) > max_len){
			break;
		}
	}
	output[pos++] = '\0';

}
