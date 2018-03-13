#include "WiFiEsp.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// ***** Temperature sensors *****
// Data wire is plugged into pin 10 on the Arduino
#define ONE_WIRE_BUS 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

uint8_t sensorCount = 0;
DeviceAddress addresses[10];
float temperatures[10];

unsigned long lastReadTime = millis();      // when we last read temps (to avoid call to delay)
unsigned long lastPrintTime = millis();     // when we last printed temps (to avoid call to delay)
#define DELAY_TEMP_READ 10000L               // delay for measuring temp (ms)
#define DELAY_TEMP_PRINT 15000L             // delay for printing temp to serial (ms)
#define TEMP_DECIMALS 4                     // 4 decimals of output
#define TEMPERATURE_PRECISION 12            // 12 bits precision

// **** WiFi *****
const char ssid[] = "MM_IoT";               // your network SSID (name)
const char pass[] = "chippo-lekkim";        // your network password
const char server[] = "desolate-meadow-68880.herokuapp.com";
#define DELAY_POST_DATA 120000L            // delay between updates, in milliseconds

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
  if (sensorCount <= 0) return;
  
  // close any connection before send a new request
  // this will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection
  if (client.connect(server, 80)) {
    Serial.println("Connecting...");

    // prepare content
    String content;
    for (uint8_t i=0; i<sensorCount; i++) {
      if (i == 0) {
        content = String("[{\"sensorId\": \"");
      } else {
        content += String(", {\"sensorId\": \"");
      }
      content += deviceAddressToString(addresses[i]) + String("\", \"sensorValue\": ") + String(temperatures[i], TEMP_DECIMALS) + String("}");
    }
    content += String("]");
    
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
  // begin to scan for change in sensors
  sensors.begin();

  // get count
  uint8_t sensorCountNew = sensors.getDeviceCount();
  if (sensorCount != sensorCountNew) {
    // sensorCount changed
    Serial.print("Detected sensor count change - was ");
    Serial.print(sensorCount);
    Serial.print(" now ");
    Serial.println(sensorCountNew);
    sensorCount = sensorCountNew;

    if (sensorCount > 0) {
      // set resolution
      sensors.setResolution(TEMPERATURE_PRECISION);
  
      // get addresses
      for (uint8_t i=0; i<sensorCount; i++) {
        sensors.getAddress(addresses[i], i);
      }
    }
  }
  
  if (sensorCount > 0) {
    // request temperatures and store
    sensors.requestTemperatures();
    for (uint8_t i=0; i<sensorCount; i++) {
      temperatures[i] = sensors.getTempCByIndex(i);
    }
  }
}

void printTemperatures() {
  if (sensorCount <= 0) return;
  
  Serial.println("------------------------");
  for (uint8_t i=0; i<sensorCount; i++) {
    Serial.print(deviceAddressToString(addresses[i]));
    Serial.print(": ");
    Serial.println(temperatures[i], TEMP_DECIMALS);
  }
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

