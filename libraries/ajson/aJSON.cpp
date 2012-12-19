/*
 Copyright (c) 2001, Interactive Matter, Marcus Nowotny

 Based on the cJSON Library, Copyright (C) 2009 Dave Gamble

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

// aJSON
// aJson Library for Arduino.
// This library is suited for Atmega328 based Arduinos.
// The RAM on ATmega168 based Arduinos is too limited
/******************************************************************************
 * Includes
 ******************************************************************************/

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>
#include <avr/pgmspace.h>
#include "aJSON.h"
#include "utility/streamhelper.h"
#include "utility/stringbuffer.h"
#include "webserver.h"

/******************************************************************************
 * Definitions
 ******************************************************************************/
//Default buffer sizes - buffers get initialized and grow acc to that size
#define BUFFER_DEFAULT_SIZE 4

//how much digits after . for float
#define FLOAT_PRECISION 5

// Internal constructor.
aJsonObject*
aJsonClass::newItem() {
	aJsonObject* node = (aJsonObject*) malloc(sizeof(aJsonObject));
	if (node)
		memset(node, 0, sizeof(aJsonObject));
	return node;
}

char ** aJsonClass::names;
int aJsonClass::namesLenght;

// Delete a aJsonObject structure.
void aJsonClass::deleteItem(aJsonObject *c) {
	aJsonObject *next;
	while (c) {
		next = c->next;
		if (!(c->type & aJson_IsReference) && c->child) {
			deleteItem(c->child);
		}
		if ((c->type == aJson_String) && c->valuestring) {
			free(c->valuestring);
		}
		if (c->name) {

			if (findNamePointer(c->name) == NULL) {
				free(c->name);
			}

		}
		free(c);
		c = next;
	}
}

// Parse the input text to generate a number, and populate the result into item.
int aJsonClass::parseNumber(aJsonObject *item, FILE* stream) {
	long i = 0;
	char sign = 1;

	int in = fgetc(stream);
	if (in == EOF) {
		return EOF;
	}
	// It is easier to decode ourselves than to use sscnaf,
	// since so we can easier decide between int & double
	if (in == '-') {
		//it is a negative number
		sign = -1;
		in = fgetc(stream);
		if (in == EOF) {
			return EOF;
		}
	}
	if (in >= '0' && in <= '9')
		do {
			i = (i * 10) + (in - '0');
			in = fgetc(stream);
		} while (in >= '0' && in <= '9'); // Number?
	//end of integer part Ð or isn't it?
	if (!(in == '.' || in == 'e' || in == 'E')) {

		if (i > 32767 || i < -32768) {
			item->valuelong = i * (int) sign;
			item->type = aJson_Long;
		} else {
			item->valueint = i * (int) sign;
			item->type = aJson_Int;
		}

	}
	//ok it seems to be a double
	else {
		double n = (double) i;
		int scale = 0;
		int subscale = 0;
		char signsubscale = 1;
		if (in == '.') {
			in = fgetc(stream);
			do {
				n = (n * 10.0) + (in - '0'), scale--;
				in = fgetc(stream);
			} while (in >= '0' && in <= '9');
		} // Fractional part?
		if (in == 'e' || in == 'E') // Exponent?
				{
			in = fgetc(stream);
			if (in == '+') {
				in = fgetc(stream);
			} else if (in == '-') {
				signsubscale = -1;
				in = fgetc(stream);
			}
			while (in >= '0' && in <= '9') {
				subscale = (subscale * 10) + (in - '0'); // Number?
				in = fgetc(stream);
			}
		}

		n = sign * n
				* pow(10.0,
						((double) scale
								+ (double) subscale * (double) signsubscale)); // number = +/- number.fraction * 10^+/- exponent

		item->valuefloat = n;
		item->type = aJson_Float;
	}
	//preserve the last character for the next routine
	ungetc(in, stream);
	return 0;
}

// Parse the input text to generate a number, and populate the result into item.
int aJsonClass::parseNumber(aJsonObject *item, File stream) {
	long i = 0;
	char sign = 1;

	int in = read(stream);
	if (in == EOF) {
		return EOF;
	}
	// It is easier to decode ourselves than to use sscnaf,
	// since so we can easier decide between int & double
	if (in == '-') {
		//it is a negative number
		sign = -1;
		in = read(stream);
		if (in == EOF) {
			return EOF;
		}
	}
	if (in >= '0' && in <= '9')
		do {
			i = (i * 10) + (in - '0');
			in = read(stream);
		} while (in >= '0' && in <= '9'); // Number?
	//end of integer part Ð or isn't it?
	if (!(in == '.' || in == 'e' || in == 'E')) {

		if (i > 32767 || i < -32768) {
			item->valuelong = i * (int) sign;
			item->type = aJson_Long;
		} else {
			item->valueint = i * (int) sign;
			item->type = aJson_Int;
		}

	}
	//ok it seems to be a double
	else {
		double n = (double) i;
		int scale = 0;
		int subscale = 0;
		char signsubscale = 1;
		if (in == '.') {
			in  = read(stream);
			do {
				n = (n * 10.0) + (in - '0'), scale--;
				in  = read(stream);
			} while (in >= '0' && in <= '9');
		} // Fractional part?
		if (in == 'e' || in == 'E') // Exponent?
				{
			in  = read(stream);
			if (in == '+') {
				in = read(stream);
			} else if (in == '-') {
				signsubscale = -1;
				in = read(stream);
			}
			while (in >= '0' && in <= '9') {
				subscale = (subscale * 10) + (in - '0'); // Number?
				in = read(stream);
			}
		}

		n = sign * n
				* pow(10.0,
						((double) scale
								+ (double) subscale * (double) signsubscale)); // number = +/- number.fraction * 10^+/- exponent

		item->valuefloat = n;
		item->type = aJson_Float;
	}
	//preserve the last character for the next routine
	push(in);
	return 0;
}

