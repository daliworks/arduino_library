#include <OneWire.h>
#include <DallasTemperature.h>
#include <Thingplus.h>
#include <TimeLib.h>

//FIXME WIFI SSID / PASSWORD
#include "ap_setting.h"

// using gpio pin for ds18b20
#define VCC_FOR_ONE_WIRE_BUS 5
#define ONE_WIRE_BUS 4
#define GND_FOR_ONE_WIRE_BUS 0

#define TEMPERATURE_PRECISION 9

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

//////////////////////////////////////////////////////////////////
//byte mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};     //FIXME MAC ADDRESS
const char *apikey = "";  	                              //FIXME APIKEY
const char *ledId = "led-xxxxxxxxxxxx-0";			            //FIXME LED ID
const char *temperatureId = "temperature-xxxxxxxxx-0";    //FIXME TEMPERATURE ID
//////////////////////////////////////////////////////////////////

int LED_GPIO = 2;
int reportIntervalSec = 60;

char* actuatingCallback(const char *id, const char *cmd, const char *options) {
  if (strcmp(id, ledId) == 0) {
    if (strcmp(cmd, "on") == 0) {
      //digitalWrite(LED_GPIO, HIGH);
      // board has inverted led
      digitalWrite(LED_GPIO, LOW);
      return "success";
    }
    else  if (strcmp(cmd, "off") == 0) {
      //digitalWrite(LED_GPIO, LOW);
      digitalWrite(LED_GPIO, HIGH);
      return "success";
    }
  }

  return NULL;
}

void wifi_connect() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int Attempt = 1;
  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(". ");
    Serial.print(Attempt);
    if ( Attempt % 20 == 0 ) {
      Serial.println();
    }
    delay(100);
    Attempt++;
    if (Attempt == 150) {
      Serial.println();
      Serial.println("-----> Could not connect to WIFI");
      while (1) {
        yield();
      }
    }
  }

  Serial.println();
  Serial.print("===> WiFi connected");
  Serial.print(" ------> IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(LED_GPIO, OUTPUT);

  // ds18b20
  pinMode(VCC_FOR_ONE_WIRE_BUS, OUTPUT);
  pinMode(GND_FOR_ONE_WIRE_BUS, OUTPUT);

  digitalWrite(VCC_FOR_ONE_WIRE_BUS, HIGH);
  digitalWrite(GND_FOR_ONE_WIRE_BUS, LOW);

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect.
  }

  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) {
    Serial.println("sensor fail");
  }
  sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);

  uint8_t mac[6];
  WiFi.macAddress(mac);

  if (String(apikey) == "") {

    Serial.println();
    Serial.println();
    Serial.println("Please register this device");
    Serial.print("Wifi mac address is : ");
    Serial.println(macToStr(mac));
    Serial.flush();
    while (1) {
      yield();
    }
  }
  wifi_connect();

  Thingplus.begin(mac, apikey);
  Serial.println(WiFi.localIP());
  Thingplus.setActuatingCallback(actuatingCallback);
}

time_t current;
time_t nextReportInterval = now();

float temperatureGet() {
  float temperature;
  sensors.requestTemperatures();
  temperature = sensors.getTempC(insideThermometer);
  Serial.print("Temp is : ");
  Serial.println(temperature);
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

String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    //if (i < 5)
    //  result += '-';
  }
  return result;
}
