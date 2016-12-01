# Thingplus

This is Thing+ MQTT library for **Arduino**, **ARM CORTEX-M** and **ESP8266**.<br>
You can make your own IoT application fast with Thing+<br>

Here is Thing+ arduino_libaray features.<br>

- Connect Thing+ via MQTT
- Send gateway and sensors status to Thing+
- Send sensors value to Thing+
- Receive actuator commands from Thing+
- Automatic time synchronzing

### News
- Dec.01.2016 : v1.0.8 Released
- Nov.08.2016 : v1.0.7 Released
- Sep.26.2016 : v1.0.6 Released
   - For now, Thingplus library is compatible with CORTEX-M
- Sep.19.2016 : v1.0.5 Released
- Jul.27.2016 : v1.0.4 Released
 - From 1.0.4, Arduino example uees timer library.
   - If you want to run Arduino example, You MUST donwload timer library. You can download the library from here : https://github.com/JChristensen/Timer
   - Thingplus library is still independent from Timer library. If you are not running the example, You don`t need to install the Timer library.
- Jul.20.2016 : v1.0.2 Released
- Jul.05.2016 : v1.0.1 Released
- Jun.18.2016 : Now you can install Thingplus library with Arduino library manager

### Release Note
- v1.0.8
 - esp8266 platform configuration added.
 - example modified
   - esp8266 led gpio number is change to 5.

- v1.0.7
 - "connected" method added.
 - Example modefied.
   - Example Name changed.
     - ReadTemperatureWriteLed to **ArduinoEthernet**
     - ReadTemperatureWriteLed-esp8266 to **Esp8266**
     - Sensors and actuator are added at ArduinoEthernet
 - Example added for ArduinoWizFi250

- v1.0.6
 - ARM Cortex-M compatibles added.
- v1.0.5
 - Library stabilty enhanced.
   - SRAM overflow fixed.
 - Example code modified.
   - Send actuator status periodically.
- v1.0.4
 - Bug Fixed
   - Invalid "options" argument is passing to actuator callback.
 - actuatorCallbackSet Method parameter type modified.

- v1.0.3
 - WRONG RELEASE.

- v1.0.2
 - Delete predefined target board configuration
 - Gracefully reduce SRAM usage.
 - A ardino time is syncronized with Thing+ server time instead of NTP.

- v1.0.1
 - could not send multiple sensor status fixed
 - broken subscribe method printing fixed

- v1.0.0 : Initial release

## Please visit our support page for more information
http://support.thingplus.net/en/open-hardware/arduino-noSSL-user-guide.html


### APIs
#### void begin(Client& client, byte mac[], const char* apikey)
```
   Description : Initialize the Thing+ arduino_library
   Parameter
		- client : Client class.
				   EthernetClient for Arduino
				   WiFiClent for ESP8266
		- mac : mac address
		- apikey : apikey generated by Thing+
		           You can get apikey from Thing+ Portal       
```

#### void connect(void)
```
   Description : Connect Thing+ via MQTT
                 Before call this API, ARDUINO(ESP8266) MUST CONNECT INTERNET.
```
#### bool connected(void)
```
   Description : Verify MQTT is connected or not.
```
#### void disconnect(void)
```
   Description : Disconnecting Thing+
```

#### bool loop(void)
```
   Description : Doing MQTT related works.
                 ARDUINO(ESP8266) MUST CALL THIS API PERIODICALLY
```
#### bool gatewayStatusPublish(bool on, time_t validSec)
```
   Description : Send gateway status to Thing+
   Parameter
      - on : True if gateway is on, false if gateway is off
      - validSec : Valid time. Unit is second.
                   You must resend the status before timeout.
                   If timeouted, the status is considered as error
```

#### bool sensorStatusPublish(const char* id, bool on, time_t validSec)
```
   Description : Send sensor status to Thing+
   Parameter
      - id : Sensor id.
      - on : True if sensor is on, false if sensor is off
      - validSec : Valid time. Unit is second.
                   You must resend the status before timeout.
                   If timeouted, the status is considered as error
```

#### bool valuePublish(const char *id, char *value)
```
   Description : Send sensor string value to Thing+
   Parameter
      - id : Sensor id.
      - value : Sensor value
```

#### bool valuePublish(const char *id, int value)
```
   Description : Send sensor interger value to Thing+
   Parameter
      - id : Sensor id.
      - value : Sensor value
```

#### bool valuePublish(const char *id, float value)
```
   Description : Send sensor float value to Thing+
   Parameter
      - id : Sensor id.
      - value : Sensor value
```

#### void actuatorCallbackSet(char (*cb)(const char* id, const char* cmd, JsonObject& options));
```
   Description : Register actuator callback function.
   Parameter
      - cb : Callback function pointer.
```

<br>
