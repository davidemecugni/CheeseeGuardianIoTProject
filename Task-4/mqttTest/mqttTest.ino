/*
   Modified from MQTT Example for SeeedStudio Wio Terminal
   Original Author: Salman Faris
   Date: 31/07/2020
   Last Updates: 02/08/2020
*/

#include "rpcWiFi.h"
#include "TFT_eSPI.h"
#include <PubSubClient.h>
#include <Arduino.h>
#include <SensirionI2CSht4x.h>
#include <Wire.h>

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
float temperature;
float humidity;
String data;

// WiFi setup functions
void setup_wifi() {

  delay(10); //small delay to prevent error

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
      client.publish(topic, "hello from Wio Terminal");
      // ... and resubscribe
      //client.subscribe("subcribedTopic");
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
  
  Serial.begin(115200); //initialise serial monitor
  Wire.begin(); //initialise I2C
  tft.begin();  //initialise LCD
  tft.fillScreen(TFT_WHITE);
  tft.setRotation(3);

  setup_wifi();
  client.setServer(mqtt_server, 1883); // Connect the MQTT Server

  //uint16_t error;
  //char errorMessage[256];

  sht4x.begin(Wire);  //connect the sensor

  //uint32_t serialNumber;
  //error = sht4x.serialNumber(serialNumber); 
}

void loop() 
{
  delay(1000);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //uint16_t error;
  //char errorMessage[256];

  //float temperature;
  //float humidity;

  // measureHighPrecsion gives variable temperature and humidity values
  // 0 on success, an error code otherwise
  errorCode = sht4x.measureHighPrecision(temperature, humidity);

  //String data;
  data = String("Temperature:")+String(temperature)+String("\t")+String("Humidity:")+String(humidity);
  data.toCharArray(msg, 50);
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(topic, msg);
}