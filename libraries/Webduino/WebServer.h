/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil;  c-file-style: "k&r"; c-basic-offset: 2; -*-

 Webduino, a simple Arduino web server
 Copyright 2009-2012 Ben Combee, Ran Talbott, Christopher Lee, Martin Lormes

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#ifndef WEBDUINO_H_
#define WEBDUINO_H_

#include <string.h>
#include <stdlib.h>

#include <EthernetClient.h>
#include <EthernetServer.h>
#include <aJSON.h>

/********************************************************************
 * CONFIGURATION
 ********************************************************************/

#define WEBDUINO_VERSION 1007
#define WEBDUINO_VERSION_STRING "1.7"

#if WEBDUINO_SUPRESS_SERVER_HEADER
#define WEBDUINO_SERVER_HEADER ""
#else
#define WEBDUINO_SERVER_HEADER "Server: Webduino/" WEBDUINO_VERSION_STRING CRLF
#endif

// standard END-OF-LINE marker in HTTP
#define CRLF "\r\n"

// If processConnection is called without a buffer, it allocates one
// of 32 bytes
#define WEBDUINO_DEFAULT_REQUEST_LENGTH 64

// How long to wait before considering a connection as dead when
// reading the HTTP request.  Used to avoid DOS attacks.
#ifndef WEBDUINO_READ_TIMEOUT_IN_MS
#define WEBDUINO_READ_TIMEOUT_IN_MS 1000
#endif

#ifndef WEBDUINO_FAIL_MESSAGE
#define WEBDUINO_FAIL_MESSAGE "<h1>EPIC FAIL</h1>"
#endif

#ifndef WEBDUINO_AUTH_REALM
#define WEBDUINO_AUTH_REALM "please login first"
#endif // #ifndef WEBDUINO_AUTH_REALM
#ifndef WEBDUINO_AUTH_MESSAGE
#define WEBDUINO_AUTH_MESSAGE "<h1>401 Unauthorized</h1>"
#endif // #ifndef WEBDUINO_AUTH_MESSAGE
#ifndef WEBDUINO_SERVER_ERROR_MESSAGE
#define WEBDUINO_SERVER_ERROR_MESSAGE "<h1>500 Internal Server Error</h1>"
#endif // WEBDUINO_SERVER_ERROR_MESSAGE
// add '#define WEBDUINO_FAVICON_DATA ""' to your application
// before including WebServer.h to send a null file as the favicon.ico file
// otherwise this defaults to a 16x16 px black diode on blue ground
// (or include your own icon if you like)
#ifndef WEBDUINO_FAVICON_DATA
#define WEBDUINO_FAVICON_DATA { 0x42,0x4D,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x12,0x0B,0x00,0x00,0x12,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0xA3,0xFC,0x00,0x00,0x00 }
#endif // #ifndef WEBDUINO_FAVICON_DATA
// add "#define WEBDUINO_SERIAL_DEBUGGING 1" to your application
// before including WebServer.h to have incoming requests logged to
// the serial port.
#ifndef WEBDUINO_SERIAL_DEBUGGING
#define WEBDUINO_SERIAL_DEBUGGING 0
#endif
#if WEBDUINO_SERIAL_DEBUGGING
#include <HardwareSerial.h>
#endif

// declared in wiring.h
extern "C" unsigned long millis(void);

// declare a static string
#define P(name)   static const prog_uchar name[] PROGMEM

// returns the number of elements in the array
#define SIZE(array) (sizeof(array) / sizeof(*array))

/********************************************************************
 * DECLARATIONS
 ********************************************************************/

/* Return codes from nextURLparam.  NOTE: URLPARAM_EOS is returned
 * when you call nextURLparam AFTER the last parameter is read.  The
 * last actual parameter gets an "OK" return code. */

typedef enum URLPARAM_RESULT {
	URLPARAM_OK,
	URLPARAM_NAME_OFLO,
	URLPARAM_VALUE_OFLO,
	URLPARAM_BOTH_OFLO,
	URLPARAM_EOS // No params left
};




