#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"

File myFile;
const char *filename = "test.txt";
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    Serial.println("Connecting to serial port...");  
  }
  Serial.print("Initializing SD card...");
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  //delete existing file if any
  if (SD.exists(filename)){ 
    Serial.println("Deleting test.txt");
    SD.remove(filename);
  }
  else 
  {
    Serial.println("Nothing to delete");    
  }
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
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
}

void loop() {
  // nothing happens after setup
}
