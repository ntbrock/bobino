/***********************************************************
 * BOBino: An Arduino Sensor Platform by Max Tucker
 * Full documentation, including a parts list, can be found at https://github.com/ntbrock/bobino
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Wire.h>
#include <MPL3115A2.h> //Temperature/pressure sensor support. Available at https://github.com/sparkfun/MPL3115A2_Breakout
#include <Narcoleptic.h> //microcontroller sleep library. Available at https://code.google.com/p/narcoleptic/downloads/detail?name=Narcoleptic_v1a.zip
#include <SD.h>
#include <DHT.h> //This library supports DHTxx sensors, in this case DHT11 temp/ humidity. Available from https://github.com/adafruit/DHT-sensor-library
#include <RTClib.h> //Available from https://github.com/adafruit/RTClib

#define DHTPIN 3    // what pin the DHT is connected to
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);
int photo_pin = A0; 
File data; //initializes "data" as a file name

MPL3115A2 baro;

RTC_DS1307 RTC;
char temp;
char alt;


int sleepTime;

void setup(){
  SD.begin(10); //This is the chipselect pin, change based on SD shield documentation.
  dht.begin();
  Wire.begin();
  RTC.begin();
  baro.begin();
  configure();
  createHeader();
  RTC_set(); 
  data.close();
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
  if(SD.exists("config/alt.txt")){
    data.print(altM);
    data.print(",");
  }
  data.print(analogRead(photo_pin));
  data.print(",");
  photoQual();
  data.println();
  data.close();
  sleep();

}

void createHeader(){ //This function creates a header at the top of the data.csv file.
  if(SD.exists("data.csv") == false){  //only creates header if file doesn't exist
    data=SD.open("data.csv", FILE_WRITE);
    data.print("Time");
    data.print(",");
    if(temp=='f'||temp=='F'){
      data.print("Temperature (F)");
    }else{
      data.print("Temperature (C)");
    }
    data.print(",");
    data.print("Humidity");
    data.print(",");
    data.print("Pressure (in Hg)");
    data.print(",");
    if(SD.exists("config/alt.txt")){
     if(alt=='f'||alt=='F'){
       data.print("Altitude (Feet)");
     }else{
    data.print("Altitude (meters)");
     }
    data.print(",");
    }
    data.print("Light (Raw value 0-1023)");
    data.print(",");
    data.print("Light (qualitative descriptor)");
    data.println();
    data.close();
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

void timestamp(){ //Current time is read from the RTC, and printed to the SD card.
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

void configure(){ //This function reads the contents of the config folder on the SD card, and configures data collection accordingly.
  temp=SD.open("config/temp.txt", FILE_READ).read();  
  alt=SD.open("config/alt.txt", FILE_READ).read();
   File timetxt;
   String time;
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



void sleep(){ //Sleeps the arduino for the ammount of time specified by the config file, using the Narcoleptic sleep library.
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

void RTC_set(){ //sets rtc from the SD card in the even that it is improperly set.
  if(SD.exists("config/rtcset.txt")){
    File rtcset;
    String timebuffer;
    int timenow[5];
    int i=0;
    rtcset=SD.open("config/rtcset.txt",FILE_READ);
     while(rtcset.peek()!=';'){
      while(rtcset.peek()==' '||rtcset.peek()==','){
        rtcset.read();
      }
      while(rtcset.peek()!=' '&&rtcset.peek()!=','&&rtcset.peek()!=';'){  
        char c=rtcset.read();
        timebuffer += c;
        
      }
      
      timenow[i]=timebuffer.toInt();
      i++;
      timebuffer="";
   
     } 
      RTC.adjust(DateTime(timenow[0],timenow[1],timenow[2],timenow[3],timenow[4],timenow[5]));
     SD.remove("config/rtcset.txt");  
  }
}


