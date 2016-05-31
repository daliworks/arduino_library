/*
 * Copyright (C) Daliworks, Inc. All rights reserved.
 * Use is subject to license terms.
 */

#ifndef THINGPLUS_H
#define THINGPLUS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Time.h>

#ifdef ESP8266
	#include <ESP8266WiFi.h>
	#include <WiFiClient.h>
#else
	#include <SPI.h>
	#include <Ethernet.h> 
#endif 

#define THINGPLUS_VALUE_MESSAGE_LEN	36
#define THINGPLUS_TOPIC_LEN		50

class ThingplusClass {
public:
	void begin(byte mac[], const char *apikey);
	void keepConnect();

	void gatewayStatusPublish(bool on, time_t durationSec);
	void sensorStatusPublish(const char *id, bool on, time_t durationSec);
	void valuePublish(const char *id, char *value);
	void valuePublish(const char *id, int value);
	void valuePublish(const char *id, float value);

	void setActuatingCallback(char* (*cb)(const char* id, const char* cmd, const char* options));

	char* (*actuatingCallback)(const char *id, const char *cmd, const char *options);
	char gw_id[13];

	PubSubClient mqtt;

	#ifdef ESP8266
		WiFiClient wifiClient;
	#else
		EthernetClient ethernet;
	#endif

private:
	const char *apikey;
	byte *mac;
	void statusPublish(const char *topic, bool on, time_t durationSec);
	void mqttOnPublish(void);

};

extern ThingplusClass Thingplus;

#endif //#ifndef THINGPLUS_H
