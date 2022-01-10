// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// Copyright (c) 2006 by BBN Technologies, Inc.
// All Rights Reserved.

/*
This is the interface to the Active Learning functions that QuickLearn
uses. When this executable is called, a socket server starts up and 
is ready to accept commands. A command is invoked by opening up a socket
to the server, sending in a command name and a list of parameters.
Commands have between 0 and 4 parameters.
*/

#include "Generic/common/leak_detection.h" //This must be the first #include

#include "Generic/names/discmodel/PIdFActiveLearning.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

void initializeWinsock();
void initializeListenSocket(SOCKET& sock, const char *port);
wstring getRetVal(bool ok, const char* txt);
void writeToSocket(SOCKET& sock, wstring wstr);
int getCommand(SOCKET& sock, char *command, int max_length);
int getIntParameter(SOCKET &sock);
bool getBoolParameter(SOCKET &sock);
wstring getStringParameter(SOCKET &sock);
int getNumber(SOCKET &sock);
wchar_t getNextUTF8Char(char * text, size_t & pos, size_t max_pos);
void makeUTF8CharArray(const wchar_t* input, char* output, size_t max_len);

int main(int argc, char **argv)
{ 
	PIdFActiveLearning *_trainer = 0;
	initializeWinsock();

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	
	printf ("This is PIdFQuickLearnServer v1.0\n");
	printf ("Copyright (c) 2006 by BBN Technologies, Inc.\n\n");

	if (argc != 2) {
		printf ("PIdFQuickLearnServer.exe is invoked with a single argument - a port number\n");
		return -1;
	}
	initializeListenSocket(ListenSocket, argv[1]);

	bool close = false;
	while (!close) {
		printf ("\nWaiting for a connection...\n");
		ClientSocket = accept(ListenSocket, NULL, NULL);

		char command[100];
		int rv = getCommand(ClientSocket, command, 99);
		if (rv) {
			printf ("Could not get command from socket. Returning error string.\n\n");
			wstring returnString = getRetVal(false, "Could not get command");
			writeToSocket(ClientSocket, returnString);
			continue;
		}
		printf ("Received command: %s\n", command);

		// The function to start the pIdF server
		if (!strcmp(command, "StartPIdFService")) {
			try {
				
				if (_trainer != 0) 
					delete _trainer;
				
				_trainer = _new PIdFActiveLearning();
				writeToSocket(ClientSocket, getRetVal(true, "The service has been started."));
				
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		// The function to stop the pIdF server
		/*else if (!strcmp(command, "StopPIdFService")) {
			try {
				if (_trainer != 0)
					delete _trainer;
				writeToSocket(ClientSocket, getRetVal(true, "The service has been stopped."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}*/

		// The function to initialize a project
		else if (!strcmp(command, "Initialize")) {
			wstring param_path = getStringParameter(ClientSocket);
			
			char *ch_string = new char[param_path.length() + 1];
			size_t num_converted = 0;
			wcstombs_s(&num_converted, ch_string, param_path.length() + 1, param_path.c_str(), param_path.length() + 1);

			try {
				if (_trainer != 0) {
					wstring return_value = _trainer->Initialize(ch_string);
					writeToSocket(ClientSocket, return_value);
				}
				else {
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
				}
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		// The function to read the corpus file
		else if (!strcmp(command, "ReadCorpus")) {
			wstring corpus_file = getStringParameter(ClientSocket);
			char *ch_string = new char[corpus_file.length() + 1];
			size_t num_converted = 0;
			wcstombs_s(&num_converted, ch_string, corpus_file.length() + 1, corpus_file.c_str(), corpus_file.length() + 1);

			// check if file exists
			ifstream inp;
			inp.open(ch_string, ifstream::in);
			inp.close();
			if (inp.fail()) {
				printf ("Could not open corpus file: %s. Returning error string.\n", ch_string);
				writeToSocket(ClientSocket, 
						getRetVal(false, "Could not open corpus file"));
			} else {
				try {
					if (_trainer != 0) {
						wstring return_value = _trainer->ReadCorpus(ch_string);
						writeToSocket(ClientSocket, return_value);
					}
					else
						writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
				}
				catch (UnrecoverableException &e) {
					writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
				}
			}
		}

		// The function to read the corpus file
		else if (!strcmp(command, "ReadDefaultCorpus")) {
			try {
				if (_trainer != 0) {
					wstring return_value = _trainer->ReadCorpus();
					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		// The function to train a model with the annotated sentences
		else if (!strcmp(command, "Train")) {
			//UTF8OutputStream file("C:/temp/out");
			wstring ann_sents = getStringParameter(ClientSocket);
			//file << ann_sents.c_str() << L"\n";
			//file.close();
			
			wstring token_sents = getStringParameter(ClientSocket);
			int epochs = getIntParameter(ClientSocket);
			bool isIncremental = getBoolParameter(ClientSocket);

			try {
				if (_trainer != 0) {
					
					wstring return_value = 
						_trainer->Train(ann_sents.c_str(), ann_sents.length(), 
							            token_sents.c_str(), token_sents.length(), epochs, isIncremental);

					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		else if (!strcmp(command, "AddToTestSet")) {
			wstring ann_sents = getStringParameter(ClientSocket);
			wstring token_sents = getStringParameter(ClientSocket);

			try {
				if (_trainer != 0) {
					
					wstring return_value = 
						_trainer->AddToTestSet(ann_sents.c_str(), ann_sents.length(), 
											   token_sents.c_str(), token_sents.length());

					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}

		}

		// The function saves the training and the model files
		else if (!strcmp(command, "Save")) {
			try {
				if (_trainer != 0) {
					wstring return_value = _trainer->Save();
					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		// The function saves the tokens for the passed sentences
		else if (!strcmp(command, "SaveSentences")) {
			wstring ann_sents = getStringParameter(ClientSocket);
			wstring token_sents = getStringParameter(ClientSocket);
			wstring tokens_file = getStringParameter(ClientSocket);

			try {
				if (_trainer != 0) {
					wstring return_value = 
						_trainer->SaveSentences(ann_sents.c_str(), ann_sents.length(), 
											    token_sents.c_str(), token_sents.length(), 
												tokens_file.c_str());
					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		// The function returns the next sentence id in the corpus file
		else if (!strcmp(command, "GetCorpusPointer")) {
			try {
				if (_trainer != 0) {
					wstring return_value = _trainer->GetCorpusPointer();
					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		// The function selects and returns new sentences to annotate.
		else if (!strcmp(command, "SelectSentences")) {
			int training_pool_size = getIntParameter(ClientSocket);
			int num_to_select = getIntParameter(ClientSocket);
			int context_size = getIntParameter(ClientSocket);
			int min_positive_cases = getIntParameter(ClientSocket);

			//printf("Got params: %d %d %d %d\n", training_pool_size, num_to_select, context_size, min_positive_cases);


			try {
				if (_trainer != 0) {
					wstring return_value = 
						_trainer->SelectSentences(training_pool_size, num_to_select, context_size, min_positive_cases);
					writeToSocket(ClientSocket, return_value);
					//wprintf(L"returned: %s\n", return_value.c_str()); 
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		else if (!strcmp(command, "GetNextSentences")) {
			int num_to_select = getIntParameter(ClientSocket);
			int context_size = getIntParameter(ClientSocket);

			//printf("Got params: %d %d %d %d\n", training_pool_size, num_to_select, context_size, min_positive_cases);

			try {
				if (_trainer != 0) {
					wstring return_value = 
						_trainer->GetNextSentences(num_to_select, context_size);
					writeToSocket(ClientSocket, return_value);
					//wprintf(L"returned: %s\n", return_value.c_str()); 
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		// The function closes the current project
		else if (!strcmp(command, "Close")) {

			try {
				if (_trainer != 0) {
					//printf("before close call\n");
					wstring return_value = _trainer->Close();
					//printf("after close call\n");
					writeToSocket(ClientSocket, return_value);
					//printf("after write to socket\n");
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}

		}

		// The function changes the corpus file to a new one
		else if (!strcmp(command, "ChangeCorpus")) {
			wstring corpus_path = getStringParameter(ClientSocket);

			try {
				if (_trainer != 0) {
					char * buffer = _new char[1024];
					wcstombs(buffer, corpus_path.c_str(), 1024);
					wstring return_value = _trainer->ChangeCorpus(buffer);
					delete buffer;
					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		// The function returns model-decoded sentences
		else if (!strcmp(command, "DecodeTraining")) {
			wstring sentences = getStringParameter(ClientSocket);	

			try {
				if (_trainer != 0) {
					wstring return_value = _trainer->DecodeTraining(sentences.c_str(), sentences.length());
					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		else if (!strcmp(command, "DecodeTestSet")) {
			wstring sentences = getStringParameter(ClientSocket);	

			try {
				if (_trainer != 0) {
					wstring return_value = _trainer->DecodeTestSet(sentences.c_str(), sentences.length());
					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		else if (!strcmp(command, "DecodeFromCorpus")) {
			wstring sentences = getStringParameter(ClientSocket);	

			try {
				if (_trainer != 0) {
					wstring return_value = _trainer->DecodeFromCorpus(sentences.c_str(), sentences.length());
					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		}

		// The function returns model-decoded sentences
		else if (!strcmp(command, "DecodeFile")) {
			wstring input_file = getStringParameter(ClientSocket);

			try {
				if (_trainer != 0) {
					//wprintf(L"Decoding file: %s\n", input_file.c_str());
					wstring return_value = _trainer->DecodeFile(input_file.c_str());
					//wprintf(L"Results: %s\n", return_value.c_str());
					writeToSocket(ClientSocket, return_value);
				}
				else
					writeToSocket(ClientSocket, 
						getRetVal(false, "The PIdF service has not been started."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
		} 
		else if (!strcmp(command, "ShutdownPIdFServer")) {
			try {
				if (_trainer != 0)
					delete _trainer;
				writeToSocket(ClientSocket, getRetVal(true, "The service has been shutdown."));
			}
			catch (UnrecoverableException &e) {
				writeToSocket(ClientSocket, getRetVal(false, e.getMessage()));
			}
			close = true;
		}
		else {
			printf ("Unknown command: %s. Returning error string.\n\n", command);
			wstring returnString = getRetVal(false, "Unknown command");
			writeToSocket(ClientSocket, returnString);
		}

		closesocket(ClientSocket);
	}
}

int getIntParameter(SOCKET &sock) {
	wstring str = getStringParameter(sock);
	return _wtoi(str.c_str());
}

bool getBoolParameter(SOCKET &sock) {
	wstring str = getStringParameter(sock);
	if (!wcscmp(str.c_str(), L"true")) {
		return true;
	}
	else {
		return false;
	}
}
wstring getStringParameter(SOCKET &sock) {
	int length = getNumber(sock);
	if (length < 0) {
		// error
		return L"";
	}

	char *param = new char[length + 1];
	char recvbuf[2];
	int iResult;
	int i;

	for (i = 0; i < length; i++) {
		iResult = recv(sock, recvbuf, 1, 0);
		param[i] = recvbuf[0];
	}
	// get empty space
	iResult = recv(sock, recvbuf, 1, 0);
	param[i] = '\0';
	//printf("Found param: %s\n", param);

	size_t pos = 0;
	int utf8_pointer = 0;
	wchar_t *output = new wchar_t[length + 1];
	while ((int)pos < length) {
		output[utf8_pointer] = getNextUTF8Char(param, pos, length);
		utf8_pointer++;
	}
	output[utf8_pointer] = L'\0';
	delete param;

	wstring rv(output);
	delete [] output;

	return rv;
}

int getNumber(SOCKET &sock)
{
	char recvbuf[2];
	char buffer[10];
	int iResult;
	char c;
	int i = 0;

	do {
		// grab one character at a time until you see a space
		iResult = recv(sock, recvbuf, 1, 0);

		c = recvbuf[0];
		if (i >= 9) {
			return -1;
		}

		buffer[i++] = c;

	} while (c != ' ');

	buffer[i-1] = '\0';
	return atoi(buffer);
}

int getCommand(SOCKET& sock, char *command, int max_length) 
{
	char recvbuf[2];
	int iResult;
	char c;
	int i = 0;

	do {
		// grab one character at a time until you see a space
		iResult = recv(sock, recvbuf, 1, 0);

		c = recvbuf[0];
		if (i >= max_length) {
			return 1;
		}

		command[i++] = c;

	} while (c != ' ');
	command[i-1] = '\0';
	return 0;
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

wstring getRetVal(bool ok, const char* txt){
	wstring retval(L"<RETURN>\n");
	if(strlen(txt) >999){
		char errbuffer[1000];
		strcpy_s(errbuffer, "Invalid Conversion, String too long\nFirst 900 characters: ");
		strncat_s(errbuffer, txt, 900);
		return getRetVal(false, errbuffer);
	}
	wchar_t conversionbuffer[1000];
	size_t num_converted;
	mbstowcs_s(&num_converted, conversionbuffer, txt, 1000);

	if(ok){
		retval +=L"\t<RETURN_CODE>OK</RETURN_CODE>\n";
	}
	else{
		retval += L"\t<RETURN_CODE>ERROR</RETURN_CODE>\n";
	}
	retval +=L"\t<RETURN_VALUE>";

	retval += conversionbuffer;
	retval += L"</RETURN_VALUE>\n";
	retval += L"</RETURN>";

	return retval;
}

void writeToSocket(SOCKET& sock, wstring wstr)
{
	size_t wchar_length = (wstr.length() + 1) * 3;
	char *buffer = new char[wchar_length];
	makeUTF8CharArray(wstr.c_str(), buffer, wchar_length);
	
	char char_length[10];
	_itoa_s((int)(strlen(buffer)), char_length, 9, 10); 

	send(sock, char_length, (int)(strlen(char_length)), 0);
	send(sock, " ", 1, 0);
	send(sock, buffer, (int)(strlen(buffer)), 0);
	delete [] buffer;
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
	// check the lead byte to see how many more we should read in
	if (c[0] >= 0xe0)
		readSize = 2;
	else if (c[0] >= 0xc0)
		readSize = 1;

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

void makeUTF8CharArray(const wchar_t* input, char* output, size_t max_len)
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
