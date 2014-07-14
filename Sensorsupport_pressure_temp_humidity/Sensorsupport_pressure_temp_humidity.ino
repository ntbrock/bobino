#include <Wire.h>
#include <Adafruit_MPL3115A2.h> //Temperature/pressure sensor support. Available at https://github.com/adafruit/Adafruit_MPL3115A2_Library
#include <JeeLib.h> //microcontroller sleep library
#include <SD.h>
#include <DHT.h> //This library supports DHTxx sensors, in this case DHT11 temp/ humidity. Available from https://github.com/adafruit/DHT-sensor-library
#include <RTClib.h> //Available from https://github.com/adafruit/RTClib

Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
#define DHTPIN 2     // what pin the DHT is connected to
#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);
int photo_pin = A0; //currently testing with photoresistor,connected to A0 and 5V, with 10kohm pulldown
File data; //initializes "data" as a file name
ISR(WDT_vect) { Sleepy::watchdogEvent();} //sets up watchdog, allows microcontroller sleep routine 
RTC_DS1307 RTC;


void setup(){
  SD.begin(10); //This is the chipselect pin, change based on SD shield documentation.
  data = SD.open("data.csv", FILE_WRITE);
  data.print("Time");
  data.print(",");
  data.print("Temperature");
  data.print(",");
  data.print("Humidity");
  data.print(",");
  data.print("Pressure (atm)");
  data.print(",");
  data.print("Altitude (meters)");
  data.print(",");
  data.print("sensor value");
  data.println();
  data.close();
  baro.begin();
  dht.begin();
  Wire.begin();
  RTC.begin();
}


void loop(){
  DateTime now = RTC.now();
  float pascals=baro.getPressure();
  float atm=pascals/101325; 
  data=SD.open("data.csv", FILE_WRITE);
   //timestamp from rtc
  data.print(now.year(), DEC);
  data.print('/');
  data.print(now.month(), DEC);
  data.print('/');
  data.print(now.day(), DEC);
  data.print(' ');
  data.print(now.hour(), DEC);
  data.print(':');
  data.print(now.minute(), DEC);
  data.print(':');
  data.print(now.second(), DEC);
 
  data.print(",");
  data.print(baro.getTemperature());
  data.print(",");
  data.print(dht.readHumidity());
  data.print(",");
  data.print(atm);
  data.print(",");
  data.print(baro.getAltitude());
  data.print(",");
  data.print(analogRead(A0));
  data.println();
  data.close();
  Sleepy::loseSomeTime(1000-millis()%1000);
  
  
}
