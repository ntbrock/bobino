
/***********************************************************
 * To Do:
 * - Calibrate qualitative light descriptors
 * 
 * - Set RTC from sd card:
 * - Check for time.txt using SD.exist
 * - Set RTC based on date/time values expressed in sd card, create specific format
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
char temp;
char alt;
String time;

int sleepTime;

void setup(){
  SD.begin(10); //This is the chipselect pin, change based on SD shield documentation.

  createHeader();
  configure(); 
  data.close();
  dht.begin();
  Wire.begin();
  RTC.begin();
  baro.begin();
  baro.setOversampleRate(7);
  baro.enableEventFlags();
  baro.setModeStandby();

}

void loop(){
  float pascals=getPressure();
  float inHg=pascals/3386.389; 
  float altM=getAlt();
  baro.setModeActive();
  float deg=getTemp();
  baro.setModeStandby();


  data=SD.open("data.csv", FILE_WRITE);
  timestamp();
  data.print(",");
  data.print(deg);
  data.print(",");
  data.print(dht.readHumidity()); //DHT readTemperature is also available, but less accurate than other sensor
  data.print(",");
  data.print(inHg);
  data.print(",");
  data.print(altM);
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
  }
  else{
    data=SD.open("data.csv", FILE_WRITE);
  }
}

void photoQual(){
  //below prints qualitative descriptors of light intensity
  if(analogRead(photo_pin) < 10){
    data.print("dark");
  }
  else if(analogRead(photo_pin)<100){
    data.print("dim");
  }
  else if(analogRead(photo_pin)<300){
    data.print("light");
  }
  else if(analogRead(photo_pin)<600){
    data.print("bright");
  }
  else{
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

float getPressure(){
  baro.setModeBarometer();
  baro.setModeActive();
  delay(550);
  return baro.readPressure();
  baro.setModeStandby();
}
float getAlt(){
  baro.setModeAltimeter(); 
  baro.setModeActive();
  delay(550);
  if(alt=='f'||alt=='F'){
    return baro.readAltitudeFt();
  }
  else{
    return baro.readAltitude();
  }
  baro.setModeStandby();
}
float getTemp(){
  if(temp=='f'||temp=='F'){
    return baro.readTempF();
  }
  else{
    return baro.readTemp();
  }
}
void configure(){
  temp=SD.open("config/temp.txt", FILE_READ).read();  
  alt=SD.open("config/alt.txt", FILE_READ).read();
  File timetxt;
  timetxt=SD.open("config/time.txt",FILE_READ);
  while(timetxt.peek()!=';'){
    char c=timetxt.read();
    time += c;
  }
  if(!time){
    sleepTime=10;
  }else{
  sleepTime=atoi(time.c_str());
  }
}


void sleep(){
  long currentTime=getUnixTime();
  long wakeTime=currentTime+(sleepTime*60)-2;
  while(currentTime<wakeTime){
    long sleepInterval=wakeTime-currentTime;
    Narcoleptic.delay(sleepInterval*900);
    currentTime=getUnixTime();
  }
}
long getUnixTime(){
  DateTime now=RTC.now();
  return now.unixtime();
}