// Parse the input text to generate a number, and populate the result into item.
int aJsonClass::parseNumber(aJsonObject *item, WebServer& server) {
	long i = 0;
	char sign = 1;

	int in = server.read();
	if (in == EOF) {
		return EOF;
	}
	// It is easier to decode ourselves than to use sscnaf,
	// since so we can easier decide between int & double
	if (in == '-') {
		//it is a negative number
		sign = -1;
		in = server.read();
		if (in == EOF) {
			return EOF;
		}
	}
	if (in >= '0' && in <= '9')
		do {
			i = (i * 10) + (in - '0');
			in = server.read();
		} while (in >= '0' && in <= '9'); // Number?
	//end of integer part Ð or isn't it?
	if (!(in == '.' || in == 'e' || in == 'E')) {
		if (i > 32767 || i < -32768) {
			item->valuelong = i * (int) sign;
			item->type = aJson_Long;
		} else {
			item->valueint = i * (int) sign;
			item->type = aJson_Int;
		}
	}
	//ok it seems to be a double
	else {
		double n = (double) i;
		int scale = 0;
		int subscale = 0;
		char signsubscale = 1;
		if (in == '.') {
			in = server.read();
			do {
				n = (n * 10.0) + (in - '0'), scale--;
				in = server.read();
			} while (in >= '0' && in <= '9');
		} // Fractional part?
		if (in == 'e' || in == 'E') // Exponent?
				{
			in = server.read();
			if (in == '+') {
				in = server.read();
			} else if (in == '-') {
				signsubscale = -1;
				in = server.read();
			}
			while (in >= '0' && in <= '9') {
				subscale = (subscale * 10) + (in - '0'); // Number?
				in = server.read();
			}
		}

		n = sign * n
				* pow(10.0,
						((double) scale
								+ (double) subscale * (double) signsubscale)); // number = +/- number.fraction * 10^+/- exponent

		item->valuefloat = n;
		item->type = aJson_Float;
	}
	//preserve the last character for the next routine
	server.push(in);
	return 0;
}

// Render the number nicely from the given item into a string.
int aJsonClass::printInt(aJsonObject *item, FILE* stream) {
	if (item != NULL) {
		return fprintf_P(stream, PSTR("%d"), item->valueint);
	}
	//printing nothing is ok
	return 0;
}

void aJsonClass::printInt(aJsonObject *item, File stream) {
	if (item != NULL) {
		stream.print(item->valueint);
	}

}

void aJsonClass::printInt(aJsonObject *item, WebServer &server) {
	if (item != NULL) {
		server.print(item->valueint);
	}

}

// Render the number nicely from the given item into a string.
int aJsonClass::printLong(aJsonObject *item, FILE* stream) {
	if (item != NULL) {
		return fprintf_P(stream, PSTR("%d"), item->valuelong);
	}
	//printing nothing is ok
	return 0;
}


void aJsonClass::printLong(aJsonObject *item, File stream) {
	if (item != NULL) {
		stream.print(item->valuelong);
	}
	//printing nothing is ok
}

void aJsonClass::printLong(aJsonObject *item, WebServer &server) {
	if (item != NULL) {
		server.print(item->valuelong);
	}
	//printing nothing is ok

}

int aJsonClass::printFloat(aJsonObject *item, FILE* stream) {
	if (item != NULL) {
		double d = item->valuefloat;
		if (d < 0.0) {
			fprintf_P(stream, PSTR("-"));
			d = -d;
		}
		//print the integer part
		unsigned long integer_number = (unsigned long) d;
		fprintf_P(stream, PSTR("%u."), integer_number);
		//print the fractional part
		double fractional_part = d - ((double) integer_number);
		//we do a do-while since we want to print at least one zero
		//we just support a certain number of digits after the '.'
		int n = FLOAT_PRECISION;
		fractional_part += 0.5 / pow(10.0, FLOAT_PRECISION);
		do {
			//make the first digit non fractional(shift it before the '.'
			fractional_part *= 10.0;
			//create an int out of it
			unsigned int digit = (unsigned int) fractional_part;
			//print it
			fprintf_P(stream, PSTR("%u"), digit);
			//remove it from the number
			fractional_part -= (double) digit;
			n--;
		} while ((fractional_part != 0) && (n > 0));
	}
	//printing nothing is ok
	return 0;
}


void aJsonClass::printFloat(aJsonObject *item, File stream) {
	if (item != NULL) {
			double d = item->valuefloat;
			if (d < 0.0) {
				stream.print("-");
				d=-d;
			}
			//print the integer part
			unsigned long integer_number = (unsigned long) d;
			sprintf_P(valueBuffer, PSTR("%u."), integer_number);
			stream.print(valueBuffer);
			//print the fractional part
			double fractional_part = d - ((double) integer_number);
			//we do a do-while since we want to print at least one zero
			//we just support a certain number of digits after the '.'
			int n = FLOAT_PRECISION;
			fractional_part += 0.5 / pow(10.0, FLOAT_PRECISION);
			do {
				//make the first digit non fractional(shift it before the '.'
				fractional_part *= 10.0;
				//create an int out of it
				unsigned int digit = (unsigned int) fractional_part;
				//print it
				sprintf_P(valueBuffer, PSTR("%u"), digit);
				stream.print(valueBuffer);
				//remove it from the number
				fractional_part -= (double) digit;
				n--;
			} while ((fractional_part != 0) && (n > 0));
		}
		//printing nothing is ok
}




void aJsonClass::printFloat(aJsonObject *item, WebServer &server) {
	if (item != NULL) {
		double d = item->valuefloat;
		if (d < 0.0) {
			P(minusString)="-";
			server.printP(minusString);
			d=-d;
		}
		//print the integer part
		unsigned long integer_number = (unsigned long) d;
		sprintf_P(valueBuffer, PSTR("%u."), integer_number);
		server.print(valueBuffer);
		//print the fractional part
		double fractional_part = d - ((double) integer_number);
		//we do a do-while since we want to print at least one zero
		//we just support a certain number of digits after the '.'
		int n = FLOAT_PRECISION;
		fractional_part += 0.5 / pow(10.0, FLOAT_PRECISION);
		do {
			//make the first digit non fractional(shift it before the '.'
			fractional_part *= 10.0;
			//create an int out of it
			unsigned int digit = (unsigned int) fractional_part;
			//print it
			sprintf_P(valueBuffer, PSTR("%u"), digit);
			server.print(valueBuffer);
			//remove it from the number
			fractional_part -= (double) digit;
			n--;
		} while ((fractional_part != 0) && (n > 0));
	}
	//printing nothing is ok

}

