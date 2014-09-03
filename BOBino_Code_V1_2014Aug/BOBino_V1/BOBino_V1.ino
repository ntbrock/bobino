/***********************************************************
 * BOBino V1: An Arduino Sensor Platform by Max Tucker
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
 * 
 * Change Log:
 *  Brockman 2014-Sep-02: Defaulted configurations for time so RAW SD card works
 *  Brockman 2014-Sep-02: Serial writing for debugging added
 *  Brockman 2014-Sep-02: Memory optimization http://forum.arduino.cc/index.php?topic=163307.0
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
#define PHOTO_PIN A0
//File data; //initializes "data" as a file name

MPL3115A2 baro;
RTC_DS1307 RTC;

char temp = 'F';
char alt = 'F';
char showAlt = 0;
short sleepTime = 1;

#define CSV ","
#define DATA_FILENAME "data.csv"
#define RTC_FILENAME "config/rtcset.txt"
#define CONFIG_DIR "config"
#define TIME_FILENAME "config/time.txt"
#define TEMP_FILENAME "config/temp.txt"
#define ALT_FILENAME "config/alt.txt"
#define RTC_EXAMPLE "config/_rtcset.txt"
#define README_FILENAME "BOBino.txt"

uint16_t freeMem() {
  char top;
  extern char *__brkval;
  extern char __bss_end;
  Serial.print(F("Freemem: (Below 800 is a bad thing!)  "));
  Serial.println( __brkval ? &top - __brkval : &top - &__bss_end);
}

void setup(){
  Serial.begin(9600);
  
  Serial.println(F("BOBino V1 ================ "));
  freeMem();

  delay(500);
  
  //This is the chipselect pin, change based on SD shield documentation.
  if ( ! SD.begin(10) ) { 
    Serial.println(F("ERROR: Unable to initalize SD Card. Can not continue."));
  } else {     
    dht.begin();
    Wire.begin();
    RTC.begin();
    baro.begin();

    delay(250);
  
    configure();

    createHeader();
    RTC_set(); 

    baro.setOversampleRate(7);
    baro.enableEventFlags();
    baro.setModeStandby();
  
    Serial.println(F("BOBino V1 - setup complete"));
  }
}

void loop(){
  Serial.print(".");
  float pascals=getPressure();
  float inHg=pascals/3386.389; 
  float altM=getAlt();
  baro.setModeActive();
  float deg=getTemp();
  baro.setModeStandby();

  File data=SD.open(DATA_FILENAME, FILE_WRITE);
  if ( data ) { 
    timestamp(data);
    data.print(CSV);
    data.print(deg);
    data.print(CSV);
    data.print(dht.readHumidity()); //DHT readTemperature is also available, but less accurate than other sensor
    data.print(CSV);
    data.print(inHg);
    data.print(CSV);
    if(showAlt > 0){
      data.print(altM);
      data.print(CSV);
    }
    data.print(analogRead(PHOTO_PIN));
    data.print(CSV);
    photoQual(data);
    data.println();
    data.close();
  } else { 
     Serial.println(F("BOBino V1 - ERROR: Unable to write to DATA_FILENAME"));
  }
  
  sleep();

}

void createHeader(){ //This function creates a header at the top of the data.csv file.
  if(SD.exists(DATA_FILENAME) == false){  //only creates header if file doesn't exist
    File data=SD.open(DATA_FILENAME, FILE_WRITE);
    data.print("Time");
    data.print(CSV);
    if(temp=='f'||temp=='F'){
      data.print(F("Temperature (F)"));
    }else{
      data.print(F("Temperature (C)"));
    }
    data.print(CSV);
    data.print(F("Humidity"));
    data.print(CSV);
    data.print(F("Pressure (in Hg)"));
    data.print(CSV);
    if(SD.exists(ALT_FILENAME)){
     if(alt=='f'||alt=='F'){
       data.print(F("Altitude (Feet)"));
     }else{
    data.print(F("Altitude (meters)"));
     }
    data.print(CSV);
    }
    data.print(F("Light (Raw value 0-1023)"));
    data.print(CSV);
    data.print(F("Light (qualitative descriptor)"));
    data.println();
    data.close();
  }
  else{
    // File handle is refereshed with every loop, not held in memory.
    // data=SD.open("data.csv", FILE_WRITE);
  }
}

void photoQual(File data){ 
  //below prints qualitative descriptors of light intensity
  if(analogRead(PHOTO_PIN) < 10){
    data.print(F("dark"));
  }
  else if(analogRead(PHOTO_PIN)<100){
    data.print(F("dim"));
  }
  else if(analogRead(PHOTO_PIN)<300){
    data.print(F("light"));
  }
  else if(analogRead(PHOTO_PIN)<600){
    data.print(F("bright"));
  }
  else{
    data.print(F("very bright"));
  }
}

void timestamp(File data){ //Current time is read from the RTC, and printed to the SD card.
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
  
  Serial.println(F("BOBino V1 - Configure beginning"));

  // Create the config directory on the card if doesn't exist
  if ( SD.exists( CONFIG_DIR ) ) {
      Serial.println(F("Good: Config directory: CONFIG_DIR exists."));
  } else { 
     if ( SD.mkdir( CONFIG_DIR ) ) { 
       Serial.println(F("Created Directory: CONFIG_DIR"));
     } else { 
      Serial.println(F("ERROR: Unable to create directory: CONFIG_DIR"));
    }
  }
  
  Serial.println(F("Info: Initializing Time"));
  
  if ( SD.exists( TIME_FILENAME ) ) {
    File timetxt;
    String time;
    timetxt=SD.open(TIME_FILENAME,FILE_READ);
    if(timetxt > 0 ) {
      char c = timetxt.read();
      while(c!=';' && c != -1){
      Serial.print( c );
        time += c;
        c=timetxt.read();
      }
      Serial.println();
    }
    if(!timetxt || !time){
      sleepTime=1;
    }else{
    sleepTime=atoi(time.c_str());
    }
    
    if ( sleepTime <= 0 ) { sleepTime = 1; } // Defensive.
    
    Serial.print(F("Info: Configured Sleep time to be: "));
    Serial.print(sleepTime);
    Serial.println();
    
  } else { 
     // Create the default value file on the new SD card. 
    // Create the default value file on the new SD card. 
    File fh=SD.open(TIME_FILENAME, FILE_WRITE);
    if ( fh ) { 
      fh.print( sleepTime );
      fh.print(";");
      fh.println();
      fh.flush();
      fh.close();
      Serial.println(F("Wrote file: TIME_FILENAME"));
    } else { 
      Serial.println(F("ERROR: Unable to create file: TIME_FILENAME"));
    }
  }
 
  
  if ( SD.exists( TEMP_FILENAME ) ) {
    temp=SD.open(TEMP_FILENAME, FILE_READ).read();  
  } else { 
    // Create the default value file on the new SD card. 
    File fh=SD.open(TEMP_FILENAME, FILE_WRITE);
    if ( fh ) { 
      fh.print( temp );
      fh.print( ";");
      fh.println();
      fh.flush();
      fh.close();
      Serial.println(F("Wrote file: TEMP_FILENAME"));
    } else { 
      Serial.println(F("ERROR: Unable to create file: TEMP_FILENAME"));
    }
  }
  if ( temp != 'F' && temp != 'f' && temp != 'C' && temp != 'c' ) { temp = 'F'; } // Defense!
  Serial.print(F("Info: Configured Temp to be: "));
  Serial.print(temp);
  Serial.println();
    
    
  if ( ! SD.exists( RTC_EXAMPLE ) ) {
    // Create the default value file on the new SD card. 
    File fh=SD.open(RTC_EXAMPLE, FILE_WRITE);
    if ( fh ) { 
      fh.print( F("2014 09 01 15 00 00;"));
      fh.println();
      fh.flush();
      fh.close();
      Serial.println(F("Wrote file: RTC_EXAMPLE"));
    } else { 
      Serial.println(F("ERROR: Unable to create file: RTC_EXAMPLE"));
    }
  }

  if ( SD.exists( ALT_FILENAME ) ) {
     alt=SD.open(ALT_FILENAME, FILE_READ).read();
     showAlt = 1;
  } else {
      // Don't Create the default value file on the new SD card. 
    Serial.println(F("Warning, No file: ALT_FILENAME"));
  }
  Serial.print(F("Info: Configured Alt to be: "));
  if ( showAlt ) { Serial.print("DISPLAYED"); } else { Serial.print("Not Displayed"); }
  Serial.println();
    
  if ( ! SD.exists( README_FILENAME ) ) {
    // Create the default value file on the new SD card. 
    File fh=SD.open(README_FILENAME, FILE_WRITE);
    if ( fh ) {
    fh.print(F("BOBino V1 2014-Sep-01"));
    fh.println();
    fh.println();
    fh.print(F("Edit The following files to set Configuration: "));
    fh.println();
    fh.println(F("   CONFIG/TEMP.TXT -- Set Temperature Units to F or C"));
    fh.println(F("   CONFIG/TIME.TXT -- Set Time Sleep Interval in minutes"));
    fh.println(F("   CONFIG/ALT.TXT -- Set Altitude Units and Activate"));
    fh.println(F("   CONFIG/RTCSET.TXT -- Set To the current calendar information to adjust system time."));
    fh.println(F("   CONFIG/_RTCSET.TXT -- Example RTC File to make clock setting easy."));
    fh.print("");
    fh.println();
//    fh.flush();
    fh.close();
    Serial.println(F("Wrote file: README_FILENAME"));
    }
  }



    DateTime now = RTC.now();   
    Serial.print(F("Info: System Time is: "));
    Serial.print(now.year(), DEC);
    Serial.print(F("/"));
    Serial.print(now.month(), DEC);
    Serial.print(F("/"));
    Serial.print(now.day(), DEC);
    Serial.print(F("T"));
    Serial.print(now.hour(), DEC);
    Serial.print(F(":"));
    Serial.print(now.minute(), DEC);
    Serial.print(F(":"));
    Serial.print(now.second(), DEC);
    Serial.println();

   Serial.print(F("Info: Unix Time is: "));
   Serial.print(getUnixTime());
   Serial.println();
   
  Serial.println(F("BOBino V1 - Configure complete"));
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
  if(SD.exists(RTC_FILENAME)){
    File rtcset;
    String timebuffer;
    short timenow[5];
    timenow[0] = 0;timenow[1] = 0;timenow[2] = 0;timenow[3] = 0;timenow[4] = 0;timenow[5] = 0;
    int i=0;
    rtcset=SD.open(RTC_FILENAME,FILE_READ);
     while(rtcset.peek()!=';'){
      while(rtcset.peek()==' '||rtcset.peek()==','){
        rtcset.read();
      }
      while(rtcset.peek()!=' '&&rtcset.peek()!=','&&rtcset.peek()!=';'){  
        char c=rtcset.read();
        timebuffer += c;
        
      }
      Serial.print(F("DEBUG> Time Buffer: "));
      Serial.print(timebuffer);
      Serial.println();
      timenow[i]=timebuffer.toInt();

      Serial.print(F("DEBUG> Time Now: "));
      Serial.print(timenow[i]);
      Serial.println();

      i++;
      timebuffer="";
   
     } 
     RTC.adjust(DateTime(timenow[0],timenow[1],timenow[2],timenow[3],timenow[4],timenow[5]));
     SD.remove(RTC_FILENAME);  

    Serial.print(F("INFO: Adjusted Real Time Clock to: "));
    Serial.print( timenow[0], DEC );
    Serial.print( F("/") );
    Serial.print( timenow[1], DEC );
    Serial.print( F("/") );
    Serial.print( timenow[2], DEC );
    Serial.print( F("T") );
    Serial.print( timenow[3], DEC );
    Serial.print( F(":") );
    Serial.print( timenow[4], DEC );
    Serial.print( F(":") );
    Serial.print( timenow[5], DEC );
    Serial.println();
  }
}



