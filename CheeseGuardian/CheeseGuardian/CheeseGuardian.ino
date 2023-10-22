#include"TFT_eSPI.h"
#include <Arduino.h>
//Screen
#include <SensirionI2CSht4x.h>
//I2C
#include <Wire.h>
//tVOC and CO2 sensor
#include "sensirion_common.h"
#include "sgp30.h"
//Accelerator library
#include"LIS3DHTR.h"
//Used for conversions
#include <math.h>
//SD card
#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
//WiFi and MQTT
#include "rpcWiFi.h"
#include <PubSubClient.h>
//Constants
#define E 2.718281828
#define DEFAULTTEXTSIZE 2
#define SCREENWIDTH 320
#define SCREENHEIGHT 240
//Values are around 1 or 2 if untouched and dry, 50 if touched, >200 if wet
#define FLOODPIN A0
#define FLOODLIMIT 150
#define LCD_BACKLIGHT (72Ul) // Control Pin of LCD
#define SAFETVOCLIMIT 500 //or 80 ppb  https://www.dcceew.gov.au/environment/protection/npi/substances/fact-sheets/total-volatile-organic-compounds
#define SAFEC02LIMIT 5000 // ppm https://www.fsis.usda.gov/sites/default/files/media_file/2020-08/Carbon-Dioxide.pdf
#define WIFITIMEOUT 100
#define MQTTTIMEOUT 1000
#define HARDERROR 1
#define SERIALHARDERROR 0
#define MAINDELAY 1000
#define STDX -0.54
#define STDY -0.02
#define STDZ -0.86
//Screen
TFT_eSPI tft;
//TempHumiSensor
SensirionI2CSht4x sht4x;
//Gyro
LIS3DHTR<TwoWire>  gyro;
//WiFi
const char* ssid = "Davide"; // your mobile hotspot SSID (WiFi Name)
const char* password = "Davide31415";  // your mobile hotspot password (WiFi Password)
const char* mqtt_server = "broker.hivemq.com";  // MQTT Broker URL
const char* ID = "Wio-Terminal-Client-1811";  // Name of you device, must be unique. Replace xxxx with your last 4 digits of student ID 
const char* topic = "Wio-CheeseGuardian"; //Topic from your device
//MQTT
WiFiClient wioClient;
PubSubClient client(wioClient);
//Payload
char msg[100];
String data;

void Credits(){
  tft.setTextSize(1);
  tft.drawRect(0,SCREENHEIGHT-11,SCREENWIDTH,SCREENHEIGHT-3,TFT_MAGENTA);
  tft.drawString("Davide Mecugni", 2, SCREENHEIGHT- 10);
  tft.drawString("CheeseGuardian", SCREENWIDTH-86, SCREENHEIGHT- 10);
  tft.setTextSize(DEFAULTTEXTSIZE);
}
void SerialData(float* temperature, float* humidity, float* ah, int *flood, u16* tvoc_ppb, u16* co2_eq_ppm, float* x_raw, float* y_raw, float* z_raw){
  Serial.print("Temperature:");
  Serial.println(*temperature);
  Serial.print("Humidity:");
  Serial.println(*humidity);
  Serial.print("AH:");
  Serial.println(*ah);
  Serial.print("Flood:");
  Serial.println(*flood);
  Serial.print("tVOC:");
  Serial.print(*tvoc_ppb);
  Serial.println("ppb");
  Serial.print("CO2eq:");
  Serial.print(*co2_eq_ppm);
  Serial.println("ppm");
  Serial.print("X:");
  Serial.print(*x_raw);
  Serial.print(", ");
  Serial.print("Y:");
  Serial.print(*y_raw);
  Serial.print(", ");
  Serial.print("Z:");
  Serial.print(*z_raw);
  Serial.print("\n\n");
}
void ScreenData(float* temperature, float* humidity, float* ah, int *flood, u16* tvoc_ppb, u16* co2_eq_ppm, float* x_raw, float* y_raw, float* z_raw){
  tft.drawString("Temp:",70,20);
  tft.setCursor(200,20);
  tft.print(*temperature);
  tft.drawString("Humidity:",70,50);
  tft.setCursor(200,50);
  tft.print(*humidity);
  tft.drawString("Flood:",70,80);
  tft.setCursor(200,80);
  tft.print((float)*flood);
  tft.drawString("tVOC:",70,110);
  tft.setCursor(200,110);
  tft.print((float)*tvoc_ppb);
  tft.drawString("CO2eq:",70,140);
  tft.setCursor(200,140);
  tft.print((float)*co2_eq_ppm);
  tft.setCursor(40,170);
  tft.print(String("X:")+String(*x_raw)+String(",")+
         String("Y:")+String(*y_raw)+String(",")+
         String("Z:")+String(*z_raw));
}

