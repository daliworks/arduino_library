#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timer.h>
#include <Thingplus.h>

#include <WizFi250.h>

//////////////////////////////////////////////////////////////////
const char *ssid = "";																		//FIXME
const char *password = "";         		                    //FIXME
const char *apikey = "";  																//FIXME APIKEY
const char *ledId = "led-000000000000-0";									//FIXME LED ID
const char *temperatureId = "temperature-000000000000-0";	//FIXME TEMPERATURE ID
const char *buzzerId = "buzzer-000000000000-0";						//FIXME BUZZER ID
const char *lightId = "light-000000000000-0";							//FIXME LIGHT ID
const char *noiseId = "noise-000000000000-1";							//FIXME NOISE ID
//////////////////////////////////////////////////////////////////

Timer t;

int ledOffTimer = 0;
int ledBlinkTimer = 0;

int LED_GPIO = 8;
int TEMP_GPIO = A0;
int LIGHT_GPIO = A3;
int BUZZER_GPIO = 7;
int NOISE_GPIO = A2;
int reportIntervalSec = 60;

static WiFiClient wifiClient;

static void _serialInit(void)
{
  Serial.begin(115200);
  while (!Serial);// wait for serial port to connect.

  Serial.println();
  Serial.println(F("Start"));
  Serial.println();
}

static void _wifiInit(void)
{
#define WIFI_MAX_RETRY 150

  WiFi.mode(WIFI_STA);
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

static void _gpioInit(void) {
	pinMode(LED_GPIO, OUTPUT);
	pinMode(BUZZER_GPIO, OUTPUT);
}

static void _ledOff(void) {
	t.stop(ledBlinkTimer);
	digitalWrite(LED_GPIO, LOW);
}

static void _buzzerOn(void) {
	digitalWrite(BUZZER_GPIO, HIGH);
}

static void _buzzerOff(void) {
	digitalWrite(BUZZER_GPIO, LOW);
}

char* actuatingCallback(const char *id, const char *cmd, JsonObject& options) {
	if (strcmp(id, ledId) == 0) {
		t.stop(ledBlinkTimer);
		t.stop(ledOffTimer);

		if (strcmp(cmd, "on") == 0) {
			int duration = options.containsKey("duration") ? options["duration"] : 0;

			digitalWrite(LED_GPIO, HIGH);

			if (duration)
				ledOffTimer = t.after(duration, _ledOff);

			return "success";
		}
		else  if (strcmp(cmd, "off") == 0) {
			_ledOff();
			return "success";
		}
		else  if(strcmp(cmd, "blink") == 0) {
			if (!options.containsKey("interval")) {
				Serial.println(F("[ERR] No blink interval"));
				return NULL;
			}

			ledBlinkTimer = t.oscillate(LED_GPIO, options["interval"], HIGH);

			if (options.containsKey("duration"))
				ledOffTimer = t.after(options["duration"], _ledOff);

			return "success";
		}
		else {
			return NULL;
		}
	}
	else if (strcmp(id, buzzerId) == 0) {
		if (strcmp(cmd, "on") == 0) {
			_buzzerOn();
			return "success";
		}
		else if (strcmp(cmd, "off") == 0) {
			_buzzerOff();
			return "success";
		}
		else {
			return NULL;
		}
	}

	return NULL;
}

void setup() {
  _serialInit();
  _gpioInit();

  WiFi.init();

  uint8_t mac[6];
  WiFi.macAddress(mac);

  Serial.print(F("[INFO] Gateway Id:"));
  Serial.println(WiFi.macAddress());

  _wifiInit();

  Thingplus.begin(wifiClient, mac, apikey);
  Thingplus.actuatorCallbackSet(actuatingCallback);
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
	Thingplus.loop();
	t.update();

	current = now();

	if (current > nextReportInterval) {
		Thingplus.gatewayStatusPublish(true, reportIntervalSec * 2);

		Thingplus.sensorStatusPublish(ledId, true, reportIntervalSec * 2);
		Thingplus.sensorStatusPublish(buzzerId, true, reportIntervalSec * 2);

		Thingplus.sensorStatusPublish(temperatureId, true, reportIntervalSec * 2);
		Thingplus.valuePublish(temperatureId, temperatureGet());

		Thingplus.sensorStatusPublish(lightId, true, reportIntervalSec * 2);
		Thingplus.valuePublish(lightId, lightGet());

		Thingplus.sensorStatusPublish(noiseId, true, reportIntervalSec * 2);
		Thingplus.valuePublish(noiseId, noiseGet());

		nextReportInterval = current + reportIntervalSec;
	}
}
