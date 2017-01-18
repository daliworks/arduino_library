/* WARNING
 * This example only sends sensors value. 
 * CAN NOT receive actuator commands from Thing+.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Thingplus.h>
#include <WiFiEsp.h>

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#endif

//////////////////////////////////////////////////////////////////
const char *ssid = "";																		//FIXME
const char *password = "";         		                    //FIXME
const uint8_t mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //FIXME
const char *apikey = "";																	//FIXME
const char *temperatureId = "temperature-000000000000-0";	//FIXME TEMPERATURE ID
const char *lightId = "light-000000000000-0";							//FIXME LIGHT ID
const char *noiseId = "noise-000000000000-1";							//FIXME NOISE ID
//////////////////////////////////////////////////////////////////

int TEMP_GPIO = A0;
int LIGHT_GPIO = A3;
int NOISE_GPIO = A2;
int reportIntervalSec = 60;

static WiFiEspClient wifiClient;

static void _serialInit(void)
{
	Serial.begin(9600);

	while (!Serial);// wait for serial port to connect.

	Serial.println();
	Serial.println(F("Start"));
	Serial.println();
}

static void _wifiInit(void)
{
	Serial1.begin(9600);
	WiFi.init(&Serial1);

	if (WiFi.status() == WL_NO_SHIELD) {
		Serial.println("WiFi shield not present");
		while (true);
	}

#define WIFI_MAX_RETRY 150

	WiFi.begin((char*)ssid, password);

	Serial.print(F("[INFO] WiFi connecting to "));
	Serial.println(ssid);

	int retry = 0;
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(retry++);
		Serial.print(F("."));
		if (!(retry % 50))
			Serial.println();
		if (retry == WIFI_MAX_RETRY) {
			Serial.println();
			Serial.print(F("[ERR] WiFi connection failed. SSID:"));
			Serial.print(ssid);
			Serial.print(F(" PASSWD:"));
			Serial.println(password);
			while (1) {
				yield();
			}
		}
		delay(100);
	}

	Serial.println();
	Serial.print(F("[INFO] WiFi connected"));
	Serial.print(F(" IP:"));
	Serial.println(WiFi.localIP());
}

void setup() {
	_serialInit();
	_wifiInit();
	ntpSync();

	Thingplus.begin(wifiClient, mac, apikey);
	Thingplus.connect();
}

time_t current;
time_t nextReportInterval = now();

float temperatureGet() {
	int B = 3975;
	float temperature;
	float resistance;
	int a = analogRead(TEMP_GPIO);

	resistance = (float)(1023-a)*10000/a; //get the resistance of the sensor;
	temperature = 1/(log(resistance/10000)/B+1/298.15)-273.15;//convert to temperature via datasheet;
	return temperature;
}

int lightGet() {
	int light = analogRead(LIGHT_GPIO);
	return light;
}

int noiseGet() {
	int noise = analogRead(NOISE_GPIO);
	return noise;
}

void loop() {
	current = now();

	if (current > nextReportInterval) {
		Thingplus.gatewayStatusPublish(true, reportIntervalSec * 2);

		Thingplus.sensorStatusPublish(temperatureId, true, reportIntervalSec * 2);
		Thingplus.valuePublish(temperatureId, temperatureGet());

		Thingplus.sensorStatusPublish(lightId, true, reportIntervalSec * 2);
		Thingplus.valuePublish(lightId, lightGet());

		Thingplus.sensorStatusPublish(noiseId, true, reportIntervalSec * 2);
		Thingplus.valuePublish(noiseId, noiseGet());

		nextReportInterval = current + reportIntervalSec;
	}
}
