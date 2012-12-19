#include <arduino.h>
#include <webserver.h>
#include <aJSON.h>

WebServer::WebServer(const char *urlPrefix, int port) :
		m_server(port), m_client(255), m_urlPrefix(urlPrefix), m_pushbackDepth(0), m_cmdCount(0), m_contentLength(0), m_failureCmd(
				&defaultFailCmd), m_defaultCmd(&defaultFailCmd) {

	this->bindingsLenght = 0;
	this->bindingsMaxLenght = 10;
	this->preventDefault = false;

}

void WebServer::begin() {
	m_server.begin();
}

void WebServer::setDefaultCommand(Command *cmd) {
	m_defaultCmd = cmd;
}

void WebServer::setFailureCommand(Command *cmd) {
	m_failureCmd = cmd;
}

void WebServer::addCommand(const char *verb, Command *cmd) {
	if (m_cmdCount < SIZE(m_commands)) {
		m_commands[m_cmdCount].verb = verb;
		m_commands[m_cmdCount++].cmd = cmd;
	}
}

size_t WebServer::write(uint8_t ch) {
	return m_client.write(ch);
}

size_t WebServer::write(const char *str) {
	return m_client.write(str);
}

size_t WebServer::write(const uint8_t *buffer, size_t size) {
	return m_client.write(buffer, size);
}

size_t WebServer::write(const char *buffer, size_t length) {
	return m_client.write((const uint8_t *) buffer, length);
}

void WebServer::writeP(const prog_uchar *data, size_t length) {
	// copy data out of program memory into local storage, write out in
	// chunks of 32 bytes to avoid extra short TCP/IP packets
	uint8_t buffer[32];
	size_t bufferEnd = 0;

	while (length--) {
		if (bufferEnd == 32) {
			m_client.write(buffer, 32);
			bufferEnd = 0;
		}

		buffer[bufferEnd++] = pgm_read_byte(data++);
	}

	if (bufferEnd > 0)
		m_client.write(buffer, bufferEnd);
}

void WebServer::printP(const prog_uchar *str) {
	// copy data out of program memory into local storage, write out in
	// chunks of 32 bytes to avoid extra short TCP/IP packets
	uint8_t buffer[32];
	size_t bufferEnd = 0;

	while (buffer[bufferEnd++] = pgm_read_byte(str++)) {
		if (bufferEnd == 32) {
			m_client.write(buffer, 32);
			bufferEnd = 0;
		}
	}

	// write out everything left but trailing NUL
	if (bufferEnd > 1)
		m_client.write(buffer, bufferEnd - 1);
}

void WebServer::printCRLF() {
	m_client.write((const uint8_t *) "\r\n", 2);
}

bool WebServer::dispatchCommand(ConnectionType requestType, char *verb, bool tail_complete) {
	// if there is no URL, i.e. we have a prefix and it's requested without a
	// trailing slash or if the URL is just the slash
	if ((verb[0] == 0) || ((verb[0] == '/') && (verb[1] == 0))) {
		m_defaultCmd(*this, requestType, "", tail_complete);
		return true;
	}
	// if the URL is just a slash followed by a question mark
	// we're looking at the default command with GET parameters passed
	if ((verb[0] == '/') && (verb[1] == '?')) {
		verb += 2; // skip over the "/?" part of the url
		m_defaultCmd(*this, requestType, verb, tail_complete);
		return true;
	}
	// We now know that the URL contains at least one character.  And,
	// if the first character is a slash,  there's more after it.
	if (verb[0] == '/') {
		char i;
		char *qm_loc;
		int verb_len;
		int qm_offset;
		// Skip over the leading "/",  because it makes the code more
		// efficient and easier to understand.
		verb++;

		// Look for a "/" separating the filename part of the URL from the
		// id.  If it's not there, compare to the whole URL.
		qm_loc = strchr(verb, '/');
		verb_len = (qm_loc == NULL) ? strlen(verb) : (qm_loc - verb);
		qm_offset = (qm_loc == NULL) ? 0 : 1;

		for (i = 0; i < this->bindingsLenght; ++i) {
			if ((verb_len == strlen(bindings[i].uri)) && (strncmp(verb, bindings[i].uri, verb_len) == 0)) {
				// Skip over the "verb" part of the URL (and the question
				// mark, if present) when passing it to the "action" routine

				processRestReguest(requestType, verb + verb_len + qm_offset, tail_complete, bindings[i]);
				return true;
			}
		}

		// Look for a "?" separating the filename part of the URL from the
		// parameters.  If it's not there, compare to the whole URL.
		qm_loc = strchr(verb, '?');
		verb_len = (qm_loc == NULL) ? strlen(verb) : (qm_loc - verb);
		qm_offset = (qm_loc == NULL) ? 0 : 1;

		for (i = 0; i < m_cmdCount; ++i) {
			if ((verb_len == strlen(m_commands[i].verb)) && (strncmp(verb, m_commands[i].verb, verb_len) == 0)) {
				// Skip over the "verb" part of the URL (and the question
				// mark, if present) when passing it to the "action" routine
				m_commands[i].cmd(*this, requestType, verb + verb_len + qm_offset, tail_complete);
				return true;
			}
		}
	}
	return false;
}