bool ReadData(float* temperature, float* humidity, float* ah, int *flood, u16* tvoc_ppb, u16* co2_eq_ppm, float* x_raw, float* y_raw, float* z_raw){
  //Error data for the temp/humi sensor
  uint16_t error_temp_humi;
  char errorMessage[256];
  //Reading Values
  error_temp_humi = sht4x.measureHighPrecision(*temperature, *humidity);
  if (error_temp_humi) {
      Serial.print("Error trying to execute measureHighPrecision(): ");
      errorToString(error_temp_humi, errorMessage, 256);
      Serial.println(errorMessage);
  }
  //Absolute humidity measured in g/m^3
  //Precide in range -30°C≤T≤35°C Bolton, D., The computation of equivalent potential temperature, Monthly Weather Review, 108, 1046-1053, 1980..
  *ah = (6.112*pow(E,((17.67* (*temperature))/((*temperature)+243.5)))*(*humidity)*2.1674)/(273.15 + (*temperature));
  //Flood value 1/2 untouched, 50 if touched, >200 if wet
  *flood = analogRead(FLOODPIN);
  //VOC/CO2 error
  s16 error_voc_co2 = 0;
  //Calibrates the TVOC CO2 sensor based on the absolute humidity
  sgp_set_absolute_humidity(*ah);
  error_voc_co2 = sgp_measure_iaq_blocking_read(tvoc_ppb, co2_eq_ppm);
  if(error_voc_co2){
    Serial.println("error reading IAQ values\n");
    errorToString(error_voc_co2, errorMessage, 256);
    Serial.println(errorMessage);
  }
  //Gyro
  *x_raw = gyro.getAccelerationX();
  *y_raw = gyro.getAccelerationY();
  *z_raw = gyro.getAccelerationZ();
}
void ScreenTerminalSetup(){
  //Serial setup
  Serial.begin(115200);
    while (!Serial && SERIALHARDERROR) {
      delay(100);
  }
  //I2C
  Wire.begin();

  //Screen settings
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK); 
  tft.setTextSize(DEFAULTTEXTSIZE);
  pinMode(LED_BUILTIN, OUTPUT); //control LED, LED_BUILTIN represents the pin which controls LED
  
  //Temp Humi sensor
  sht4x.begin(Wire);
  uint16_t error;
  char errorMessage[256];
  uint32_t serialNumber;
  error = sht4x.serialNumber(serialNumber);
  if (error) {
      Serial.print("Error trying to execute serialNumber(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
      while(HARDERROR);
  } else {
    Serial.print("Serial Number: ");
    Serial.println(serialNumber);
    
  }
  digitalWrite(LCD_BACKLIGHT, HIGH);
}
void TVOCCO2Setup(){
  //tVOC and CO2 sensor
    s16 err;
    u16 scaled_ethanol_signal, scaled_h2_signal;
    #if defined(ESP8266)
    pinMode(15, OUTPUT);
    digitalWrite(15, 1);
    Serial.println("Setting up tVOC CO2");
    delay(500);
    #endif
    /*  Init module,Reset all baseline,The initialization takes up to around 15 seconds, during which
        all APIs measuring IAQ(Indoor air quality ) output will not change.Default value is 400(ppm) for co2,0(ppb) for tvoc*/
    while (sgp_probe() != STATUS_OK) {
        Serial.println("SGP failed");
        while(HARDERROR);
    }
    /*Read H2 and Ethanol signal in the way of blocking*/
    err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal,
                                            &scaled_h2_signal);
    if (err == STATUS_OK) {
        Serial.println("Got tVOC CO2 signal!");
    } else {
        Serial.println("Error reading signals");
        while(HARDERROR);
    }
    err = sgp_iaq_init();
}
void GyroSetup(){
  gyro.begin(Wire1);
  gyro.setOutputDataRate(LIS3DHTR_DATARATE_25HZ);
  gyro.setFullScaleRange(LIS3DHTR_RANGE_2G);
}
void SDSetup(){
  Serial.print("Initializing SD card...");
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("SD initialization failed!");
    while(HARDERROR);
  }
  Serial.print("SD OK");
  /*
  myFile = SD.open(filename, FILE_WRITE); // O_TRUNC is to erase all content

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt: ");
    Serial.print("testing 1, 2, 3.");
    myFile.print("testing 1, 2, 3.");
    // close the file:
    myFile.close();
    Serial.println("...done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  // re-open the file for reading:
  myFile = SD.open(filename, FILE_READ);
  if (myFile) {
    Serial.print("Reading test.txt:");
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      // read symbol by symbol
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
  */
}

