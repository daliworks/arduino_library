/*
 * Copyright (C) Daliworks, Inc. All rights reserved.
 * Use is subject to license terms.
 */
#include "Arduino.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Timelib.h>

#ifdef ESP8266
	#include <ESP8266WiFi.h>
	#include <WiFiClient.h>
#else
	#include <SPI.h>
	#include <Ethernet.h> 
#endif

#include "Thingplus.h"
#include "Ntp.h"

const char *server = "dmqtt.thingplus.net";
const int port = 1883;

ThingplusClass Thingplus;


void callbackSubscribe(char *topic, uint8_t *payload, unsigned int length) {
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject((char*)payload);
	if (!root.success()) {
		Serial.println("parseObject() failed");
		return;
	}

	const char *id = root["id"];
	const char *method = root["method"];

	if (Thingplus.actuatingCallback && strcmp(method, "controlActuator") == 0) {
		char responseTopic[32] = {0,};
		sprintf(responseTopic, "v/a/g/%s/res", Thingplus.gw_id);

		char resposeMessage[64] = {0,};

		const char *result = Thingplus.actuatingCallback(root["params"]["id"], root["params"]["cmd"], root["params"]["options"]);
		if (result)
			sprintf(resposeMessage, "{\"id\":\"%s\",\"result\":\"%s\"}", id, result);
		else
			sprintf(resposeMessage, "{\"id\":\"%s\",\"error\": {\"code\": -32000}}", id);
		Thingplus.mqtt.publish(responseTopic, resposeMessage);
	}
}

void ThingplusClass::begin(byte mac[], const char *apikey) {
	this->mac = mac;
	sprintf(this->gw_id, "%02x%02x%02x%02x%02x%02x", 
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	this->apikey = apikey;

	#ifdef ESP8266
      	Serial.println();
	#else
		Ethernet.begin(this->mac);
		delay(1500);
	#endif

	NtpBegin();
	NtpSync();

	#ifdef ESP8266
		this->mqtt.setCallback(callbackSubscribe);
		this->mqtt.setClient(this->wifiClient);
		this->mqtt.setServer(server, port);		
	#else
		this->mqtt.setCallback(callbackSubscribe);
		this->mqtt.setClient(this->ethernet);
		this->mqtt.setServer(server, port);	
	#endif
}

void ThingplusClass::mqttOnPublish() {
	char topic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(topic, "v/a/g/%s/mqtt/status", this->gw_id);

	this->mqtt.publish(topic, "on");
}

void ThingplusClass::keepConnect() {
	while (!this->mqtt.connected()) {
		Serial.println("Attempting MQTT connection...");

		char willTopic[32] =  {0,};
		sprintf(willTopic, "v/a/g/%s/mqtt/status", this->gw_id);
		if (this->mqtt.connect(this->gw_id, this->gw_id, this->apikey, willTopic, 1, true, "err")) 
		{
			Serial.println("connected");

			char subscribeTopic[32] = {0,};
			sprintf(subscribeTopic, "v/a/g/%s/req", this->gw_id);
			this->mqtt.subscribe(subscribeTopic);
			this->mqttOnPublish();
			break;
		}
		else
		{
			Serial.print("MQTT connection failed, rc=");
			Serial.print(this->mqtt.state());
			Serial.println(" try again in 5 seconds");
			delay(5000);
		}
	}

	this->mqtt.loop();
}

void ThingplusClass::setActuatingCallback(char* (*cb)(const char* id, const char* cmd, const char* options)) {
	this->actuatingCallback = cb;
}

void ThingplusClass::statusPublish(const char *topic, bool on, time_t durationSec) {
	time_t current = now();
	char status[16] = {0,};
	sprintf(status, "%s,%ld000", on ? "on" : "off", current + durationSec);

	this->mqtt.publish(topic, status);
}

void ThingplusClass::sensorStatusPublish(const char *id, bool on, time_t durationSec) {
	char statusPublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(statusPublishTopic, "v/a/g/%s/s/%s/status", this->gw_id, id);

	this->statusPublish(statusPublishTopic, on, durationSec);
}

void ThingplusClass::gatewayStatusPublish(bool on, time_t durationSec) {
	char statusPublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(statusPublishTopic, "v/a/g/%s/status", this->gw_id);

	this->statusPublish(statusPublishTopic, on, durationSec);
}

void ThingplusClass::valuePublish(const char *id, char *value) {
	char valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(valuePublishTopic, "v/a/g/%s/s/%s", this->gw_id, id);

	time_t current = now();
	char v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	sprintf(v, "%ld000,%s", current, value);

	this->mqtt.publish(valuePublishTopic, v);
}

void ThingplusClass::valuePublish(const char *id, float value) {
	char valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(valuePublishTopic, "v/a/g/%s/s/%s", this->gw_id, id);

	time_t current = now();
	char v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	sprintf(v, "%ld000,", current);
	dtostrf(value, 5, 2, &v[strlen(v)]);

	this->mqtt.publish(valuePublishTopic, v);
}

void ThingplusClass::valuePublish(const char *id, int value) {
	char valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(valuePublishTopic, "v/a/g/%s/s/%s", this->gw_id, id);

	time_t current = now();
	char v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	sprintf(v, "%ld000,%d", current, value);

	this->mqtt.publish(valuePublishTopic, v);
}