// Parse the input text into an unescaped cstring, and populate item.
int aJsonClass::parseString(aJsonObject *item, FILE* stream) {
	//we do not need to skip here since the first byte should be '\"'
	int in = fgetc(stream);
	if (in != '\"') {
		return EOF; // not a string!
	}
	item->type = aJson_String;
	//allocate a buffer & track how long it is and how much we have read
	string_buffer* buffer = stringBufferCreate();
	if (buffer == NULL) {
		//unable to allocate the string
		return EOF;
	}
	in = fgetc(stream);
	if (in == EOF) {
		stringBufferFree(buffer);
		return EOF;
	}
	while (in != EOF) {
		while (in != '\"' && in >= 32) {
			if (in != '\\') {
				stringBufferAdd((char) in, buffer);
			} else {
				in = fgetc(stream);
				if (in == EOF) {
					stringBufferFree(buffer);
					return EOF;
				}
				switch (in) {
				case '\\':
					stringBufferAdd('\\', buffer);
					break;
				case '\"':
					stringBufferAdd('\"', buffer);
					break;
				case 'b':
					stringBufferAdd('\b', buffer);
					break;
				case 'f':
					stringBufferAdd('\f', buffer);
					break;
				case 'n':
					stringBufferAdd('\n', buffer);
					break;
				case 'r':
					stringBufferAdd('\r', buffer);
					break;
				case 't':
					stringBufferAdd('\t', buffer);
					break;
				default:
					//we do not understand it so we skip it
					break;
				}
			}
			in = fgetc(stream);
			if (in == EOF) {
				stringBufferFree(buffer);
				return EOF;
			}
		}
		//the string ends here
		item->valuestring = stringBufferToString(buffer);
		return 0;
	}
	//we should not be here but it is ok
	return 0;
}


int aJsonClass::parseString(aJsonObject *item, File stream) {
	//we do not need to skip here since the first byte should be '\"'
	int in = read(stream);
	if (in != '\"') {
		return EOF; // not a string!
	}
	item->type = aJson_String;
	//allocate a buffer & track how long it is and how much we have read
	string_buffer* buffer = stringBufferCreate();
	if (buffer == NULL) {
		//unable to allocate the string
		return EOF;
	}
	in = read(stream);
	if (in == EOF) {
		stringBufferFree(buffer);
		return EOF;
	}
	while (in != EOF) {
		while (in != '\"' && in >= 32) {
			if (in != '\\') {
				stringBufferAdd((char) in, buffer);
			} else {
				in = read(stream);
				if (in == EOF) {
					stringBufferFree(buffer);
					return EOF;
				}
				switch (in) {
				case '\\':
					stringBufferAdd('\\', buffer);
					break;
				case '\"':
					stringBufferAdd('\"', buffer);
					break;
				case 'b':
					stringBufferAdd('\b', buffer);
					break;
				case 'f':
					stringBufferAdd('\f', buffer);
					break;
				case 'n':
					stringBufferAdd('\n', buffer);
					break;
				case 'r':
					stringBufferAdd('\r', buffer);
					break;
				case 't':
					stringBufferAdd('\t', buffer);
					break;
				default:
					//we do not understand it so we skip it
					break;
				}
			}
			in = read(stream);
			if (in == EOF) {
				stringBufferFree(buffer);
				return EOF;
			}
		}
		//the string ends here
		item->valuestring = stringBufferToString(buffer);
		return 0;
	}
	//we should not be here but it is ok
	return 0;
}

int aJsonClass::parseString(aJsonObject *item, WebServer& server) {
	//we do not need to skip here since the first byte should be '\"'
	int in = server.read();
	if (in != '\"') {
		return EOF; // not a string!
	}
	item->type = aJson_String;
	//allocate a buffer & track how long it is and how much we have read
	string_buffer* buffer = stringBufferCreate();
	if (buffer == NULL) {
		//unable to allocate the string
		return EOF;
	}
	in = server.read();
	if (in == EOF) {
		stringBufferFree(buffer);
		return EOF;
	}
	while (in != EOF) {
		while (in != '\"' && in >= 32) {
			if (in != '\\') {
				stringBufferAdd((char) in, buffer);
			} else {
				in = server.read();
				if (in == EOF) {
					stringBufferFree(buffer);
					return EOF;
				}
				switch (in) {
				case '\\':
					stringBufferAdd('\\', buffer);
					break;
				case '\"':
					stringBufferAdd('\"', buffer);
					break;
				case 'b':
					stringBufferAdd('\b', buffer);
					break;
				case 'f':
					stringBufferAdd('\f', buffer);
					break;
				case 'n':
					stringBufferAdd('\n', buffer);
					break;
				case 'r':
					stringBufferAdd('\r', buffer);
					break;
				case 't':
					stringBufferAdd('\t', buffer);
					break;
				default:
					//we do not understand it so we skip it
					break;
				}
			}
			in = server.read();
			if (in == EOF) {
				stringBufferFree(buffer);
				return EOF;
			}
		}
		//the string ends here
		item->valuestring = stringBufferToString(buffer);
		return 0;
	}
	//we should not be here but it is ok
	return 0;
}

// Render the cstring provided to an escaped version that can be printed.
int aJsonClass::printStringPtr(const char *str, FILE* stream) {
	if (fputc('\"', stream) == EOF) {
		return EOF;
	}
	char* ptr = (char*) str;
	if (ptr != NULL) {
		while (*ptr != 0) {
			if ((unsigned char) *ptr > 31 && *ptr != '\"' && *ptr != '\\') {
				if (fputc(*ptr, stream) == EOF) {
					return EOF;
				}
				ptr++;
			} else {
				if (fputc('\\', stream) == EOF) {
					return EOF;
				}
				switch (*ptr++) {
				case '\\':
					if (fputc('\\', stream) == EOF) {
						return EOF;
					}
					break;
				case '\"':
					if (fputc('\"', stream) == EOF) {
						return EOF;
					}
					break;
				case '\b':
					if (fputc('b', stream) == EOF) {
						return EOF;
					}
					break;
				case '\f':
					if (fputc('f', stream) == EOF) {
						return EOF;
					}
					break;
				case '\n':
					if (fputc('n', stream) == EOF) {
						return EOF;
					}
					break;
				case '\r':
					if (fputc('r', stream) == EOF) {
						return EOF;
					}
					break;
				case '\t':
					if (fputc('t', stream) == EOF) {
						return EOF;
					}
					break;
				default:
					break; // eviscerate with prejudice.
				}
			}

		}
	}
	if (fputc('\"', stream) == EOF) {
		return EOF;
	}
	return 0;
}

void aJsonClass::printStringPtr(const char *str, File stream) {
	stream.print('\"');

	char* ptr = (char*) str;
	if (ptr != NULL) {
		while (*ptr != 0) {
			if ((unsigned char) *ptr > 31 && *ptr != '\"' && *ptr != '\\') {
				stream.print(*ptr);
				ptr++;
			} else {
				stream.print('\\');
				switch (*ptr++) {
				case '\\':
					stream.print('\\');
					break;
				case '\"':
					stream.print('\"');
					break;
				case '\b':
					stream.print('b');
					break;
				case '\f':
					stream.print('f');
					break;
				case '\n':
					stream.print('n');
					break;
				case '\r':
					stream.print('r');
					break;
				case '\t':
					stream.print('t');
					break;
				default:
					break; // eviscerate with prejudice.
				}
			}

		}
	}
	stream.print('\"');
}

