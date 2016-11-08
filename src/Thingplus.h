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

class ThingplusClass {
public:
	void begin(Client& client, byte mac[], const char *apikey);

	void connect(void);
	void disconnect(void);
	bool connected(void);
	bool loop(void);

	bool mqttStatusPublish(bool on);

	bool gatewayStatusPublish(bool on, time_t durationSec);
	bool sensorStatusPublish(const char *id, bool on, time_t durationSec);

	bool valuePublish(const char *id, char *value);
	bool valuePublish(const char *id, int value);
	bool valuePublish(const char *id, float value);

	void actuatorCallbackSet(char* (*cb)(const char* id, const char* cmd, JsonObject& options));

	// INTERNAL USE ONLY
	char *_actuatorDo(const char *id, const char *cmd, JsonObject& options);
	void _actuatorResultPublish(const char *messageId, char *result);
	//

private:
	char gatewayId[13];
	const char *apikey;
	PubSubClient mqtt;
	byte *mac;

	char* (*actuatorCallback)(const char *id, const char *cmd, JsonObject& options);
	bool statusPublish(const char *topic, bool on, time_t durationSec);
	bool mqttPublish(const char *topic, const char *msg);
};

extern ThingplusClass Thingplus;

#endif //#ifndef THINGPLUS_H
