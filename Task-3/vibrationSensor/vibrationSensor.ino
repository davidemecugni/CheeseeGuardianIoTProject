#include"TFT_eSPI.h"
#include <Arduino.h>
#include <SensirionI2CSht4x.h>
#include <Wire.h>
TFT_eSPI tft;

SensirionI2CSht4x sht4x;

void setup() {
  //Serial
  Serial.begin(115200);
    while (!Serial) {
      delay(100);
  }
  //I2C
  Wire.begin();
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK); 
    
  pinMode(LED_BUILTIN, OUTPUT); //control LED, LED_BUILTIN represents the pin which controls LED
  uint16_t error;
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
      
    }
} 
 
void loop() {
      uint16_t error;
    char errorMessage[256];

    delay(1000);

    float temperature;
    float humidity;
    error = sht4x.measureHighPrecision(temperature, humidity);
    int vibration = analogRead(A0);//read the vibration
    if (error) {
        Serial.print("Error trying to execute measureHighPrecision(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("Temperature:");
        Serial.print(temperature);
        Serial.print("\t");
        Serial.print("Humidity:");
        Serial.println(humidity);
        Serial.print("Vibration:");
        Serial.println(vibration);//print it on monitor
        tft.drawString("Temperature:",70,80);//prints string at (70,80)
        tft.setCursor(200,80);
        tft.print(temperature);
        tft.drawString("Humidity:",70,110);//prints string at (70,110)
        tft.setCursor(200,110);
        tft.print(humidity);
        tft.drawString("Vibration:",70,140);//prints string at (70,110)
        tft.setCursor(200,140);
        tft.print(vibration);
        }

}