void aJsonClass::printStringPtr(const char *str, WebServer &server) {
	server.print('\"');

	char* ptr = (char*) str;
	if (ptr != NULL) {
		while (*ptr != 0) {
			if ((unsigned char) *ptr > 31 && *ptr != '\"' && *ptr != '\\') {
				server.print(*ptr);
				ptr++;
			} else {
				server.print('\\');
				switch (*ptr++) {
				case '\\':
					server.print('\\');
					break;
				case '\"':
					server.print('\"');
					break;
				case '\b':
					server.print('b');
					break;
				case '\f':
					server.print('f');
					break;
				case '\n':
					server.print('n');
					break;
				case '\r':
					server.print('r');
					break;
				case '\t':
					server.print('t');
					break;
				default:
					break; // eviscerate with prejudice.
				}
			}

		}
	}
	server.print('\"');
}

// Invote print_string_ptr (which is useful) on an item.
int aJsonClass::printString(aJsonObject *item, FILE* stream) {
	return printStringPtr(item->valuestring, stream);
}

void aJsonClass::printString(aJsonObject *item, File stream) {
	printStringPtr(item->valuestring, stream);
}

void aJsonClass::printString(aJsonObject *item, WebServer &server) {
	printStringPtr(item->valuestring, server);
}

// Utility to jump whitespace and cr/lf
int aJsonClass::skip(FILE* stream) {
	if (stream == NULL) {
		return EOF;
	}
	int in = fgetc(stream);
	while (in != EOF && (in <= 32)) {
		in = fgetc(stream);
	}
	if (in != EOF) {
		if (ungetc(in, stream) == EOF) {
			return EOF;
		}
		return 0;
	}
	return EOF;
}

// Utility to jump whitespace and cr/lf
int aJsonClass::skip(File stream) {

	int in = read(stream);
	while (in != EOF && (in <= 32)) {
		in = read(stream);
	}
	if (in != EOF) {
		push(in);
		return 0;
	}
	return EOF;
}

// Utility to jump whitespace and cr/lf
int aJsonClass::skip(WebServer &server) {

	int in = server.read();
	while (in != EOF && (in <= 32)) {
		in = server.read();
	}
	if (in != EOF) {
		server.push(in);
		return 0;
	}
	return EOF;
}

// Parse an object - create a new root, and populate.
aJsonObject*
aJsonClass::parse(char *value) {
	FILE* string_input_stream = openStringInputStream(value);
	aJsonObject* result = parse(string_input_stream, NULL);
	closeStringInputStream(string_input_stream);
	return result;
}

// Parse an object - create a new root, and populate.
aJsonObject*
aJsonClass::parse(FILE* stream) {
	return parse(stream, NULL);
}

aJsonObject*
aJsonClass::parse(File stream) {
	return parse(stream, NULL);
}

aJsonObject*
aJsonClass::parse(WebServer& server) {
	return parse(server, NULL);
}

// Parse an object - create a new root, and populate.
aJsonObject*
aJsonClass::parse(FILE* stream, char** filter) {
	if (stream == NULL) {
		return NULL;
	}
	aJsonObject *c = newItem();
	if (!c)
		return NULL; /* memory fail */

	skip(stream);
	if (parseValue(c, stream, filter) == EOF) {
		deleteItem(c);
		return NULL;
	}
	return c;
}

aJsonObject*
aJsonClass::parse(File stream, char** filter) {

	aJsonObject *c = newItem();
	if (!c)
		return NULL; /* memory fail */

	skip(stream);
	if (parseValue(c, stream, filter) == EOF) {
		deleteItem(c);
		return NULL;
	}
	return c;
}

aJsonObject*
aJsonClass::parse(WebServer& server, char** filter) {

	aJsonObject *c = newItem();
	if (!c)
		return NULL; /* memory fail */

	skip(server);
	if (parseValue(c, server, filter) == EOF) {
		deleteItem(c);
		return NULL;
	}
	return c;
}

// Render a aJsonObject item/entity/structure to text.
int aJsonClass::print(aJsonObject* item, FILE* stream) {
	return printValue(item, stream);
}

void aJsonClass::print(aJsonObject* item, File stream) {
	printValue(item, stream);
}

void aJsonClass::print(aJsonObject* item, WebServer& server) {
	printValue(item, server);
}

char*
aJsonClass::print(aJsonObject* item) {
	FILE* stream = openStringOutputStream();
	if (stream == NULL) {
		return NULL;
	}
	print(item, stream);
	return closeStringOutputStream(stream);
}

// Parser core - when encountering text, process appropriately.
int aJsonClass::parseValue(aJsonObject *item, FILE* stream, char** filter) {
	if (stream == NULL) {
		return EOF; // Fail on null.
	}
	if (skip(stream) == EOF) {
		return EOF;
	}
	//read the first byte from the stream
	int in = fgetc(stream);
	if (in == EOF) {
		return EOF;
	}
	if (ungetc(in, stream) == EOF) {
		return EOF;
	}
	if (in == '\"') {
		return parseString(item, stream);
	} else if (in == '-' || (in >= '0' && in <= '9')) {
		return parseNumber(item, stream);
	} else if (in == '[') {
		return parseArray(item, stream, filter);
	} else if (in == '{') {
		return parseObject(item, stream, filter);
	}
	//it can only be null, false or true
	else if (in == 'n') {
		//a buffer to read the value
		char buffer[] = { 0, 0, 0, 0 };
		if (fread(buffer, sizeof(char), 4, stream) != 4) {
			return EOF;
		}
		if (!strncmp(buffer, "null", 4)) {
			item->type = aJson_NULL;
			return 0;
		} else {
			return EOF;
		}
	} else if (in == 'f') {
		//a buffer to read the value
		char buffer[] = { 0, 0, 0, 0, 0 };
		if (fread(buffer, sizeof(char), 5, stream) != 5) {
			return EOF;
		}
		if (!strncmp(buffer, "false", 5)) {
			item->type = aJson_False;
			item->valuebool = 0;
			return 0;
		}
	} else if (in == 't') {
		//a buffer to read the value
		char buffer[] = { 0, 0, 0, 0 };
		if (fread(buffer, sizeof(char), 4, stream) != 4) {
			return EOF;
		}
		if (!strncmp(buffer, "true", 4)) {
			item->type = aJson_True;
			item->valuebool = -1;
			return 0;
		}
	}

	return EOF; // failure.
}

