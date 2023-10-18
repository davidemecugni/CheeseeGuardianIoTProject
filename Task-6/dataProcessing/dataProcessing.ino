#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include <Arduino.h>
#include <SensirionI2CSht4x.h>
#include <Wire.h>
#include "rpcWiFi.h"
#include "TFT_eSPI.h"
#include <PubSubClient.h>
//#include <ctype.h>


// Update these variables accordingly
const char* ssid = "Davide"; // your mobile hotspot SSID (WiFi Name)
const char* password = "Davide31415";  // your mobile hotspot password (WiFi Password)
const char* mqtt_server = "broker.hivemq.com";  // MQTT Broker URL
const char *ID = "Wio-Terminal-Client-1811";  // Name of you device, must be unique. Replace xxxx with your last 4 digits of student ID 
const char *topic = "Wio-temp"; //Topic from your device

// Declare all variables and objects here
SensirionI2CSht4x sht4x;
TFT_eSPI tft;
WiFiClient wioClient;
PubSubClient client(wioClient);
char msg[50];
uint16_t errorCode;
char errorMessage[256];
String data;
//File myFile; // do file saving in microSD later...
float temperature;
float temperature_high = 28.0;
float humidity;
float humidity_high = 80.0;

// WiFi setup function
void setup_wifi() {

  delay(10);

  tft.setTextSize(2);
  tft.setCursor((320 - tft.textWidth("Connecting to Wi-Fi..")) / 2, 120);
  tft.print("Connecting to Wi-Fi..");

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); // Connecting WiFi

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  tft.fillScreen(TFT_BLACK);
  tft.setCursor((320 - tft.textWidth("Connected!")) / 2, 120);
  tft.print("Connected!");

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // Display Local IP Address
}

// MQTT reconnect function
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ID)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topic, "Hello from Wio Terminal");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// For initialization, only run once
void setup() {
  
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  Wire.begin();
  tft.begin();
  tft.fillScreen(TFT_WHITE); 
  tft.setRotation(3);
  sht4x.begin(Wire);  
  setup_wifi(); // Call setup WiFi function
  client.setServer(mqtt_server, 1883); // Connect the MQTT Server
  
  /*uint16_t error;
  char errorMessage[256];
  sht4x.begin(Wire);
  uint32_t serialNumber;
  error = sht4x.serialNumber(serialNumber);
  if (error) {
    Serial.print("Error trying to execute serialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("Serial Number: ");
    Serial.println(serialNumber);
  }  */
}

void loop() {

delay(1000);

// connect broker and publish message to mqtt broker
if (!client.connected()) {
  reconnect();
}
//client.loop(); //loop for message, may not be required

// measureHighPrecsion gives variable temperature and humidity values
errorCode = sht4x.measureHighPrecision(temperature, humidity);
    if (errorCode) {
        Serial.print("Error trying to execute measureHighPrecision(): ");
        errorToString(errorCode, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("Temperature:");
        Serial.print(temperature);
        Serial.print("\t");
        Serial.print("Humidity:");
        Serial.println(humidity);
    }
 
if (temperature>=temperature_high)  {
  data = String("High Temperature Detected:")+String(temperature);
  data.toCharArray(msg, 50);
  Serial.println(msg);
  Serial.println("---------------------------------------------------------");
  client.publish(topic, msg);
}
    
if (humidity>=humidity_high)  {
    data = String("High Humidity Detected:")+String(humidity);
    data.toCharArray(msg, 50);
    Serial.println(msg);
    Serial.println("---------------------------------------------------------");
    client.publish(topic, msg);
  }
}