// processConnection with a default buffer
void WebServer::processConnection() {
	char request[WEBDUINO_DEFAULT_REQUEST_LENGTH];
	int request_len = WEBDUINO_DEFAULT_REQUEST_LENGTH;
	processConnection(request, &request_len);
}

void WebServer::processConnection(char *buff, int *bufflen) {
	int urlPrefixLen = strlen(m_urlPrefix);

	m_client = m_server.available();

	if (m_client) {
		m_readingContent = false;
		buff[0] = 0;
		ConnectionType requestType = INVALID;
		getRequest(requestType, buff, bufflen);

		// don't even look further at invalid requests.
		// this is done to prevent Webduino from hanging
		// - when there are illegal requests,
		// - when someone contacts it through telnet rather than proper HTTP,
		// - etc.
		if (requestType != INVALID) {
			processHeaders();

			if (strcmp(buff, "/robots.txt") == 0) {
				noRobots(requestType);
			} else if (strcmp(buff, "/favicon.ico") == 0) {
				favicon(requestType);
			}
		}
		if (requestType == INVALID || strncmp(buff, m_urlPrefix, urlPrefixLen) != 0
				|| !dispatchCommand(requestType, buff + urlPrefixLen, (*bufflen) >= 0)) {
			m_failureCmd(*this, requestType, buff, (*bufflen) >= 0);
		}
		reset();
		if(this->finalCommand != NULL){
			this->finalCommand(requestType, this->currentCollection, this->currentModel);
			this->finalCommand = NULL;
		}

	}
}

bool WebServer::checkCredentials(const char authCredentials[45]) {
	char basic[7] = "Basic ";
	if ((0 == strncmp(m_authCredentials, basic, 6)) && (0 == strcmp(authCredentials, m_authCredentials + 6)))
		return true;
	return false;
}

void WebServer::httpFail() {
	P(failMsg) = "HTTP/1.0 400 Bad Request"
	CRLF
	WEBDUINO_SERVER_HEADER
	"Content-Type: text/html"
	CRLF
	CRLF WEBDUINO_FAIL_MESSAGE;

	printP (failMsg);
}

void WebServer::defaultFailCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	server.httpFail();
}

void WebServer::noRobots(ConnectionType type) {
	httpSuccess("text/plain");
	if (type != HEAD) {
		P(allowNoneMsg) = "User-agent: *"
		CRLF
		"Disallow: /"
		CRLF;
		printP (allowNoneMsg);
	}
}

void WebServer::favicon(ConnectionType type) {
	httpSuccess("image/x-icon", "Cache-Control: max-age=31536000\r\n");
	if (type != HEAD) {
		P (faviconIco) = WEBDUINO_FAVICON_DATA;
		writeP(faviconIco, sizeof(faviconIco));
	}
}