int aJsonClass::parseValue(aJsonObject *item, File stream,
		char** filter) {

	if (skip(stream) == EOF) {
		return EOF;
	}
	//read the first byte from the stream
	int in = read(stream);
	if (in == EOF) {
		return EOF;
	}

	push(in);

	if (in == '\"') {
		return parseString(item, stream);
	} else if (in == '-' || (in >= '0' && in <= '9')) {
		return parseNumber(item, stream);
	} else if (in == '[') {
		return parseArray(item, stream, filter);
	} else if (in == '{') {
		return parseObject(item, stream, filter);
	}
	//it can only be null, false or true
	else if (in == 'n') {
		//a buffer to read the value
		char buffer[] = { 0, 0, 0, 0 };

		for (int i = 0; i < 4; i++) {
			if ((buffer[i] = read(stream)) == EOF) {
				return EOF;
			}
		}

		if (!strncmp(buffer, "null", 4)) {
			item->type = aJson_NULL;
			return 0;
		} else {
			return EOF;
		}
	} else if (in == 'f') {
		//a buffer to read the value
		char buffer[] = { 0, 0, 0, 0, 0 };
		for (int i = 0; i < 5; i++) {
			if ((buffer[i] = read(stream)) == EOF) {
				return EOF;
			}
		}
		if (!strncmp(buffer, "false", 5)) {
			item->type = aJson_False;
			item->valuebool = 0;
			return 0;
		}
	} else if (in == 't') {
		//a buffer to read the value
		char buffer[] = { 0, 0, 0, 0 };
		for (int i = 0; i < 4; i++) {
			if ((buffer[i] = read(stream)) == EOF) {
				return EOF;
			}
		}
		if (!strncmp(buffer, "true", 4)) {
			item->type = aJson_True;
			item->valuebool = -1;
			return 0;
		}
	}

	return EOF; // failure.
}

int aJsonClass::parseValue(aJsonObject *item, WebServer& server,
		char** filter) {

	if (skip(server) == EOF) {
		return EOF;
	}
	//read the first byte from the stream
	int in = server.read();
	if (in == EOF) {
		return EOF;
	}

	server.push(in);

	if (in == '\"') {
		return parseString(item, server);
	} else if (in == '-' || (in >= '0' && in <= '9')) {
		return parseNumber(item, server);
	} else if (in == '[') {
		return parseArray(item, server, filter);
	} else if (in == '{') {
		return parseObject(item, server, filter);
	}
	//it can only be null, false or true
	else if (in == 'n') {
		//a buffer to read the value
		char buffer[] = { 0, 0, 0, 0 };

		for (int i = 0; i < 4; i++) {
			if ((buffer[i] = server.read()) == EOF) {
				return EOF;
			}
		}

		if (!strncmp(buffer, "null", 4)) {
			item->type = aJson_NULL;
			return 0;
		} else {
			return EOF;
		}
	} else if (in == 'f') {
		//a buffer to read the value
		char buffer[] = { 0, 0, 0, 0, 0 };
		for (int i = 0; i < 5; i++) {
			if ((buffer[i] = server.read()) == EOF) {
				return EOF;
			}
		}
		if (!strncmp(buffer, "false", 5)) {
			item->type = aJson_False;
			item->valuebool = 0;
			return 0;
		}
	} else if (in == 't') {
		//a buffer to read the value
		char buffer[] = { 0, 0, 0, 0 };
		for (int i = 0; i < 4; i++) {
			if ((buffer[i] = server.read()) == EOF) {
				return EOF;
			}
		}
		if (!strncmp(buffer, "true", 4)) {
			item->type = aJson_True;
			item->valuebool = -1;
			return 0;
		}
	}

	return EOF; // failure.
}

// Render a value to text.
int aJsonClass::printValue(aJsonObject *item, FILE* stream) {
	int result = 0;
	if (item == NULL) {
		//nothing to do
		return 0;
	}
	switch (item->type) {
	case aJson_NULL:
		result = fprintf_P(stream, PSTR("null"));
		break;
	case aJson_False:
		result = fprintf_P(stream, PSTR("false"));
		break;
	case aJson_True:
		result = fprintf_P(stream, PSTR("true"));
		break;
	case aJson_Int:
		result = printInt(item, stream);
		break;
	case aJson_Long:
		result = printLong(item, stream);
		break;
	case aJson_Float:
		result = printFloat(item, stream);
		break;
	case aJson_String:
		result = printString(item, stream);
		break;
	case aJson_Array:
		result = printArray(item, stream);
		break;
	case aJson_Object:
		result = printObject(item, stream);
		break;
	}
	//good time to flush the stream
	fflush(stream);
	return result;
}


void aJsonClass::printValue(aJsonObject *item, File stream) {

	if (item == NULL) {
		//nothing to do

	}
	switch (item->type) {
	case aJson_NULL:
		sprintf_P(valueBuffer, PSTR("null"));
		stream.print(valueBuffer);

		stream.print("null");
		break;
		case aJson_False:
		stream.print("false");
		break;
		case aJson_True:
		stream.print("true");
		break;
		case aJson_Int:
		printInt(item, stream);
		break;
		case aJson_Long:
		printLong(item, stream);
		break;
		case aJson_Float:
		printFloat(item, stream);
		break;
		case aJson_String:
		printString(item, stream);
		break;
		case aJson_Array:
		printArray(item, stream);
		break;
		case aJson_Object:
		printObject(item, stream);
		break;
	}
	//good time to flush the stream
}

void aJsonClass::printValue(aJsonObject *item, WebServer &server) {

	if (item == NULL) {
		//nothing to do

	}
	switch (item->type) {
	case aJson_NULL:
		sprintf_P(valueBuffer, PSTR("null"));
		server.print(valueBuffer);
		P(nullString)="null";
		server.printP(nullString);
		break;
		case aJson_False:
		P(falseString)="false";
		server.printP(falseString);
		break;
		case aJson_True:
		P(trueString)="true";
		server.printP(trueString);
		break;
		case aJson_Int:
		printInt(item, server);
		break;
		case aJson_Long:
		printLong(item, server);
		break;
		case aJson_Float:
		printFloat(item, server);
		break;
		case aJson_String:
		printString(item, server);
		break;
		case aJson_Array:
		printArray(item, server);
		break;
		case aJson_Object:
		printObject(item, server);
		break;
	}
	//good time to flush the stream
}

// Build an array from input text.
int aJsonClass::parseArray(aJsonObject *item, FILE* stream, char** filter) {
	int in = fgetc(stream);
	if (in != '[') {
		return EOF; // not an array!
	}

	item->type = aJson_Array;
	skip(stream);
	in = fgetc(stream);
	//check for empty array
	if (in == ']') {
		return 0; // empty array.
	}
	//now put back the last character
	ungetc(in, stream);
	aJsonObject *child;
	char first = -1;
	while ((first) || (in == ',')) {
		aJsonObject *new_item = newItem();
		if (new_item == NULL) {
			return EOF; // memory fail
		}
		if (first) {
			item->child = new_item;
			first = 0;
		} else {
			child->next = new_item;
			new_item->prev = child;
		}
		child = new_item;
		skip(stream);
		if (parseValue(child, stream, filter)) {
			return EOF;
		}
		skip(stream);
		in = fgetc(stream);
	}
	if (in == ']') {
		return 0; // end of array
	} else {
		return EOF; // malformed.
	}
}


