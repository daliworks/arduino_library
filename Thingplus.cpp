/*
 * Copyright (C) Daliworks, Inc. All rights reserved.
 * Use is subject to license terms.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Time.h>

#include "Ntp.h"
#include "Thingplus.h"

#define THINGPLUS_VALUE_MESSAGE_LEN	36
#define THINGPLUS_TOPIC_LEN		50
#define THINGPLUS_SHORT_TOPIC_LEN	32

const char *server = "dmqtt.thingplus.net";
const int port = 1883;

void mqttSubscribeCallback(char *topic, uint8_t *payload, unsigned int length)
{
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject((char*)payload);
	if (!root.success()) {
		Serial.println("parseObject() failed");
		return;
	}

	const char *id = root["id"];
	const char *method = root["method"];

	Serial.print("Message subscribed. method:");
	Serial.println(method);

	if (strcmp(method, "controlActuator") == 0) {
		char *result = Thingplus._actuatorDo(root["params"]["id"], root["params"]["cmd"], root["params"]["options"]);
		Thingplus._actuatorResultPublish(id, result);
	}
}

void ThingplusClass::_actuatorResultPublish(const char *messageId, char *result)
{
	char responseTopic[THINGPLUS_SHORT_TOPIC_LEN] = {0,};
	sprintf(responseTopic, "v/a/g/%s/res", this->gatewayId);

	char resposeMessage[64] = {0,};
	if (result)
		sprintf(resposeMessage, "{\"id\":\"%s\",\"result\":\"%s\"}", messageId, result);
	else
		sprintf(resposeMessage, "{\"id\":\"%s\",\"error\": {\"code\": -32000}}", messageId);
	this->mqtt.publish(responseTopic, resposeMessage);
}

char *ThingplusClass::_actuatorDo(const char *id, const char *cmd, const char *options)
{
	if (!this->actuatorCallback) {
		return NULL;
	}

	return this->actuatorCallback(id, cmd, options);
}

void ThingplusClass::actuatorCallbackSet(char* (*cb)(const char* id, const char* cmd, const char* options))
{
	this->actuatorCallback = cb;
}

bool ThingplusClass::valuePublish(const char *id, char *value) 
{
	char valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(valuePublishTopic, "v/a/g/%s/s/%s", this->gatewayId, id);

	time_t current = now();
	char v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	sprintf(v, "%ld000,%s", current, value);

	return this->mqtt.publish(valuePublishTopic, v);
}

bool ThingplusClass::valuePublish(const char *id, float value) 
{
	char valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(valuePublishTopic, "v/a/g/%s/s/%s", this->gatewayId, id);

	time_t current = now();
	char v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	sprintf(v, "%ld000,", current);
	dtostrf(value, 5, 2, &v[strlen(v)]);

	return this->mqtt.publish(valuePublishTopic, v);
}

bool ThingplusClass::valuePublish(const char *id, int value) 
{
	char valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(valuePublishTopic, "v/a/g/%s/s/%s", this->gatewayId, id);

	time_t current = now();
	char v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	sprintf(v, "%ld000,%d", current, value);

	return this->mqtt.publish(valuePublishTopic, v);
}

bool ThingplusClass::statusPublish(const char *topic, bool on, time_t durationSec)
{
	time_t current = now();
	char status[16] = {0,};
	sprintf(status, "%s,%ld000", on ? "on" : "off", current + durationSec);

	return this->mqtt.publish(topic, status);
}

bool ThingplusClass::sensorStatusPublish(const char *id, bool on, time_t durationSec)
{
	char statusPublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(statusPublishTopic, "v/a/g/%s/s/%s/status", this->gatewayId, id);

	return this->statusPublish(statusPublishTopic, on, durationSec);
}

bool ThingplusClass::gatewayStatusPublish(bool on, time_t durationSec)
{
	char statusPublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(statusPublishTopic, "v/a/g/%s/status", this->gatewayId);

	return this->statusPublish(statusPublishTopic, on, durationSec);
}

bool ThingplusClass::mqttStatusPublish(bool on)
{
	char topic[THINGPLUS_TOPIC_LEN] = {0,};
	sprintf(topic, "v/a/g/%s/mqtt/status", this->gatewayId);

	const char *message = on ? "on" : "off";

	return this->mqtt.publish(topic, message);
}

bool ThingplusClass::loop(void)
{
	if (!this->mqtt.connected()) {
		Serial.println("[ERR] MQTT disconnected");
		this->connect();
	}

	return this->mqtt.loop();
}

void ThingplusClass::connect(void)
{
#define MQTT_RETRY_INTERVAL_MS	(5 * 1000)

	bool ret;
	char willTopic[32] =  {0,};
	sprintf(willTopic, "v/a/g/%s/mqtt/status", this->gatewayId);

	do 
	{
		ret = this->mqtt.connect(this->gatewayId, this->gatewayId, 
				this->apikey, willTopic, 1, true, "err");
		if (!ret) {
			Serial.print("ERR: MQTT connection failed.");

			int errCode = this->mqtt.state();
			if (errCode == 5)
				Serial.println(" APIKEY ERROR");
			else
				Serial.println(this->mqtt.state());
			Serial.println("ERR: Retry");
			delay(MQTT_RETRY_INTERVAL_MS);
		}
	} while(!this->mqtt.connected());

	char subscribeTopic[32] = {0,};
	sprintf(subscribeTopic, "v/a/g/%s/req", this->gatewayId);
	this->mqtt.subscribe(subscribeTopic);

	this->mqttStatusPublish(true);

	Serial.println("INFO: MQTT Connected");
}

void ThingplusClass::disconnect(void)
{
	this->mqtt.disconnect();
	Serial.println("INFO: MQTT Disconnected");
}

void ThingplusClass::begin(Client& client, byte mac[], const char *apikey)
{
	this->mac = mac;
	sprintf(this->gatewayId, "%02x%02x%02x%02x%02x%02x", 
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	this->apikey = apikey;

	this->mqtt.setCallback(mqttSubscribeCallback);
	this->mqtt.setServer(server, port);		
	this->mqtt.setClient(client);

	NtpBegin();
	NtpSync();
}

ThingplusClass Thingplus;