void SetupWiFi() {
  delay(100); // small delay to prevent error
  tft.fillScreen(TFT_BLACK); 
  tft.setTextSize(2);
  tft.setCursor((SCREENWIDTH - tft.textWidth("Connecting to Wi-Fi..")) / 2, 120);
  tft.print("Connecting to Wi-Fi..");
  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); // Connecting WiFi
  long i=0;
  uint color = TFT_WHITE;
  while (WiFi.status() != WL_CONNECTED) {
    delay(WIFITIMEOUT);
    WiFi.begin(ssid, password); // Connecting WiFi
    Serial.print(".");
    ++i;
    tft.drawCircle(SCREENWIDTH/2, SCREENHEIGHT/2 + 70, i*2, color);
    if(i>15){
      i=0;
      if(color == TFT_WHITE){
        color = TFT_BLACK;
      }
      else{
        color = TFT_WHITE;
      }
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  tft.fillScreen(TFT_BLACK);
  tft.setCursor((320 - tft.textWidth("Connected!")) / 2, 120);
  tft.print("Connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // Display Local IP Address
  delay(200);
  tft.fillScreen(TFT_BLACK);
}

// Define reconnect function
void ReconnectMQTT() {
  tft.fillScreen(TFT_BLACK); 
  // Loop until we're reconnected
  int i=0;
  while (!client.connected()) {
    tft.setCursor((SCREENWIDTH - tft.textWidth("Connecting to MQTT..")) / 2, 120);
    tft.print("Connecting to MQTT..");
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ID)) {
      Serial.println("");
      Serial.println("MQTT connected");
      tft.fillScreen(TFT_BLACK);
      tft.setCursor((320 - tft.textWidth("MQTT connected!")) / 2, 120);
      tft.print("MQTT connected!");
      delay(200);
      // Once connected, publish an announcement...
      client.publish(topic, "CheeseGuardian");
      // ... and resubscribe
      //client.subscribe("subTopic");
      tft.fillScreen(TFT_BLACK);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("Trying again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(MQTTTIMEOUT);
    }
    Serial.print(".");
    ++i;
    uint color = TFT_WHITE;
    tft.drawCircle(SCREENWIDTH/2, SCREENHEIGHT/2 + 70, i*2, color);
    if(i>15){
      i=0;
      if(color == TFT_WHITE){
        color = TFT_BLACK;
      }
      else{
        color = TFT_WHITE;
      }
    }
  }
  tft.fillScreen(TFT_BLACK); 
}
void SwitchSetup(){
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);
  /*
    // put your main code here, to run repeatedly:
   if (digitalRead(WIO_5S_UP) == LOW) {
    Serial.println("5 Way Up");
   }
   else if (digitalRead(WIO_5S_DOWN) == LOW) {
    Serial.println("5 Way Down");
   }
   else if (digitalRead(WIO_5S_LEFT) == LOW) {
    Serial.println("5 Way Left");
   }
   else if (digitalRead(WIO_5S_RIGHT) == LOW) {
    Serial.println("5 Way Right");
   }
   else if (digitalRead(WIO_5S_PRESS) == LOW) {
    Serial.println("5 Way Press");
   }
   delay(200);
   */
}

String CreatePayload(float* t, float* h, float* ah, int *flood, u16* tvoc_ppb, u16* co2_eq_ppm, float* x_raw, float* y_raw, float* z_raw){
  return String("T:")+String(*t)+String(",")+
         String("H:")+String(*h)+String(",")+
         String("AH:")+String(*ah)+String(",")+
         String("tVOC:")+String(*tvoc_ppb)+String(",")+
         String("CO2:")+String(*co2_eq_ppm)+String(",")+
         String("X:")+String(*x_raw)+String(",")+
         String("Y:")+String(*y_raw)+String(",")+
         String("Z:")+String(*z_raw)+String("\n");
}

void setup() {
  ScreenTerminalSetup();
  TVOCCO2Setup();
  GyroSetup();
  SDSetup();
  SetupWiFi();
  client.setServer(mqtt_server, 1883); // Connect the MQTT Server
} 

void loop() {
  //Reconnect WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Reconnecting WiFi");
    SetupWiFi();
  }
  //Reconnect to server
  if (!client.connected()) {
    Serial.println("Reconnecting MQTT");
    ReconnectMQTT();
  }
  //Variables definition
  float temperature;
  float humidity;
  float ah;
  int flood;
  u16 tvoc_ppb, co2_eq_ppm;
  float x_raw, y_raw, z_raw;
  ReadData(&temperature,&humidity,&ah,&flood,&tvoc_ppb,&co2_eq_ppm, &x_raw, &y_raw, &z_raw);
  //SerialData(&temperature,&humidity,&ah,&flood,&tvoc_ppb,&co2_eq_ppm, &x_raw, &y_raw, &z_raw);
  ScreenData(&temperature,&humidity,&ah,&flood,&tvoc_ppb,&co2_eq_ppm, &x_raw, &y_raw, &z_raw);
  Credits();
  //MQTT
  data = CreatePayload(&temperature,&humidity,&ah,&flood,&tvoc_ppb,&co2_eq_ppm, &x_raw, &y_raw, &z_raw);
  data.toCharArray(msg, 100);
  Serial.println(msg);
  client.publish(topic, msg);
  delay(MAINDELAY);
}
