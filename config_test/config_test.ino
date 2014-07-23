#include <SD.h>

char byteRead;
File temp;

void setup() {                
// Turn the Serial Protocol ON
  Serial.begin(9600);
  SD.begin();
  temp=SD.open("temp.txt",FILE_READ);
  byteRead=temp.read();
}

void loop() {
  if (byteRead=='F'|| byteRead=='f'){
    Serial.println("Set to Farenheit");
  }else{
    Serial.println("Set to Celcius");
  }
  delay(1000);
}