void WebServer::httpNotAllowed() {
	P(failMsg) = "HTTP/1.0 405 Method not allowed"
	CRLF
	WEBDUINO_SERVER_HEADER
	"Content-Type: text/html";
	printP (failMsg);
	printCRLF();
	print("Allow: GET");
	printCRLF();

	printP (failMsg);
}

void WebServer::httpUnauthorized() {
	P(failMsg) = "HTTP/1.0 401 Authorization Required"
	CRLF
	WEBDUINO_SERVER_HEADER
	"Content-Type: text/html"
	CRLF
	"WWW-Authenticate: Basic realm=\""
	WEBDUINO_AUTH_REALM
	"\""
	CRLF
	CRLF WEBDUINO_AUTH_MESSAGE;

	printP (failMsg);
}

void WebServer::httpServerError() {
	P(failMsg) = "HTTP/1.0 500 Internal Server Error"
	CRLF
	WEBDUINO_SERVER_HEADER
	"Content-Type: text/html"
	CRLF
	CRLF WEBDUINO_SERVER_ERROR_MESSAGE;

	printP (failMsg);
}

void WebServer::httpSuccess(const char *contentType, const char *extraHeaders) {
	P(successMsg1) = "HTTP/1.0 200 OK"
	CRLF
	WEBDUINO_SERVER_HEADER
	"Access-Control-Allow-Origin: *"
	CRLF
	"Content-Type: ";

	printP (successMsg1);
	print(contentType);
	printCRLF();
	if (extraHeaders)
	print(extraHeaders);
	printCRLF();
}

void WebServer::httpNoContent() {
	P(noContentMsg) =
	"HTTP/1.0 204 NO CONTENT" CRLF
	WEBDUINO_SERVER_HEADER
	CRLF
	CRLF;

	printP(noContentMsg);
}

void WebServer::httpCreated(const char *contentType, const char *extraHeaders) {
	P(successMsg1) = "HTTP/1.0 201 CREATED"
	CRLF
	WEBDUINO_SERVER_HEADER
	"Access-Control-Allow-Origin: *"
	CRLF
	"Content-Type: ";

	printP (successMsg1);
	print(contentType);
	printCRLF();
	if (extraHeaders)
	print(extraHeaders);
	printCRLF();
}

void WebServer::httpSeeOther(const char *otherURL) {
	P(seeOtherMsg) = "HTTP/1.0 303 See Other"
	CRLF
	WEBDUINO_SERVER_HEADER
	"Location: ";

	printP (seeOtherMsg);
	print(otherURL);
	printCRLF();
	printCRLF();
}

int WebServer::read() {
	if (m_client == NULL)
		return -1;

	if (m_pushbackDepth == 0) {
		unsigned long timeoutTime = millis() + WEBDUINO_READ_TIMEOUT_IN_MS;

		while (m_client.connected()) {
			// stop reading the socket early if we get to content-length
			// characters in the POST.  This is because some clients leave
			// the socket open because they assume HTTP keep-alive.
			if (m_readingContent) {
				if (m_contentLength == 0) {
					return -1;
				}
			}

			int ch = m_client.read();

			// if we get a character, return it, otherwise continue in while
			// loop, checking connection status
			if (ch != -1) {
				// count character against content-length
				if (m_readingContent) {
					--m_contentLength;
				}

				return ch;
			} else {
				unsigned long now = millis();
				if (now > timeoutTime) {
					// connection timed out, destroy client, return EOF

					reset();
					return -1;
				}
			}
		}

		// connection lost, return EOF
		return -1;
	} else
		return m_pushback[--m_pushbackDepth];
}

void WebServer::push(int ch) {
	// don't allow pushing EOF
	if (ch == -1)
		return;

	m_pushback[m_pushbackDepth++] = ch;
	// can't raise error here, so just replace last char over and over
	if (m_pushbackDepth == SIZE(m_pushback))
		m_pushbackDepth = SIZE(m_pushback) - 1;
}

void WebServer::reset() {
	m_pushbackDepth = 0;
	m_client.flush();
	m_client.stop();
}

