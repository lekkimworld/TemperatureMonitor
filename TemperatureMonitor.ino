#include "WiFiEsp.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// ***** Temperature sensors *****
// Data wire is plugged into pin 10 on the Arduino
#define ONE_WIRE_BUS 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress thermColdWater =  {0x28, 0xFF, 0x18, 0x89, 0xB3, 0x16, 0x04, 0x3F};  
DeviceAddress thermWarmWater =  {0x28, 0xFF, 0xB2, 0x1B, 0xB3, 0x16, 0x05, 0x09};
DeviceAddress thermOutside =    {0x28, 0xff, 0x46, 0xC7, 0x60, 0x17, 0x05, 0x9A};
DeviceAddress thermPumpOut =    {0x28, 0xFF, 0x5D, 0x1A, 0x80, 0x14, 0x02, 0x1B};
DeviceAddress thermPumpReturn = {0x28, 0xFF, 0x18, 0x1A, 0x80, 0x14, 0x02, 0x20};
float tempColdWater = 0.0000f;
float tempWarmWater = 0.0000f;
float tempOutside = 0.0000f;
float tempPumpOut = 0.0000f;
float tempPumpReturn = 0.0000f;

unsigned long lastReadTime = millis();      // when we last read temps (to avoid call to delay)
unsigned long lastPrintTime = millis();     // when we last printed temps (to avoid call to delay)
#define DELAY_TEMP_READ 5000      // delay for measuring temp (ms)
#define DELAY_TEMP_PRINT 60000L    // delay for printing temp to serial (ms)
#define TEMP_DECIMALS 4           // 4 decimals of output
#define TEMPERATURE_PRECISION 12  // 12 bits precision

// **** WiFi *****
const char ssid[] = "MM_IoT";               // your network SSID (name)
const char pass[] = "chippo-lekkim";        // your network password
const char server[] = "desolate-meadow-68880.herokuapp.com";
#define DELAY_POST_DATA 60000L              // delay between updates, in milliseconds

int status = WL_IDLE_STATUS;      // the Wifi radio's status
unsigned long lastConnectTime = millis();         // last time you connected to the server, in milliseconds

// Initialize the Ethernet client object
WiFiEspClient client;

void setup() {
  // initialize serial
  Serial.begin(115200);
  Serial1.begin(9600);

  // initialize temp sensors
  initTempSensors();
  
  // init wifi
  initWifi();
}

void loop() {
  if ((((unsigned long)millis()) - lastReadTime) >= DELAY_TEMP_READ) {
    readTemperatures();
    lastReadTime = millis();
  }
  if ((((unsigned long)millis()) - lastPrintTime) >= DELAY_TEMP_PRINT) {
    printTemperatures();
    lastPrintTime = millis();
  }
  if ((((unsigned long)millis()) - lastConnectTime) >= DELAY_POST_DATA) {
    postData();
    lastConnectTime = millis();
  }
  
  // if there's incoming data from the net connection send it out the serial port
  // this is for debugging purposes only
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
}

// this method makes a HTTP connection to the server
void postData() {
  // close any connection before send a new request
  // this will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection
  if (client.connect(server, 80)) {
    Serial.println("Connecting...");

    // prepare content
    String content = String("[{\"sensorId\": \"") + deviceAddressToString(thermColdWater) + String("\", \"value\": ") + String(tempColdWater, TEMP_DECIMALS) + String("}, ") + 
      String("{\"sensorId\": \"") + deviceAddressToString(thermWarmWater) + String("\", \"value\": ") + String(tempWarmWater, TEMP_DECIMALS) + String("}, ") + 
      String("{\"sensorId\": \"") + deviceAddressToString(thermOutside) + String("\", \"value\": ") + String(tempOutside, TEMP_DECIMALS) + String("}, ") + 
      String("{\"sensorId\": \"") + deviceAddressToString(thermPumpOut) + String("\", \"value\": ") + String(tempPumpOut, TEMP_DECIMALS) + String("}, ") + 
      String("{\"sensorId\": \"") + deviceAddressToString(thermPumpReturn) + String("\", \"value\": ") + String(tempPumpReturn, TEMP_DECIMALS) + String("}]");
    
    // send the HTTP POST request
    client.println(F("POST / HTTP/1.0"));
    client.println(String("Host: ") + String(server));
    client.println(F("Connection: Close"));
    client.println(F("User-Agent: Arduino-ESP8266-EspWifi"));
    client.println(F("Content-Type: application/json"));
    client.print("Content-Length: ");
    client.println(content.length() + 4);
    client.println();
    client.println(content);
    client.println();

    // note the time that the connection was made
    lastConnectTime = millis();
    
  } else {
    // if you couldn't make a connection
    Serial.println("Connection failed");
  }
}

void printMacAddress() {
  // get your MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  
  // print MAC address
  char buf[20];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  Serial.print("MAC address: ");
  Serial.println(buf);
}

void readTemperatures() {
  sensors.requestTemperatures();
  tempColdWater = sensors.getTempC(thermColdWater);
  tempWarmWater = sensors.getTempC(thermWarmWater);
  tempOutside = sensors.getTempC(thermOutside);
  tempPumpOut = sensors.getTempC(thermPumpOut);
  tempPumpReturn = sensors.getTempC(thermPumpReturn);
}

void printTemperatures() {
  Serial.println("------------------------");
  Serial.print("Cold water: ");
  Serial.println(tempColdWater, TEMP_DECIMALS);
  Serial.print("Warm water: ");
  Serial.println(tempWarmWater, TEMP_DECIMALS);
  Serial.print("Outside: ");
  Serial.println(tempOutside, TEMP_DECIMALS);
  Serial.print("Pump, out: ");
  Serial.println(tempPumpOut, TEMP_DECIMALS);
  Serial.print("Pump, return: ");
  Serial.println(tempPumpReturn, TEMP_DECIMALS);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void initTempSensors() {
  // Start up the sensors
  Serial.println("Initializing sensors");
  sensors.begin();
  Serial.println("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // set the resolution for all devices
  sensors.setResolution(thermColdWater, TEMPERATURE_PRECISION);
  sensors.setResolution(thermWarmWater, TEMPERATURE_PRECISION);
  sensors.setResolution(thermOutside, TEMPERATURE_PRECISION);
  sensors.setResolution(thermPumpOut, TEMPERATURE_PRECISION);
  sensors.setResolution(thermPumpReturn, TEMPERATURE_PRECISION);
  
  // read and show initial temperatures
  delay(2000);
  readTemperatures();
  delay(2000);
  printTemperatures();
}

void initWifi() {
  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  } else {
    Serial.println("Initialized WiFi shield...");
  }
  
  // show MAC
  printMacAddress();
  
  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  // connected
  Serial.println("You're connected to the network");
  printWifiStatus();
}

String deviceAddressToString(DeviceAddress deviceAddress){
    static char return_me[18];
    static char *hex = "0123456789ABCDEF";
    uint8_t i, j;

    for (i=0, j=0; i<8; i++) 
    {
         return_me[j++] = hex[deviceAddress[i] / 16];
         return_me[j++] = hex[deviceAddress[i] & 15];
    }
    return_me[j] = '\0';
    
    return String(return_me);
}

