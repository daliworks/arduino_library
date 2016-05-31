#include <Thingplus.h>
#include <TimeLib.h>

//FIXME WIFI SSID / PASSWORD
#include "ap_setting.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

//////////////////////////////////////////////////////////////////
//byte mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};     //FIXME MAC ADDRESS
const char *apikey = "";  					              //FIXME APIKEY
const char *ledId = "led-000000000000-0";			      //FIXME LED ID
const char *temperatureId = "temperature-000000000000-0"; //FIXME TEMPERATURE ID
//////////////////////////////////////////////////////////////////

int LED_GPIO = 5;
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

void wifi_connect() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int Attempt = 0;
  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(". ");
    Serial.print(Attempt);
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
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect.
  }

  uint8_t mac[6];
  WiFi.macAddress(mac);

  if (String(apikey) == "") {

    Serial.println();
    Serial.println();
    Serial.println("Please register this device");
    Serial.print("Wifi mac address is : ");
    Serial.println(macToStr(mac));
    Serial.flush();
    delay(1000);
    ESP.reset();
  }
  wifi_connect();
  Thingplus.begin(mac, apikey);
  Serial.println(WiFi.localIP());
  Thingplus.setActuatingCallback(actuatingCallback);
}

time_t current;
time_t nextReportInterval = now();

float temperatureGet() {
  int B = 3975;
  float temperature;
  float resistance;
  int a = analogRead(TEMP_GPIO);
  resistance = (float)(1023 - a) * 10000 / a; //get the resistance of the sensor;
  temperature = 1 / (log(resistance / 10000) / B + 1 / 298.15) - 273.15; //convert to temperature via datasheet;
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