bool WebServer::expect(const char *str) {
	const char *curr = str;
	while (*curr != 0) {
		int ch = read();
		if (ch != *curr++) {
			// push back ch and the characters we accepted
			push(ch);
			while (--curr != str)
				push(curr[-1]);
			return false;
		}
	}
	return true;
}

bool WebServer::readInt(int &number) {
	bool negate = false;
	bool gotNumber = false;
	int ch;
	number = 0;

	// absorb whitespace
	do {
		ch = read();
	} while (ch == ' ' || ch == '\t');

	// check for leading minus sign
	if (ch == '-') {
		negate = true;
		ch = read();
	}

	// read digits to update number, exit when we find non-digit
	while (ch >= '0' && ch <= '9') {
		gotNumber = true;
		number = number * 10 + ch - '0';
		ch = read();
	}

	push(ch);
	if (negate)
		number = -number;
	return gotNumber;
}

void WebServer::readHeader(char *value, int valueLen) {
	int ch;
	memset(value, 0, valueLen);
	--valueLen;

	// absorb whitespace
	do {
		ch = read();
	} while (ch == ' ' || ch == '\t');

	// read rest of line
	do {
		if (valueLen > 1) {
			*value++ = ch;
			--valueLen;

		}
		ch = read();
	} while (ch != '\r');
	push(ch);
}

bool WebServer::readPOSTparam(char *name, int nameLen, char *value, int valueLen) {
	// assume name is at current place in stream
	int ch;
	// to not to miss the last parameter
	bool foundSomething = false;

	// clear out name and value so they'll be NUL terminated
	memset(name, 0, nameLen);
	memset(value, 0, valueLen);

	// decrement length so we don't write into NUL terminator
	--nameLen;
	--valueLen;

	while ((ch = read()) != -1) {
		foundSomething = true;
		if (ch == '+') {
			ch = ' ';
		} else if (ch == '=') {
			/* that's end of name, so switch to storing in value */
			nameLen = 0;
			continue;
		} else if (ch == '&') {
			/* that's end of pair, go away */
			return true;
		} else if (ch == '%') {
			/* handle URL encoded characters by converting back to original form */
			int ch1 = read();
			int ch2 = read();
			if (ch1 == -1 || ch2 == -1)
				return false;
			char hex[3] = { ch1, ch2, 0 };
			ch = strtoul(hex, NULL, 16);
		}

		// output the new character into the appropriate buffer or drop it if
		// there's no room in either one.  This code will malfunction in the
		// case where the parameter name is too long to fit into the name buffer,
		// but in that case, it will just overflow into the value buffer so
		// there's no harm.
		if (nameLen > 0) {
			*name++ = ch;
			--nameLen;
		} else if (valueLen > 0) {
			*value++ = ch;
			--valueLen;
		}
	}

	if (foundSomething) {
		// if we get here, we have one last parameter to serve
		return true;
	} else {
		// if we get here, we hit the end-of-file, so POST is over and there
		// are no more parameters
		return false;
	}
}

/* Retrieve a parameter that was encoded as part of the URL, stored in
 * the buffer pointed to by *tail.  tail is updated to point just past
 * the last character read from the buffer. */