int aJsonClass::parseArray(aJsonObject *item, File stream,
		char** filter) {
	int in = read(stream);
	if (in != '[') {
		return EOF; // not an array!
	}

	item->type = aJson_Array;
	skip(stream);
	in = read(stream);
	//check for empty array
	if (in == ']') {
		return 0; // empty array.
	}
	//now put back the last character
	push(in);
	aJsonObject *child;
	char first = -1;
	while ((first) || (in == ',')) {
		aJsonObject *new_item = newItem();
		if (new_item == NULL) {
			return EOF; // memory fail
		}
		if (first) {
			item->child = new_item;
			first = 0;
		} else {
			child->next = new_item;
			new_item->prev = child;
		}
		child = new_item;
		skip(stream);
		if (parseValue(child, stream, filter)) {
			return EOF;
		}
		skip(stream);
		in = read(stream);
	}
	if (in == ']') {
		return 0; // end of array
	} else {
		return EOF; // malformed.
	}
}


int aJsonClass::parseArray(aJsonObject *item, WebServer &server,
		char** filter) {
	int in = server.read();
	if (in != '[') {
		return EOF; // not an array!
	}

	item->type = aJson_Array;
	skip(server);
	in = server.read();
	//check for empty array
	if (in == ']') {
		return 0; // empty array.
	}
	//now put back the last character
	server.push(in);
	aJsonObject *child;
	char first = -1;
	while ((first) || (in == ',')) {
		aJsonObject *new_item = newItem();
		if (new_item == NULL) {
			return EOF; // memory fail
		}
		if (first) {
			item->child = new_item;
			first = 0;
		} else {
			child->next = new_item;
			new_item->prev = child;
		}
		child = new_item;
		skip(server);
		if (parseValue(child, server, filter)) {
			return EOF;
		}
		skip(server);
		in = server.read();
	}
	if (in == ']') {
		return 0; // end of array
	} else {
		return EOF; // malformed.
	}
}

// Render an array to text
int aJsonClass::printArray(aJsonObject *item, FILE* stream) {
	if (item == NULL) {
		//nothing to do
		return 0;
	}
	aJsonObject *child = item->child;
	if (fputc('[', stream) == EOF) {
		return EOF;
	}
	while (child) {
		if (printValue(child, stream) == EOF) {
			return EOF;
		}
		child = child->next;
		if (child) {
			if (fputc(',', stream) == EOF) {
				return EOF;
			}
		}
	}
	if (fputc(']', stream) == EOF) {
		return EOF;
	}
	return 0;
}

void aJsonClass::printArray(aJsonObject *item, File stream) {
	if (item == NULL) {
		//nothing to do
		return;
	}
	aJsonObject *child = item->child;
	stream.print('[');

	while (child) {
		printValue(child, stream);
		child = child->next;

		if (child) {
			stream.print(',');
		}
	}
	stream.print(']');
}

void aJsonClass::printArray(aJsonObject *item, WebServer &server) {
	if (item == NULL) {
		//nothing to do
		return;
	}
	aJsonObject *child = item->child;
	server.print('[');

	while (child) {
		printValue(child, server);
		child = child->next;

		if (child) {
			server.print(',');
		}
	}
	server.print(']');
}

// Build an object from the text.
int aJsonClass::parseObject(aJsonObject *item, FILE* stream, char** filter) {
	int in = fgetc(stream);
	if (in != '{') {
		return EOF; // not an object!
	}

	item->type = aJson_Object;
	skip(stream);
	//check for an empty object
	in = fgetc(stream);
	if (in == '}') {
		return 0; // empty object.
	}
	//preserve the char for the next parser
	ungetc(in, stream);

	aJsonObject* child;
	char first = -1;
	while ((first) || (in == ',')) {
		//in = fgetc(stream);
		aJsonObject* new_item = newItem();
		if (new_item == NULL) {
			return EOF; // memory fail
		}
		if (first) {
			first = 0;
			item->child = new_item;
		} else {
			child->next = new_item;
			new_item->prev = child;
		}
		child = new_item;
		skip(stream);
		if (parseString(child, stream) == EOF) {
			return EOF;
		}
		skip(stream);

		char * namePointer = findNamePointer(child->valuestring);

		if (namePointer != NULL) {

			child->name = namePointer;
			free(child->valuestring);
		} else {
			child->name = child->valuestring;
		}

		child->valuestring = NULL;

		in = fgetc(stream);
		if (in != ':') {
			return EOF; // fail!
		}
		// skip any spacing, get the value.
		skip(stream);
		if (parseValue(child, stream, filter) == EOF) {
			return EOF;
		}
		in = fgetc(stream);
	}

	if (in == '}') {
		return 0; // end of array
	} else {
		return EOF; // malformed.
	}
}

int aJsonClass::parseObject(aJsonObject *item, File stream,
		char** filter) {
	int in = read(stream);
	if (in != '{') {
		return EOF; // not an object!
	}

	item->type = aJson_Object;
	skip(stream);
	//check for an empty object
	in = read(stream);
	if (in == '}') {
		return 0; // empty object.
	}
	//preserve the char for the next parser
	push(in);

	aJsonObject* child;
	char first = -1;
	while ((first) || (in == ',')) {
		//in = fgetc(stream);
		aJsonObject* new_item = newItem();
		if (new_item == NULL) {
			return EOF; // memory fail
		}
		if (first) {
			first = 0;
			item->child = new_item;
		} else {
			child->next = new_item;
			new_item->prev = child;
		}
		child = new_item;
		skip(stream);
		if (parseString(child, stream) == EOF) {
			return EOF;
		}
		skip(stream);

		char * namePointer = findNamePointer(child->valuestring);

		if (namePointer != NULL) {
			child->name = namePointer;
			free(child->valuestring);
		} else {
			child->name = child->valuestring;
		}
		child->valuestring = NULL;

		in = read(stream);
		if (in != ':') {
			return EOF; // fail!
		}
		// skip any spacing, get the value.
		skip(stream);
		if (parseValue(child, stream, filter) == EOF) {
			return EOF;
		}
		in = read(stream);
	}

	if (in == '}') {
		return 0; // end of array
	} else {
		return EOF; // malformed.
	}
}

