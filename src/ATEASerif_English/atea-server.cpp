// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

// Copyright (c) 2006 by BBN Technologies, Inc.
// All Rights Reserved.

// ATEA server for Serif. 

#include "common/leak_detection.h" //This must be the first #include

#include "common/version.h"
#include "English/common/en_version.h"

#include "driver/DocumentDriver.h"
#include "driver/SessionProgram.h"
#include "common/ParamReader.h"
#include "ATEASerif_generic/results/ATEAResultCollector.h"
#include "common/HeapChecker.h"
#include "common/UnrecoverableException.h"
#include "common/OutputUtil.h"
#include "state/objectIDTable.h"
#include "English/EnglishModule.h"

#include "ATEASerif_generic/text/HandleXMLCharacters.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <tchar.h>

#include <iostream>
#include <string>

#include <tchar.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


using namespace std;

#define OK 1
#define ERROR 0

#define UPCASE_DOCUMENT false

int get_command (char *buffer, char *command);
bool hasDocId(wchar_t* document, wstring *docId);
bool hasTextSection(wchar_t* document);
void stringPrintEmptyResults(wstring &results, wstring &docId);
void initializeWinsock();
void initializeListenSocket(SOCKET& sock, const char *port);
int readCommandSize(SOCKET& sock, char *buf, int max_length);
int readFromSocket(SOCKET& sock, char *buf, int num_characters);

void usage ()
{
	cerr << "Usage: atea-serif-server.exe <paramfile> <port>\n"
		<< "\tparamfile - Serif parameters file\n"
		<< "\tport - port number for server\n";
}

// This is the entry point for this application
int main(int argc, char **argv)
{ 
	setup_English();

	char *buffer = 0;
	wchar_t *wbuffer = 0;
	char *doc;
	char command[256];
	char error_string[1000];
	char out_bytes[1000];
	wstring results;

	SessionProgram *sessionProgram;
	ATEAResultCollector *resultCollector;
	DocumentDriver *documentDriver;

	cout << "This is ATEA Serif Server, version 4.3\n"
		 << "Copyright (c) 2011 by Raytheon BBN Technologies Corp.\n"
		 << "Serif version: " << SerifVersion::getVersionString() << "\n"
		 << "Serif Language Module: " << SerifVersion::getSerifLanguage().toString() << "\n"
		 << endl;

	if (argc != 3)
	{ 
		usage ();
		return -1;
	} 

	initializeWinsock();

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	initializeListenSocket(ListenSocket, argv[2]);

	try {
		ParamReader::readParamFile (argv[1]);
		sessionProgram = new SessionProgram;
		resultCollector = new ATEAResultCollector(ATEAResultCollector::APF2005);
		documentDriver = new DocumentDriver (sessionProgram, resultCollector);
	} catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage () << "\n";
		HeapChecker::checkHeap ("main(); About to exit due to error");

		return -1;
	} 

	while (1) {

		printf ("Waiting for a connection...\n");
		ClientSocket = accept(ListenSocket, NULL, NULL);

		// grab everything up to the first space, this is the length of the command
		char command_size[10];
		int rv = readCommandSize(ClientSocket, command_size, 9);
		if (rv) {
			printf ("Bad command size\n");
			strcpy_s(error_string, "Could not read command size from beginning of stream");
			sprintf_s(out_bytes, "%d\n%d\n%s", ERROR, strlen(error_string), error_string);
			send(ClientSocket, out_bytes, strlen(out_bytes), 0);
			continue;
		}

		delete [] buffer;
		buffer = new char[atoi(command_size) + 1];

		readFromSocket(ClientSocket, buffer, atoi(command_size));
		int offset = get_command(buffer, command);

		if (!strcmp(command, "SHUTDOWN")) {
			printf ("Shutdown by client.\n\n");
			sprintf_s(out_bytes, "%d\n0\n", OK);
			send(ClientSocket, out_bytes, strlen(out_bytes), 0);
			break;
		}
		
		if (!strcmp(command, "DISCONNECT")) {
			printf ("Disconnect command received.\n\n");
			sprintf_s(out_bytes, "%d\n0\n", OK);
			send(ClientSocket, out_bytes, strlen(out_bytes), 0);
			closesocket(ClientSocket);
			continue;
		}
		
		if (strcmp(command, "ANALYZE")) {
			printf ("Unknown command: %s\n\n", command);
			sprintf_s(error_string, "Unknown command: %s", command);
			sprintf_s(out_bytes, "%d\n%d\n%s", ERROR, strlen(error_string), error_string);
			send(ClientSocket, out_bytes, strlen(out_bytes), 0);
			continue;
		}

		// we have an ANALYZE command
		doc = buffer + offset + 1;
		handleXMLcharacters(doc);
		int size = strlen(doc) + 1;

		delete [] wbuffer;
		wbuffer = new wchar_t[size];

		for (int i = 0; i < size; i++) {
			if (UPCASE_DOCUMENT)
				doc[i] = toupper(doc[i]);
			wbuffer[i] = (wchar_t) doc[i];
		}

		results = L"";
		try
		{ 
			std::wstring docId(L"NONE_FOUND");

			if (!hasDocId(wbuffer, &docId)) {
				printf("Error - Could not find well-formed DOCID section\n");
				sprintf_s(error_string, "SERIF ERROR: No DOCID found\n", command);
				sprintf_s(out_bytes, "%d\n%d\n%s", ERROR, strlen(error_string), error_string);
				send(ClientSocket, out_bytes, strlen(out_bytes), 0);
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
			HeapChecker::checkHeap ("main(); About to terminate client session due to error");

			sprintf_s(error_string, e.getMessage());
			sprintf_s(out_bytes, "%d\n%d\n%s", ERROR, strlen(error_string), error_string);
			send(ClientSocket, out_bytes, strlen(out_bytes), 0);
			continue;
		} 

		const wchar_t *result_string = results.c_str();
		size = wcslen (result_string);
		sprintf_s(out_bytes, "%d\n%d\n", OK, size);
		send(ClientSocket, out_bytes, strlen(out_bytes), 0);

		char bytes[2];
		for (int i = 0; i < size; i++)
		{ 
			bytes[0] = (char) result_string[i];
			send(ClientSocket, bytes, 1, 0);
		} 
		closesocket(ClientSocket);
	}

	// cleanup
	shutdown(ClientSocket, SD_SEND);
	closesocket(ClientSocket);
	closesocket(ListenSocket);
	WSACleanup();
}

int get_command (char *buffer, char *command)
{
	int index, i;
	int size;

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
	int start_id = str.find(L"<TEXT>");
	int end_id = str.find(L"</TEXT>");
    
	if (start_id == str.npos || end_id == str.npos || end_id <= start_id) 
		return false;
	return true;
}

bool hasDocId(wchar_t* document, wstring *docId)
{
	std::wstring str(document);
	int start_id = str.find(L"<DOCID>");
	int end_id = str.find(L"</DOCID>");
    
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

void initializeWinsock() 
{
	WSADATA wsaData;
	// Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        exit(1);
    }
}

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
        exit(1);
    }

    // Create a SOCKET for connecting to server
    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        printf("socket failed: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        exit(1);
    }

    // Setup the TCP listening socket
    iResult = bind( sock, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(sock);
        WSACleanup();
        exit(1);
    }

    freeaddrinfo(result);

    iResult = listen(sock, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        exit(1);
    }
}

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
