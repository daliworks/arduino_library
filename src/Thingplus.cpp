/*
 * Copyright (C) Daliworks, Inc. All rights reserved.
 * Use is subject to license terms.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Time.h>

#include "Thingplus.h"

#define THINGPLUS_VALUE_MESSAGE_LEN	36
#define THINGPLUS_TOPIC_LEN		80
#define THINGPLUS_SHORT_TOPIC_LEN	32

void serverTimeSync(const char *serverTimeMs) {
#define TIME_LENGTH 10
	char serverTimeSec[TIME_LENGTH+1] = {0,};

	strncpy(serverTimeSec, serverTimeMs, TIME_LENGTH);

	int i;
	time_t syncTime = 0;
	for (i=0; i<TIME_LENGTH; i++)
		syncTime = (10 * syncTime) + (serverTimeSec[i] - '0');

	setTime(syncTime);
	Serial.print(F("INFO: Time Synced. NOW:"));
	Serial.println(now());
}

void mqttSubscribeCallback(char *topic, uint8_t *payload, unsigned int length) {
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject((char*)payload);
	if (!root.success()) {
		Serial.println(F("parseObject() failed"));
		return;
	}

	const char *id = root["id"];
	const char *method = root["method"];

	Serial.print(F("INFO: Message subscribed. method:"));
	Serial.println(method);
	Serial.flush();

	if (strcmp(method, "controlActuator") == 0) {
		JsonObject& options = root["params"]["options"].as<JsonObject&>();
		char *result = Thingplus._actuatorDo(root["params"]["id"], root["params"]["cmd"], options);
		Thingplus._actuatorResultPublish(id, result);
	}
	else if (strcmp(method, "timeSync") == 0) {
		serverTimeSync(root["params"]["time"]);
	}
}

bool ThingplusClass::mqttPublish(const char *topic, const char *payload) {
	if (!this->mqtt.publish(topic, payload, strlen(payload))) {
		Serial.println(F("ERR Publish failed."));

		Serial.print(F("TOPIC:"));
		Serial.println(topic);

		Serial.print(F("PAYLOAD:"));
		Serial.println(payload);

		return false;
	}
	return true;
}

void ThingplusClass::_actuatorResultPublish(const char *messageId, char *result) {
	char responseTopic[THINGPLUS_SHORT_TOPIC_LEN] = {0,};
	snprintf_P(responseTopic, sizeof(responseTopic), PSTR("v/a/g/%s/res"), this->gatewayId);

	char responseMessage[64] = {0,};
	if (result)
		snprintf_P(responseMessage, sizeof(responseMessage), PSTR("{\"id\":\"%s\",\"result\":\"%s\"}"), messageId, result);
	else
		snprintf_P(responseMessage, sizeof(responseMessage), PSTR("{\"id\":\"%s\",\"error\": {\"code\": -32000}}"), messageId);

	this->mqttPublish(responseTopic, responseMessage);
}

char *ThingplusClass::_actuatorDo(const char *id, const char *cmd, JsonObject& options) {
	if (!this->actuatorCallback) {
		return NULL;
	}

	return this->actuatorCallback(id, cmd, options);
}

void ThingplusClass::actuatorCallbackSet(char* (*cb)(const char* id, const char* cmd, JsonObject& options)) {
	this->actuatorCallback = cb;
}

bool ThingplusClass::valuePublish(const char *id, char *value) {
	char valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	snprintf_P(valuePublishTopic, sizeof(valuePublishTopic), PSTR("v/a/g/%s/s/%s"), this->gatewayId, id);

	char v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	snprintf_P(v, sizeof(v), PSTR("%ld000,%s"), now(), value);

	return this->mqttPublish(valuePublishTopic, v);
}

bool ThingplusClass::valuePublish(const char *id, float value) {
	char valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	snprintf_P(valuePublishTopic, sizeof(valuePublishTopic), PSTR("v/a/g/%s/s/%s"), this->gatewayId, id);

	char v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	snprintf_P(v, sizeof(v), PSTR("%ld000,"), now());
	dtostrf(value, 5, 2, &v[strlen(v)]);

	return this->mqttPublish(valuePublishTopic, v);
}

bool ThingplusClass::valuePublish(const char *id, int value) {
	char valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	snprintf_P(valuePublishTopic, sizeof(valuePublishTopic), PSTR("v/a/g/%s/s/%s"), this->gatewayId, id);

	char v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	snprintf_P(v, sizeof(v), PSTR("%ld000,%d"), now(), value);

	return this->mqttPublish(valuePublishTopic, v);
}

bool ThingplusClass::statusPublish(const char *topic, bool on, time_t durationSec) {
	char status[20] = {0,};
	snprintf_P(status, sizeof(status), PSTR("%s,%ld000"), on ? "on" : "off", now() + durationSec);

	return this->mqttPublish(topic, status);
}

bool ThingplusClass::sensorStatusPublish(const char *id, bool on, time_t durationSec) {
	char statusPublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	snprintf_P(statusPublishTopic, sizeof(statusPublishTopic), PSTR("v/a/g/%s/s/%s/status"), this->gatewayId, id);

	return this->statusPublish(statusPublishTopic, on, durationSec);
}

bool ThingplusClass::gatewayStatusPublish(bool on, time_t durationSec) {
	char statusPublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	snprintf_P(statusPublishTopic, sizeof(statusPublishTopic), PSTR("v/a/g/%s/status"), this->gatewayId);

	return this->statusPublish(statusPublishTopic, on, durationSec);
}

bool ThingplusClass::mqttStatusPublish(bool on) {
	char topic[THINGPLUS_TOPIC_LEN] = {0,};
	snprintf_P(topic, sizeof(topic), PSTR("v/a/g/%s/mqtt/status"), this->gatewayId);

	const char *message = on ? "on" : "off";

	return this->mqttPublish(topic, message);
}

bool ThingplusClass::loop(void) {
	if (!this->mqtt.connected()) {
		Serial.println(F("[ERR] MQTT disconnected"));
		this->connect();
	}

	return this->mqtt.loop();
}

void ThingplusClass::connect(void) {
#define MQTT_RETRY_INTERVAL_MS	(5 * 1000)

	bool ret;
	char willTopic[32] =  {0,};
	snprintf_P(willTopic, sizeof(willTopic), PSTR("v/a/g/%s/mqtt/status"), this->gatewayId);

	do 
	{
		ret = this->mqtt.connect(this->gatewayId, this->gatewayId, 
				this->apikey, willTopic, 1, true, "err");
		if (!ret) {
			Serial.print(F("ERR: MQTT connection failed."));

			int errCode = this->mqtt.state();
			if (errCode == 5)
				Serial.println(F(" APIKEY ERROR"));
			else
				Serial.println(this->mqtt.state());
			Serial.println(F("ERR: Retry"));
			delay(MQTT_RETRY_INTERVAL_MS);
		}
	} while(!this->mqtt.connected());

	char subscribeTopic[32] = {0,};
	snprintf_P(subscribeTopic, sizeof(subscribeTopic), PSTR("v/a/g/%s/req"), this->gatewayId);
	this->mqtt.subscribe(subscribeTopic);

	this->mqttStatusPublish(true);

	Serial.println(F("INFO: MQTT Connected"));
}

void ThingplusClass::disconnect(void) {
	this->mqtt.disconnect();
	Serial.println(F("INFO: MQTT Disconnected"));
}

void ThingplusClass::begin(Client& client, byte mac[], const char *apikey)
{
	const char *server = "dmqtt.thingplus.net";
	const int port = 1883;

	this->mac = mac;
	snprintf_P(this->gatewayId, sizeof(this->gatewayId), PSTR("%02x%02x%02x%02x%02x%02x"),
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	this->apikey = apikey;

	this->mqtt.setCallback(mqttSubscribeCallback);
	this->mqtt.setServer(server, port);
	this->mqtt.setClient(client);
}

ThingplusClass Thingplus;