int aJsonClass::parseObject(aJsonObject *item, WebServer& server,
		char** filter) {
	int in = server.read();
	if (in != '{') {
		return EOF; // not an object!
	}

	item->type = aJson_Object;
	skip(server);
	//check for an empty object
	in = server.read();
	if (in == '}') {
		return 0; // empty object.
	}
	//preserve the char for the next parser
	server.push(in);

	aJsonObject* child;
	char first = -1;
	while ((first) || (in == ',')) {
		//in = fgetc(stream);
		aJsonObject* new_item = newItem();
		if (new_item == NULL) {
			return EOF; // memory fail
		}
		if (first) {
			first = 0;
			item->child = new_item;
		} else {
			child->next = new_item;
			new_item->prev = child;
		}
		child = new_item;
		skip(server);
		if (parseString(child, server) == EOF) {
			return EOF;
		}
		skip(server);

		char * namePointer = findNamePointer(child->valuestring);

		if (namePointer != NULL) {
			child->name = namePointer;
			free(child->valuestring);
		} else {
			child->name = child->valuestring;
		}
		child->valuestring = NULL;

		in = server.read();
		if (in != ':') {
			return EOF; // fail!
		}
		// skip any spacing, get the value.
		skip(server);
		if (parseValue(child, server, filter) == EOF) {
			return EOF;
		}
		in = server.read();
	}

	if (in == '}') {
		return 0; // end of array
	} else {
		return EOF; // malformed.
	}
}

// Render an object to text.
int aJsonClass::printObject(aJsonObject *item, FILE* stream) {
	if (item == NULL) {
		//nothing to do
		return 0;
	}
	aJsonObject *child = item->child;
	if (fputc('{', stream) == EOF) {
		return EOF;
	}
	while (child) {
		if (printStringPtr(child->name, stream) == EOF) {
			return EOF;
		}
		if (fputc(':', stream) == EOF) {
			return EOF;
		}
		if (printValue(child, stream) == EOF) {
			return EOF;
		}
		child = child->next;
		if (child) {
			if (fputc(',', stream) == EOF) {
				return EOF;
			}
		}
	}
	if (fputc('}', stream) == EOF) {
		return EOF;
	}
	return 0;
}


void aJsonClass::printObject(aJsonObject *item, File stream) {
	if (item == NULL) {
		return;
	}
	aJsonObject *child = item->child;
	stream.print('{');

	while (child) {
		printStringPtr(child->name, stream);
		stream.print(':');
		printValue(child, stream);

		child = child->next;
		if (child) {
			stream.print(',');
		}
	}
	stream.print('}');
}

void aJsonClass::printObject(aJsonObject *item, WebServer& server) {
	if (item == NULL) {
		return;
	}
	aJsonObject *child = item->child;
	server.print('{');

	while (child) {
		printStringPtr(child->name, server);
		server.print(':');
		printValue(child, server);

		child = child->next;
		if (child) {
			server.print(',');
		}
	}
	server.print('}');
}

// Get Array size/item / object item.
unsigned char aJsonClass::getArraySize(aJsonObject *array) {
	aJsonObject *c = array->child;
	unsigned char i = 0;
	while (c)
		i++, c = c->next;
	return i;
}
aJsonObject*
aJsonClass::getArrayItem(aJsonObject *array, unsigned char item) {
	aJsonObject *c = array->child;
	while (c && item > 0)
		item--, c = c->next;
	return c;
}
aJsonObject*
aJsonClass::getObjectItem(aJsonObject *object, const char *string) {
	aJsonObject *c = object->child;
	while (c && strcasecmp(c->name, string))
		c = c->next;
	return c;
}

// Utility for array list handling.
void aJsonClass::suffixObject(aJsonObject *prev, aJsonObject *item) {
	prev->next = item;
	item->prev = prev;
}
// Utility for handling references.
aJsonObject*
aJsonClass::createReference(aJsonObject *item) {
	aJsonObject *ref = newItem();
	if (!ref)
		return 0;
	memcpy(ref, item, sizeof(aJsonObject));
	ref->name = 0;
	ref->type |= aJson_IsReference;
	ref->next = ref->prev = 0;
	return ref;
}

// Add item to array/object.
void aJsonClass::addItemToArray(aJsonObject *array, aJsonObject *item) {
	aJsonObject *c = array->child;
	if (!item)
		return;
	if (!c) {
		array->child = item;
	} else {
		while (c && c->next)
			c = c->next;
		suffixObject(c, item);
	}
}

void aJsonClass::addItemToObject(aJsonObject *object, const char *string,
		aJsonObject *item) {
	if (!item)
		return;

	if (item->name) {
		if (findNamePointer(item->name) == NULL) {
			free(item->name);
		}
	}

	char * namePointer = findNamePointer(const_cast<char*>(string));
	if (namePointer == NULL) {
		item->name = strdup(string);
	} else {
		item->name = namePointer;
	}

	addItemToArray(object, item);
}

void aJsonClass::addItemReferenceToArray(aJsonObject *array,
		aJsonObject *item) {
	addItemToArray(array, createReference(item));
}
void aJsonClass::addItemReferenceToObject(aJsonObject *object,
		const char *string, aJsonObject *item) {
	addItemToObject(object, string, createReference(item));
}

aJsonObject*
aJsonClass::detachItemFromArray(aJsonObject *array, unsigned char which) {
	aJsonObject *c = array->child;
	while (c && which > 0)
		c = c->next, which--;
	if (!c)
		return 0;
	if (c->prev)
		c->prev->next = c->next;
	if (c->next)
		c->next->prev = c->prev;
	if (c == array->child)
		array->child = c->next;
	c->prev = c->next = 0;
	return c;
}
void aJsonClass::deleteItemFromArray(aJsonObject *array, unsigned char which) {
	deleteItem(detachItemFromArray(array, which));
}
aJsonObject*
aJsonClass::detachItemFromObject(aJsonObject *object, const char *string) {
	unsigned char i = 0;
	aJsonObject *c = object->child;
	while (c && strcasecmp(c->name, string))
		i++, c = c->next;
	if (c)
		return detachItemFromArray(object, i);
	return 0;
}
void aJsonClass::deleteItemFromObject(aJsonObject *object, const char *string) {
	deleteItem(detachItemFromObject(object, string));
}

