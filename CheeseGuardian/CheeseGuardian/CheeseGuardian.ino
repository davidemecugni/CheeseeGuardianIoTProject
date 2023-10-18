#include"TFT_eSPI.h"
#include <Arduino.h>
//Screen
#include <SensirionI2CSht4x.h>
//I2C
#include <Wire.h>
#include "sensirion_common.h"
#include "sgp30.h"
#include <math.h>

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
TFT_eSPI tft;

SensirionI2CSht4x sht4x;


void Credits(){
  tft.setTextSize(1);
  tft.drawRect(0,SCREENHEIGHT-11,SCREENWIDTH,SCREENHEIGHT-3,TFT_MAGENTA);
  tft.drawString("Davide Mecugni", 2, SCREENHEIGHT- 10);
  tft.drawString("CheeseGuardian", SCREENWIDTH-86, SCREENHEIGHT- 10);
  tft.setTextSize(DEFAULTTEXTSIZE);
}
void SerialData(float* temperature, float* humidity, float* ah, int *flood, u16* tvoc_ppb, u16* co2_eq_ppm){
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
  Serial.print("\n\n");
}
void ScreenData(float* temperature, float* humidity, float* ah, int *flood, u16* tvoc_ppb, u16* co2_eq_ppm){
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
}

bool ReadData(float* temperature, float* humidity, float* ah, int *flood, u16* tvoc_ppb, u16* co2_eq_ppm){
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
}


void setup() {
  Serial.begin(115200);
    while (!Serial) {
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
  

  sht4x.begin(Wire);
  uint16_t error;
  char errorMessage[256];

  uint32_t serialNumber;
  error = sht4x.serialNumber(serialNumber);
  if (error) {
      Serial.print("Error trying to execute serialNumber(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
  } else {
      Serial.print("Serial Number: ");
      Serial.println(serialNumber);
      
    }
    digitalWrite(LCD_BACKLIGHT, HIGH);



    s16 err;
    u16 scaled_ethanol_signal, scaled_h2_signal;
    Serial.begin(115200);
    Serial.println("serial start!!");

    /*For wio link!*/
    #if defined(ESP8266)
    pinMode(15, OUTPUT);
    digitalWrite(15, 1);
    Serial.println("Set wio link power!");
    delay(500);
    #endif
    /*  Init module,Reset all baseline,The initialization takes up to around 15 seconds, during which
        all APIs measuring IAQ(Indoor air quality ) output will not change.Default value is 400(ppm) for co2,0(ppb) for tvoc*/
    while (sgp_probe() != STATUS_OK) {
        Serial.println("SGP failed");
        while (1);
    }
    /*Read H2 and Ethanol signal in the way of blocking*/
    err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal,
                                            &scaled_h2_signal);
    if (err == STATUS_OK) {
        Serial.println("get ram signal!");
    } else {
        Serial.println("error reading signals");
    }
    err = sgp_iaq_init();
    //
} 
 
void loop() {
    uint16_t error;
    char errorMessage[256];

    delay(1000);
    //Variables definition
    float temperature;
    float humidity;
    float ah;
    int flood;
    u16 tvoc_ppb, co2_eq_ppm;
    ReadData(&temperature,&humidity,&ah,&flood,&tvoc_ppb,&co2_eq_ppm);
    SerialData(&temperature,&humidity,&ah,&flood,&tvoc_ppb,&co2_eq_ppm);
    ScreenData(&temperature,&humidity,&ah,&flood,&tvoc_ppb,&co2_eq_ppm);
    Credits();
    }