URLPARAM_RESULT WebServer::nextURLparam(char **tail, char *name, int nameLen, char *value, int valueLen) {
	// assume name is at current place in stream
	char ch, hex[3];
	URLPARAM_RESULT result = URLPARAM_OK;
	char *s = *tail;
	bool keep_scanning = true;
	bool need_value = true;

	// clear out name and value so they'll be NUL terminated
	memset(name, 0, nameLen);
	memset(value, 0, valueLen);

	if (*s == 0)
		return URLPARAM_EOS;
	// Read the keyword name
	while (keep_scanning) {
		ch = *s++;
		switch (ch) {
		case 0:
			s--; // Back up to point to terminating NUL
			// Fall through to "stop the scan" code
		case '&':
			/* that's end of pair, go away */
			keep_scanning = false;
			need_value = false;
			break;
		case '+':
			ch = ' ';
			break;
		case '%':
			/* handle URL encoded characters by converting back
			 * to original form */
			if ((hex[0] = *s++) == 0) {
				s--; // Back up to NUL
				keep_scanning = false;
				need_value = false;
			} else {
				if ((hex[1] = *s++) == 0) {
					s--; // Back up to NUL
					keep_scanning = false;
					need_value = false;
				} else {
					hex[2] = 0;
					ch = strtoul(hex, NULL, 16);
				}
			}
			break;
		case '=':
			/* that's end of name, so switch to storing in value */
			keep_scanning = false;
			break;
		}

		// check against 1 so we don't overwrite the final NUL
		if (keep_scanning && (nameLen > 1)) {
			*name++ = ch;
			--nameLen;
		} else
			result = URLPARAM_NAME_OFLO;
	}

	if (need_value && (*s != 0)) {
		keep_scanning = true;
		while (keep_scanning) {
			ch = *s++;
			switch (ch) {
			case 0:
				s--; // Back up to point to terminating NUL
					 // Fall through to "stop the scan" code
			case '&':
				/* that's end of pair, go away */
				keep_scanning = false;
				need_value = false;
				break;
			case '+':
				ch = ' ';
				break;
			case '%':
				/* handle URL encoded characters by converting back to original form */
				if ((hex[0] = *s++) == 0) {
					s--; // Back up to NUL
					keep_scanning = false;
					need_value = false;
				} else {
					if ((hex[1] = *s++) == 0) {
						s--; // Back up to NUL
						keep_scanning = false;
						need_value = false;
					} else {
						hex[2] = 0;
						ch = strtoul(hex, NULL, 16);
					}

				}
				break;
			}

			// check against 1 so we don't overwrite the final NUL
			if (keep_scanning && (valueLen > 1)) {
				*value++ = ch;
				--valueLen;
			} else
				result = (result == URLPARAM_OK) ? URLPARAM_VALUE_OFLO : URLPARAM_BOTH_OFLO;
		}
	}
	*tail = s;
	return result;
}

// Read and parse the first line of the request header.
// The "command" (GET/HEAD/POST) is translated into a numeric value in type.
// The URL is stored in request,  up to the length passed in length
// NOTE 1: length must include one byte for the terminating NUL.
// NOTE 2: request is NOT checked for NULL,  nor length for a value < 1.
// Reading stops when the code encounters a space, CR, or LF.  If the HTTP
// version was supplied by the client,  it will still be waiting in the input
// stream when we exit.
//
// On return, length contains the amount of space left in request.  If it's
// less than 0,  the URL was longer than the buffer,  and part of it had to
// be discarded.

void WebServer::getRequest(WebServer::ConnectionType &type, char *request, int *length) {
	--*length; // save room for NUL

	type = INVALID;

	// store the HTTP method line of the request
	if (expect("GET "))
		type = GET;
	else if (expect("HEAD "))
		type = HEAD;
	else if (expect("POST "))
		type = POST;
	else if (expect("PUT "))
		type = PUT;
	else if (expect("DELETE "))
		type = DELETE;
	else if (expect("PATCH "))
		type = PATCH;

	// if it doesn't start with any of those, we have an unknown method
	// so just get out of here
	else
		return;

	int ch;
	while ((ch = read()) != -1) {
		// stop storing at first space or end of line
		if (ch == ' ' || ch == '\n' || ch == '\r') {
			break;
		}
		if (*length > 0) {
			*request = ch;
			++request;
		}
		--*length;
	}
	// NUL terminate
	*request = 0;
}

void WebServer::processHeaders() {
	// look for three things: the Content-Length header, the Authorization
	// header, and the double-CRLF that ends the headers.

	// empty the m_authCredentials before every run of this function.
	// otherwise users who don't send an Authorization header would be treated
	// like the last user who tried to authenticate (possibly successful)
	m_authCredentials[0] = 0;

	while (1) {
		if (expect("Content-Length:")) {
			readInt(m_contentLength);
			continue;
		}

		if (expect("Authorization:")) {
			readHeader(m_authCredentials, 51);
			continue;
		}

		if (expect(CRLF CRLF)) {
			m_readingContent = true;
			return;
		}

		// no expect checks hit, so just absorb a character and try again
		if (read() == -1) {
			return;
		}
	}
}

