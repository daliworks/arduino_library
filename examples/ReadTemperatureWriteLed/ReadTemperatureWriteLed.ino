#include <Thingplus.h>

//////////////////////////////////////////////////////////////////
byte mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 		//FIXME MAC ADDRESS
const char *apikey = "";  					//FIXME APIKEY
const char *ledId = "led-000000000000-0";			//FIXME LED ID
const char *temperatureId = "temperature-000000000000-0";	//FIXME TEMPERATURE ID
//////////////////////////////////////////////////////////////////

int LED_GPIO = 8;
int TEMP_GPIO = A0;
int reportIntervalSec = 60;

char* actuatingCallback(const char *id, const char *cmd, const char *options) {
	if (strcmp(id, ledId) == 0) { 
		if (strcmp(cmd, "on") == 0) {
			digitalWrite(LED_GPIO, HIGH);
			return "success";
		}
		else  if (strcmp(cmd, "off") == 0) {
			digitalWrite(LED_GPIO, LOW);
			return "success";
		}
	}

	return NULL;
}

void setup() {
  pinMode(LED_GPIO, OUTPUT);
  Serial.begin(9600);
  Thingplus.begin(mac, apikey);
  Serial.println(Ethernet.localIP());
  Thingplus.setActuatingCallback(actuatingCallback);
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
  Thingplus.keepConnect();

  current = now();
  if (current > nextReportInterval) {
    Thingplus.gatewayStatusPublish(true, reportIntervalSec * 3);
    Thingplus.sensorStatusPublish(temperatureId, true, reportIntervalSec * 3);
    Thingplus.valuePublish(temperatureId, temperatureGet());
    nextReportInterval = current + reportIntervalSec;
  }
}
