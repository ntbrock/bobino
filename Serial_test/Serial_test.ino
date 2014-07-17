
/***********************************************************
This is exactly the same as the main sensor code, but with sd logging changed to serial output for debugging
************************************************************/
#include <Wire.h>
#include <Adafruit_MPL3115A2.h> //Temperature/pressure sensor support. Available at https://github.com/adafruit/Adafruit_MPL3115A2_Library
#include <Narcoleptic.h> //microcontroller sleep library
#include <SD.h>
#include <DHT.h> //This library supports DHTxx sensors, in this case DHT11 temp/ humidity. Available from https://github.com/adafruit/DHT-sensor-library
#include <RTClib.h> //Available from https://github.com/adafruit/RTClib

Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
#define DHTPIN 3    // what pin the DHT is connected to
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);
int photo_pin = A0; //currently testing with photoresistor,connected to A0 and 5V, with 1kohm pulldown

RTC_DS1307 RTC;

//pins defined below are switches for unit conversions
int temp_pin=0 ;//C to F
int alt_pin=1 ;//meters to feet
int time_pin=2 ;//1 min intervals and 30 min intervals


void setup(){
  Serial.begin(9600); 

   Serial.print("Time");
  Serial.print(",");
  Serial.print("Temperature");
  Serial.print(",");
  Serial.print("Humidity");
  Serial.print(",");
  Serial.print("Pressure (in Hg)");
  Serial.print(",");
  Serial.print("Altitude (meters)");
  Serial.print(",");
  Serial.print("Light (Raw value 0-1023)");
  Serial.print(",");
  Serial.print("Light (qualitative descriptor");
  Serial.println();

  baro.begin();
  dht.begin();
  Wire.begin();
  RTC.begin();
  
  pinMode(temp_pin, INPUT);
  pinMode(alt_pin, INPUT);
  pinMode(time_pin, INPUT);
}


void loop(){
  DateTime now = RTC.now();
  float pascals=baro.getPressure();
  float inHg=pascals/3386.389; 
  float degC=baro.getTemperature();
  float degF=degC*9/5+32;
  float altM=baro.getAltitude();
  float altFt=altM*3.28;

   //timestamp from rtc
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
 
  Serial.print(",");
  if(digitalRead(temp_pin)==HIGH){
    Serial.print(degC);
  }else{
    Serial.print(degF);
  }
  Serial.print(",");
  Serial.print(dht.readHumidity()); //DHT uses "read" rather than "get." readTemperature is also available, but less accurate than other sensor
  Serial.print(",");
  Serial.print(inHg);
  Serial.print(",");
  if(digitalRead(alt_pin)==HIGH){
    Serial.print(altM);
  }else{
    Serial.print(altFt);
  }
  Serial.print(",");
  Serial.print(analogRead(photo_pin));
  Serial.print(",");
  //below prints qualitative descriptors of light intensity
  if(analogRead(photo_pin) < 10){
    Serial.print("dark");
  }else if(analogRead(photo_pin)<100){
    Serial.print("dim");
  }else if(analogRead(photo_pin)<300){
    Serial.print("light");
  }else if(analogRead(photo_pin)<600){
    Serial.print("bright");
  }else{
    Serial.print("very bright");
  }
  
  Serial.println();

  if(digitalRead(time_pin)==HIGH){
    Narcoleptic.delay(1800000-millis()%1000); //set to collect data every 30 minutes
  }else{
    Narcoleptic.delay(60000-millis()%1000); //set to collect data every minute
  }
}
