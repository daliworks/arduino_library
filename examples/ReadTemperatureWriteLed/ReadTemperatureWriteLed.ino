#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h> 

#include <Timer.h>
#include <Thingplus.h>

//////////////////////////////////////////////////////////////////
byte mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 		//FIXME MAC ADDRESS
const char *apikey = "";  					//FIXME APIKEY
const char *ledId = "led-000000000000-0";			//FIXME LED ID
const char *temperatureId = "temperature-000000000000-0";	//FIXME TEMPERATURE ID
//////////////////////////////////////////////////////////////////

Timer t;

int ledOffTimer = 0;
int ledBlinkTimer = 0;

int LED_GPIO = 8;
int TEMP_GPIO = A0;
int reportIntervalSec = 60;

static EthernetClient ethernetClient;

static void _serialInit(void)
{
	Serial.begin(115200);
	while (!Serial);// wait for serial port to connect.
	Serial.println();
}

static void _ethernetInit(void) {
	Ethernet.begin(mac);
	Serial.print("[INFO] IP:");
	Serial.println(Ethernet.localIP());
}

static void _gpioInit(void) {
	pinMode(LED_GPIO, OUTPUT);
}

static void _ledOff() {
	t.stop(ledBlinkTimer);
	digitalWrite(LED_GPIO, LOW);
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

	return NULL;
}

void setup() {
	_serialInit();
	_ethernetInit();
	_gpioInit();

	Thingplus.begin(ethernetClient, mac, apikey);
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

	resistance=(float)(1023-a)*10000/a; //get the resistance of the sensor;
	temperature=1/(log(resistance/10000)/B+1/298.15)-273.15;//convert to temperature via datasheet;
	return temperature;
}

void loop() {
	Thingplus.loop();
	t.update();

	current = now();

	if (current > nextReportInterval) {
		Thingplus.gatewayStatusPublish(true, reportIntervalSec * 3);
		Thingplus.sensorStatusPublish(temperatureId, true, reportIntervalSec * 3);
		Thingplus.sensorStatusPublish(ledId, true, reportIntervalSec * 3);
		Thingplus.valuePublish(temperatureId, temperatureGet());
		nextReportInterval = current + reportIntervalSec;
	}
}
