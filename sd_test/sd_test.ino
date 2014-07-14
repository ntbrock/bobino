
#include <JeeLib.h> //microcontroller sleep library, minimizes power consumption
#include <SD.h>


int led_pin = 13;
int photo_pin = A0; //currently testing with photoresistor,connected to A0 and 5V w/ pulldown to ground
File data; //initializes "data" as a file name
ISR(WDT_vect) { Sleepy::watchdogEvent();} //sets up watchdog, allows microcontroller sleep routine 

void setup(){
  SD.begin(10); //This is the chipselect pin, change based on SD shield documentation.
  data = SD.open("data.csv", FILE_WRITE);
  data.print("Time");
  data.print(",");
  data.print("sensor value");
  data.println();
  data.close();
  
}


void loop(){
  float time=millis()/1000;
  data=SD.open("data.csv", FILE_WRITE);
  data.print(time);
  data.print(",");
  data.print(analogRead(A0));
  data.println();
  data.close();
  Sleepy::loseSomeTime(5000-millis()%1000); //corrects for sensor detection time.
  
  
}
