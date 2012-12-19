#include "SPI.h"
#include "Ethernet.h"
#include "SD.h"
#include "aJSON.h"
#include "WebServer.h"


//define range of pins you want to control
#define FROM_PIN 55
#define TO_PIN 58

byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
aJsonObject* pins;
WebServer webserver("", 80);

File file;

void indexCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
	server.httpSuccess();
	if (type != WebServer::HEAD) {
		P(helloMsg) =
				"<!DOCTYPE html>\n<html lang=\"en\">\n"
				"<head>\n"
				"\t<meta charset=\"utf-8\">\n"
				"\t<title>Pin Toggle</title>\n"
				"\t<link href=\"http://www.cs.helsinki.fi/u/ljlukkar/pincontrol/css/style.css\" rel=\"stylesheet\">\n"
				"\t<link href=\'http://fonts.googleapis.com/css?family=Open+Sans+Condensed:700,300\' rel=\'stylesheet\' type=\'text/css\'/>\n"
				"</head>\n"
				"<body>\n"
				"<div class=\"container\">\n"
				"\t<div id=\"pin-list\">\n"
				"\t</div>\n"
				"</div>\n"
				"<script src=\"http://www.cs.helsinki.fi/u/ljlukkar/pincontrol/js/jquery.min.js\"></script>\n"
				"<script src=\"http://www.cs.helsinki.fi/u/ljlukkar/pincontrol/js/lodash.min.js\"></script>\n"
				"<script src=\"http://www.cs.helsinki.fi/u/ljlukkar/pincontrol/js/lib/backbone-min.js\"></script>\n"
				"<script src=\"http://www.cs.helsinki.fi/u/ljlukkar/pincontrol/js/models/pin.js\"></script>\n"
				"<script src=\"http://www.cs.helsinki.fi/u/ljlukkar/pincontrol/js/collections/pins.js\"></script>\n"
				"<script src=\"http://www.cs.helsinki.fi/u/ljlukkar/pincontrol/js/views/pins.js\"></script>\n"
				"<script src=\"http://www.cs.helsinki.fi/u/ljlukkar/pincontrol/js/views/app.js\"></script>\n"
				"<script src=\"http://www.cs.helsinki.fi/u/ljlukkar/pincontrol/js/app.js\"></script>\n</body>\n"
				"</html>";
		server.printP(helloMsg);
	}
}

// this method is performed before the reguest is prosessessed and default responce is generated
// server.setPreventDefault(true); disables the default prosessing and responce once
void switchBeforeResposeCmd(WebServer &server, WebServer::ConnectionType type, char * tail, bool tailComplete,
		aJsonObject ** collection, aJsonObject ** model) {
 	if (type == WebServer::PUT) {
		if (model != NULL) {
			pinMode(aJson.getObjectItem(*model, "pin")->valueint, (bool) aJson.getObjectItem(*model, "low")->valuebool);
		}
	}


}

// this method is performed after the responce is sent so it doens't block it anymore
void switchAfterResposeCmd(WebServer::ConnectionType type, aJsonObject ** collection, aJsonObject ** model) {
	pinMode(53, OUTPUT);

	if (SD.exists("pin.txt")) {
		SD.remove("pin.txt");
	}

	file = SD.open("pin.txt", FILE_WRITE);
	aJson.print(&**collection, file);
	file.close();
}

void setup() {
	Serial.begin(9600);
	pinMode(53, OUTPUT);

	if (!SD.begin(4)) {
		Serial.println("SD initialization failed!");
	}

	if (SD.exists("pin.txt")) { //load settings file if found
		file = SD.open("pin.txt");
		pins = aJson.parse(file);
		file.close();
	} else { //if not found populate dta
		pins = aJson.createArray();

		for (int i = FROM_PIN; i < TO_PIN + 1; i++) {
			pinMode(i, LOW);
			pinMode(i, OUTPUT);
			aJsonObject* pin = aJson.createObject();
			aJson.addItemToObject(pin, "id", aJson.createItem(i - FROM_PIN + 1));
			aJson.addItemToObject(pin, "pin", aJson.createItem(i));
			aJson.addItemToObject(pin, "low", aJson.createItem(false));
			aJson.addItemToArray(pins, pin);
		}
	}

	//add binding to webserver and set actions
	webserver.addJSONBinding("pins", &pins);
	webserver.setBeforeRequest("pins", &switchBeforeResposeCmd);
	webserver.setAfterRequest("pins", &switchAfterResposeCmd);

	webserver.setDefaultCommand(&indexCmd);

	Ethernet.begin(mac);
	webserver.begin();

	for (byte i = 0; i < 4; i++) {
		Serial.print(Ethernet.localIP()[i]);
		if (i < 3) {
			Serial.print('.');
		}
	}
	Serial.println();
}

void loop() {
	webserver.processConnection();
}