class WebServer: public Print {
public:
	enum ConnectionType {
		INVALID, GET, HEAD, POST, PUT, DELETE, PATCH
	};
	typedef void BeforeResponce(WebServer& server, ConnectionType type,
			char* url_tail, bool tail_complete, aJsonObject** collection,
			aJsonObject** model);
	typedef void AfterResponce(ConnectionType type, aJsonObject** collection,
			aJsonObject** model);
	struct Binding {
		aJsonObject** collection;
		char* uri;
		long curId;
		BeforeResponce* beforeResponse;
		AfterResponce* afterResponce;
		boolean persistent;
	};
	typedef void Command(WebServer& server, ConnectionType type, char* url_tail,
			bool tail_complete);
	WebServer(const char* urlPrefix = "", int port = 80);
	void begin();
	void processConnection();
	void processConnection(char* buff, int* bufflen);
	void setDefaultCommand(Command* cmd);
	void setFailureCommand(Command* cmd);
	void addCommand(const char* verb, Command* cmd);
	void printCRLF();
	void printP(const prog_uchar* str);

	void printP(const prog_char* str) {
		printP((prog_uchar*) ((((str)))));
	}

	void writeP(const prog_uchar* data, size_t length);
	int read();
	void push(int ch);
	bool expect(const char* expectedStr);
	bool readInt(int& number);
	void readHeader(char* value, int valueLen);
	bool readPOSTparam(char* name, int nameLen, char* value, int valueLen);
	URLPARAM_RESULT nextURLparam(char** tail, char* name, int nameLen,
			char* value, int valueLen);
	bool checkCredentials(const char authCredentials[45]);
	void httpFail();
	void httpUnauthorized();
	void httpServerError();
	void httpSuccess(const char* contentType = "text/html; charset=utf-8",
			const char* extraHeaders = NULL);
	void httpCreated(const char* contentType = "text/html; charset=utf-8",
			const char* extraHeaders = NULL);
	void httpNoContent();
	void httpNotAllowed();
	void httpSeeOther(const char* otherURL);
	virtual size_t write(uint8_t);
	virtual size_t write(const char* str);
	virtual size_t write(const uint8_t* buffer, size_t size);
	size_t write(const char* data, size_t length);
	void addJSONBinding(char* uri, aJsonObject** jsonObject);
	void setBeforeRequest(char* uri, BeforeResponce beforeResponce);
	void setAfterRequest(char* uri, AfterResponce beforeResponce);

	void setPreventDefault(boolean preventDefault) {
		this->preventDefault = preventDefault;
	}

private:
	EthernetServer m_server;
	EthernetClient m_client;
	const char* m_urlPrefix;
	unsigned char m_pushback[32];
	char m_pushbackDepth;
	int m_contentLength;
	char m_authCredentials[51];
	bool m_readingContent;
	Command* m_failureCmd;
	Command* m_defaultCmd;
	struct CommandMap {
		const char* verb;
		Command* cmd;
	} m_commands[5];
	char m_cmdCount;
	void reset();
	void getRequest(WebServer::ConnectionType& type, char* request,
			int* length);
	bool dispatchCommand(ConnectionType requestType, char* verb,
			bool tail_complete);
	void processRestReguest(ConnectionType type, char* tail, bool tail_complete,
			Binding& binding);
	void processHeaders();
	static void defaultFailCmd(WebServer& server, ConnectionType type,
			char* url_tail, bool tail_complete);
	void noRobots(ConnectionType type);
	void favicon(ConnectionType type);


	int bindingsLenght;
	int bindingsMaxLenght;
	Binding bindings[10];

	char* uriBuffer;

	aJsonObject **currentModel;
	aJsonObject **currentCollection;

	AfterResponce* finalCommand;

	boolean preventDefault;

};

#endif
