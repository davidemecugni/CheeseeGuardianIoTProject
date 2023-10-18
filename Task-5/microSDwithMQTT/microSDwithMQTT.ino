#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include <Arduino.h>
#include <SensirionI2CSht4x.h>
#include <Wire.h>
#include "rpcWiFi.h"
#include "TFT_eSPI.h"
#include <PubSubClient.h>

// Update these variables accordingly
const char* ssid = "Davide"; // your mobile hotspot SSID (WiFi Name)
const char* password = "Davide31415";  // your mobile hotspot password (WiFi Password)
const char* mqtt_server = "broker.hivemq.com";  // MQTT Broker URL
const char* ID = "Wio-Terminal-Client-xxxx";  // Name of you device, must be unique. Replace xxxx with your last 4 digits of student ID 
const char* topic = "Wio-temp"; //Topic from your device

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
File myFile;
const char *filename = "test.txt";

void setup_wifi() {

  delay(10); // small delay to prevent error

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

// Define reconnect function
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
      //client.subscribe("subTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  
  Serial.begin(115200);
  Wire.begin();
  tft.begin();
  tft.fillScreen(TFT_WHITE); 
  tft.setRotation(3);
  sht4x.begin(Wire);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883); // Connect the MQTT Server
 
 //Initialise microSD card
  while (!Serial) {
  }
  Serial.print("Initializing SD card...");
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  //Delete existing file if any
  if (SD.exists(filename)){ 
    Serial.print("Deleting :");
    Serial.println(filename);
    SD.remove(filename);
  }
  else 
  {
    Serial.println("Nothing to delete");    
  }
  
  //uint16_t error;
  //char errorMessage[256];
  /*sht4x.begin(Wire);
  uint32_t serialNumber;
  error = sht4x.serialNumber(serialNumber);
    if (error) {
        Serial.print("Error trying to execute serialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("Serial Number: ");
        Serial.println(serialNumber);
        
    }*/
}

void loop() {

  delay(1000);
 
   // reconnect broker
  if (!client.connected()) {
    reconnect();
  }
  //client.loop(); //loop for message, may not be required
  
  // measureHighPrecsion gives variable temperature and humidity values
  errorCode = sht4x.measureHighPrecision(temperature, humidity); //ignore error handling
 
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  // this means append behind
  // FILE_WRITE
  myFile = SD.open(filename, FILE_APPEND);

  // if the file opened okay, write it:
   if (myFile) {
    Serial.print("Writing to : ");
    Serial.print(filename);
    myFile.print("Temperature:");
    myFile.print(temperature);
    myFile.print("\t");
    myFile.print("Humidity:");
    myFile.print(humidity);
    myFile.println();
    // close the file:
    myFile.close();
    Serial.println("...done");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening : ");
    Serial.println(filename);
  }

  // re-open the file for reading:
  myFile = SD.open(filename, FILE_READ);
  if (myFile) {
    Serial.print("Reading : ");
    Serial.println(filename);
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening : ");
    Serial.println(filename);
  }

  Serial.println("---------------------------------------------------------");
  Serial.print("Publish message: ");
  //String data;
  data = String("Temperature:")+String(temperature)+String("\t")+String("Humidity:")+String(humidity);
  data.toCharArray(msg, 50);
  Serial.println(msg);
  Serial.println("---------------------------------------------------------");
  client.publish(topic, msg);
}