void WebServer::processRestReguest(ConnectionType type, char *tail, bool tail_complete, Binding & binding) {

	long id = atol(tail);
	byte i = 0;

	aJsonObject *oldModel;
	aJsonObject *newModel;

	this->currentCollection = binding.collection;

	if (id) {
		oldModel = (*binding.collection)->child;
		if (type == WebServer::GET || type == WebServer::PUT || type == WebServer::DELETE) {
			while (oldModel) {
				aJsonObject* idItem = aJson.getObjectItem(oldModel, "id");
				if (idItem->valuelong == id) {
					break;
				}
				oldModel = oldModel->next;
				i++;
			}
			this->currentModel = &oldModel;
		}
		if (type == WebServer::PUT) {
			newModel = aJson.parse(*this);
			this->currentModel = &newModel;
		}
	} else {
		if (type == WebServer::POST) {
			newModel = aJson.parse(*this);
			this->currentModel = &newModel;
		} else {
			this->currentModel = NULL;
		}
	}

	if (*binding.beforeResponse != NULL) {
		binding.beforeResponse(*this, type, tail, tail_complete, this->currentCollection, this->currentModel);
	}

	if (!this->preventDefault) {
		if (id) {
			if (type == WebServer::GET) {
				httpSuccess("application/json");
				aJson.print(oldModel, *this);
			}
			if (type == WebServer::PUT) {
				aJson.replaceItemInArray(*this->currentCollection, i, newModel);
				httpSuccess("application/json");
				aJson.print(newModel, *this);
			}
			if (type == WebServer::DELETE) {
				aJson.deleteItemFromArray(*this->currentCollection, i);
				httpSuccess();
			}

		} else {
			if (type == WebServer::GET) {
				httpSuccess("application/json");
				aJson.print(*this->currentCollection, *this);
			}
			if (type == WebServer::POST) {
				aJson.addNumberToObject(newModel, "id", ++binding.curId);
				aJson.addItemToArray(*this->currentCollection, newModel);
				httpCreated("application/json");
				aJson.print(newModel, *this);
			}
			if (type == WebServer::PUT) {
				aJson.deleteItem(*this->currentCollection);
				*this->currentCollection = aJson.parse(*this);
				httpSuccess("application/json");
				aJson.print(*this->currentCollection, *this);
			}
			if (type == WebServer::DELETE) {
				aJson.deleteItem(*this->currentCollection);
				httpNoContent();
			}
		}

	} else {
		this->preventDefault = false;
	}


	if (*binding.afterResponce != NULL){
		this->finalCommand = binding.afterResponce;
	}

}

void WebServer::addJSONBinding(char * uri, aJsonObject ** jsonObject) {

	if (bindingsLenght < bindingsMaxLenght) {
		bindings[bindingsLenght].collection = jsonObject;
		bindings[bindingsLenght].uri = uri;


		long maxId = 0;

		aJsonObject *child = (*jsonObject)->child;

		while (child) {
			aJsonObject* idObject = aJson.getObjectItem(child, "id");
			if (idObject->valuelong > maxId) {
				maxId = idObject->valuelong;
			}
			child = child->next;
		}

		bindings[bindingsLenght].curId = maxId;
		bindingsLenght++;

	}



}
void WebServer::setBeforeRequest(char* uri, BeforeResponce beforeResponce){
	byte i;

	for (i = 0; i < this->bindingsLenght; ++i) {
			if ( (strcmp(uri, bindings[i].uri) == 0)) {
				bindings[i].beforeResponse = beforeResponce;
			}
		}

}
void WebServer::setAfterRequest(char* uri, AfterResponce afterResponce){
	byte i;

	for (i = 0; i < this->bindingsLenght; ++i) {
			if ( (strcmp(uri, bindings[i].uri) == 0)) {
				bindings[i].afterResponce = afterResponce;
			}
		}

}
