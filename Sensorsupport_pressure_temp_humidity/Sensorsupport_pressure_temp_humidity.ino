
/***********************************************************
To Do:
- Calibrate qualitative light descriptors
-Create separate functions to simplify loop() 
- Set RTC from sd card:
  - Check for time.txt using SD.exist
  - Set RTC based on date/time values expressed in sd card, create specific format
************************************************************/
#include <Wire.h>
#include <MPL3115A2.h> //Temperature/pressure sensor support. Available at https://github.com/sparkfun/MPL3115A2_Breakout
#include <Narcoleptic.h> //microcontroller sleep library
#include <SD.h>
#include <DHT.h> //This library supports DHTxx sensors, in this case DHT11 temp/ humidity. Available from https://github.com/adafruit/DHT-sensor-library
#include <RTClib.h> //Available from https://github.com/adafruit/RTClib

#define DHTPIN 3    // what pin the DHT is connected to
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);
int photo_pin = A0; //currently testing with photoresistor,connected to A0 and 5V, with 1kohm pulldown
File data; //initializes "data" as a file name

MPL3115A2 baro;

RTC_DS1307 RTC;

//pins defined below are switches for unit conversions
int temp_pin=0 ;//C to F
int alt_pin=1 ;//meters to feet
int time_pin=2 ;//1 min intervals and 30 min intervals


void setup(){
  SD.begin(10); //This is the chipselect pin, change based on SD shield documentation.
   
  createHeader();
   
  data.close();
  dht.begin();
  Wire.begin();
  RTC.begin();
  baro.begin();
  baro.setModeBarometer();
  baro.setModeAltimeter();
  baro.setOversampleRate(7);
  baro.enableEventFlags();
  
  pinMode(temp_pin, INPUT);
  pinMode(alt_pin, INPUT);
  pinMode(time_pin, INPUT);
}

void loop(){
  float pascals=baro.readPressure();
  float inHg=pascals/3386.389; 
  float degC=baro.readTemp();
  float degF=baro.readTempF();
  
  data=SD.open("data.csv", FILE_WRITE);
   timestamp();
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
    data.print(baro.readAltitude());
  }else{
    data.print(baro.readAltitudeFt());
  }
  data.print(",");
  data.print(analogRead(photo_pin));
  data.print(",");
  photoQual();
  data.println();
  data.close();
  sleep();
  
}

void createHeader(){
  if(SD.exists("data.csv") == false){  //only creates header if file doesn't exist
   data=SD.open("data.csv", FILE_WRITE);
   data.print("Time");
  data.print(",");
  data.print("Temperature");
  data.print(",");
  data.print("Humidity");
  data.print(",");
  data.print("Pressure (in Hg)");
  data.print(",");
  data.print("Altitude (meters)");
  data.print(",");
  data.print("Light (Raw value 0-1023)");
  data.print(",");
  data.print("Light (qualitative descriptor");
  data.println();
 }else{
   data=SD.open("data.csv", FILE_WRITE);
 }
}

void photoQual(){
  //below prints qualitative descriptors of light intensity
  if(analogRead(photo_pin) < 10){
    data.print("dark");
  }else if(analogRead(photo_pin)<100){
    data.print("dim");
  }else if(analogRead(photo_pin)<300){
    data.print("light");
  }else if(analogRead(photo_pin)<600){
    data.print("bright");
  }else{
    data.print("very bright");
  }
}

void timestamp(){
  DateTime now = RTC.now();
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
}

void sleep(){
  if(digitalRead(time_pin)==HIGH){
    Narcoleptic.delay(2000-millis()%1000); //set to collect data every 30 minutes
  }else{
    Narcoleptic.delay(1000-millis()%1000); //set to collect data every minute
  }
}