// Replace array/object items with new ones.
void aJsonClass::replaceItemInArray(aJsonObject *array, unsigned char which,
		aJsonObject *newitem) {
	aJsonObject *c = array->child;
	while (c && which > 0)
		c = c->next, which--;
	if (!c)
		return;
	newitem->next = c->next;
	newitem->prev = c->prev;
	if (newitem->next)
		newitem->next->prev = newitem;
	if (c == array->child)
		array->child = newitem;
	else
		newitem->prev->next = newitem;
	c->next = c->prev = 0;
	deleteItem(c);
}
void aJsonClass::replaceItemInObject(aJsonObject *object, const char *string,
		aJsonObject *newitem) {
	unsigned char i = 0;
	aJsonObject *c = object->child;
	while (c && strcasecmp(c->name, string))
		i++, c = c->next;
	if (c) {

		char * namePointer = findNamePointer(const_cast<char*>(string));
		if (namePointer == NULL) {
			newitem->name = strdup(string);
		} else {
			newitem->name = namePointer;
		}

		replaceItemInArray(object, i, newitem);
	}
}

// Create basic types:
aJsonObject*
aJsonClass::createNull() {
	aJsonObject *item = newItem();
	if (item)
		item->type = aJson_NULL;
	return item;
}

aJsonObject*
aJsonClass::createTrue() {
	aJsonObject *item = newItem();
	if (item) {
		item->type = aJson_True;
		item->valuebool = -1;
	}
	return item;
}
aJsonObject*
aJsonClass::createFalse() {
	aJsonObject *item = newItem();
	if (item) {
		item->type = aJson_False;
		item->valuebool = 0;
	}
	return item;
}
aJsonObject*
aJsonClass::createItem(char b) {
	aJsonObject *item = newItem();
	if (item) {
		item->type = b ? aJson_True : aJson_False;
		item->valuebool = b ? -1 : 0;
	}
	return item;
}

aJsonObject*
aJsonClass::createItem(int num) {
	aJsonObject *item = newItem();
	if (item) {
		item->type = aJson_Int;
		item->valueint = (int) num;
	}
	return item;
}

aJsonObject*
aJsonClass::createItem(long num) {
	aJsonObject *item = newItem();
	if (item) {
		item->type = aJson_Long;
		item->valuelong = (long) num;
	}
	return item;
}

aJsonObject*
aJsonClass::createItem(double num) {
	aJsonObject *item = newItem();
	if (item) {
		item->type = aJson_Float;
		item->valuefloat = num;
	}
	return item;
}

aJsonObject*
aJsonClass::createItem(const char *string) {
	aJsonObject *item = newItem();
	if (item) {
		item->type = aJson_String;
		item->valuestring = strdup(string);
	}
	return item;
}

aJsonObject*
aJsonClass::createArray() {
	aJsonObject *item = newItem();
	if (item)
		item->type = aJson_Array;
	return item;
}
aJsonObject*
aJsonClass::createObject() {
	aJsonObject *item = newItem();
	if (item)
		item->type = aJson_Object;
	return item;
}

// Create Arrays:
aJsonObject*
aJsonClass::createIntArray(int *numbers, unsigned char count) {
	unsigned char i;
	aJsonObject *n = 0, *p = 0, *a = createArray();
	for (i = 0; a && i < count; i++) {
		n = createItem(numbers[i]);
		if (!i)
			a->child = n;
		else
			suffixObject(p, n);
		p = n;
	}
	return a;
}

aJsonObject*
aJsonClass::createLongArray(long *numbers, unsigned char count) {
	unsigned char i;
	aJsonObject *n = 0, *p = 0, *a = createArray();
	for (i = 0; a && i < count; i++) {
		n = createItem(numbers[i]);
		if (!i)
			a->child = n;
		else
			suffixObject(p, n);
		p = n;
	}
	return a;
}

aJsonObject*
aJsonClass::createFloatArray(double *numbers, unsigned char count) {
	unsigned char i;
	aJsonObject *n = 0, *p = 0, *a = createArray();
	for (i = 0; a && i < count; i++) {
		n = createItem(numbers[i]);
		if (!i)
			a->child = n;
		else
			suffixObject(p, n);
		p = n;
	}
	return a;
}

aJsonObject*
aJsonClass::createDoubleArray(double *numbers, unsigned char count) {
	unsigned char i;
	aJsonObject *n = 0, *p = 0, *a = createArray();
	for (i = 0; a && i < count; i++) {
		n = createItem(numbers[i]);
		if (!i)
			a->child = n;
		else
			suffixObject(p, n);
		p = n;
	}
	return a;
}

aJsonObject*
aJsonClass::createStringArray(const char **strings, unsigned char count) {
	unsigned char i;
	aJsonObject *n = 0, *p = 0, *a = createArray();
	for (i = 0; a && i < count; i++) {
		n = createItem(strings[i]);
		if (!i)
			a->child = n;
		else
			suffixObject(p, n);
		p = n;
	}
	return a;
}

void aJsonClass::addNullToObject(aJsonObject* object, const char* name) {
	addItemToObject(object, name, createNull());
}

void aJsonClass::addTrueToObject(aJsonObject* object, const char* name) {
	addItemToObject(object, name, createTrue());
}

void aJsonClass::addFalseToObject(aJsonObject* object, const char* name) {
	addItemToObject(object, name, createFalse());
}

void aJsonClass::addNumberToObject(aJsonObject* object, const char* name,
		int n) {
	addItemToObject(object, name, createItem(n));
}

void aJsonClass::addNumberToObject(aJsonObject* object, const char* name,
		long n) {
	addItemToObject(object, name, createItem(n));
}

void aJsonClass::addNumberToObject(aJsonObject* object, const char* name,
		double n) {
	addItemToObject(object, name, createItem(n));
}

void aJsonClass::addStringToObject(aJsonObject* object, const char* name,
		const char* s) {
	addItemToObject(object, name, createItem(s));
}

char * aJsonClass::findNamePointer(char * name) {

	for (byte i = 0; i < this->namesLenght; i++) {

		int verb_len = strlen(name);
		if ((verb_len == strlen(this->names[i]))
				&& (strncmp(name, this->names[i], verb_len) == 0)) {
			return this->names[i];
		}
	}
	return NULL;
}

void aJsonClass::setNames(char** names, int namesLenght) {

	this->names = names;
	this->namesLenght = namesLenght;

}

void aJsonClass::push(int ch) {
	// don't allow pushing EOF
	if (ch == -1)
		return;

	m_pushback[m_pushbackDepth++] = ch;
	// can't raise error here, so just replace last char over and over
	if (m_pushbackDepth == SIZE(m_pushback))
		m_pushbackDepth = SIZE(m_pushback) - 1;
}

int aJsonClass::read(File stream) {

	if (m_pushbackDepth == 0) {
			int ch = stream.read();
			return ch;
	} else
		return m_pushback[--m_pushbackDepth];
}

//TODO conversion routines btw. float & int types?

aJsonClass aJson;
