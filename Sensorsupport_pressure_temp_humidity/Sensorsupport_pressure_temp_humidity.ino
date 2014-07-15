#include <Wire.h>
#include <Adafruit_MPL3115A2.h> //Temperature/pressure sensor support. Available at https://github.com/adafruit/Adafruit_MPL3115A2_Library
#include <JeeLib.h> //microcontroller sleep library
#include <SD.h>
#include <DHT.h> //This library supports DHTxx sensors, in this case DHT11 temp/ humidity. Available from https://github.com/adafruit/DHT-sensor-library
#include <RTClib.h> //Available from https://github.com/adafruit/RTClib

Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
#define DHTPIN 3    // what pin the DHT is connected to
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);
int photo_pin = A0; //currently testing with photoresistor,connected to A0 and 5V, with 1kohm pulldown
File data; //initializes "data" as a file name
ISR(WDT_vect) { Sleepy::watchdogEvent();} //sets up watchdog, allows microcontroller sleep routine 
RTC_DS1307 RTC;

//pins defined below are switches for unit conversions
int temp_pin=0 ;//C to F
int alt_pin=1 ;//meters to feet
int time_pin=2 ;//1 min intervals and 30 min intervals


void setup(){
  SD.begin(10); //This is the chipselect pin, change based on SD shield documentation.
   
  if(SD.exists("data.csv") == false){  //only creates header if file doesn't exist
   data=SD.open("data.csv", FILE_WRITE);
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
 }else{
   data=SD.open("data.csv", FILE_WRITE);
 }
  data.close();
  baro.begin();
  dht.begin();
  Wire.begin();
  RTC.begin();
}


void loop(){
  DateTime now = RTC.now();
  float pascals=baro.getPressure();
  float inHg=pascals/3386.389; 
  float degC=baro.getTemperature();
  float degF=degC*9/5+32;
  float altM=baro.getAltitude();
  float altFt=altM*3.28;
  
  
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
  if(digitalRead(temp_pin)==HIGH){
    data.print(degC);
  }else{
    data.print(degF);
  }
  data.print(",");
  data.print(dht.readHumidity()); //DHT uses "read" rather than "get." readTemperature is also available, but less accurate than other sensor
  data.print(",");
  data.print(inHg);
  data.print(",");
  if(digitalRead(alt_pin)==HIGH){
    data.print(altM);
  }else{
    data.print(altFt);
  }
  data.print(",");
  data.print(analogRead(A0));
  data.println();
  data.close();
  if(digitalRead(time_pin)==HIGH){
    Sleepy::loseSomeTime(1800000-millis()%1000); //set to collect data every 30 minutes
  }else{
    Sleepy::loseSomeTime(60000-millis()%1000); //set to collect data every minute
  }
}
