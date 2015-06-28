/***********************************************************
 * BOBino V1.1: An Arduino Sensor Platform by Max Tucker, Taylor Brockman
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
 *  Brockman 2015-Jun-18: Inclusion of the DallasTemperature and Onewire libs
 *      Dedpopulated the MPL and DHT to opt for 3 temp sensors!
 *  Brockman 2014-Sep-02: Defaulted configurations for time so RAW SD card works
 *  Brockman 2014-Sep-02: Serial writing for debugging added
 *  Brockman 2014-Sep-02: Memory optimization http://forum.arduino.cc/index.php?topic=163307.0
 */

#include <Wire.h>

// 2015-Jun - Replaced the components with simple DS18B20 one-wire temperature sensors
#include <OneWire.h>
#include <DallasTemperature.h>

// SD Card and Sleep
#include <SPI.h>
#include <SD.h>
#include <avr/sleep.h>

#include <RTClib.h> //Available from https://github.com/adafruit/RTClib
#include <Narcoleptic.h> //microcontroller sleep library. Available at https://code.google.com/p/narcoleptic/downloads/detail?name=Narcoleptic_v1a.zip

//http://www.arduino.cc/en/Tutorial/EEPROMWrite
#include <EEPROM.h>

//http://www.instructables.com/id/two-ways-to-reset-arduino-in-software/step2/using-just-software/
void(* resetFunc) (void) = 0; //declare reset function @ address 0


#define DEFAULT_SLEEP_SECONDS 15


// Enable up to three differene one wire busses
// This avoids identifying each one-wire by hex id.
#define ONE_WIRE_BUS_A   2
#define ONE_WIRE_BUS_B   3
#define ONE_WIRE_BUS_C   4

// Analog Voltage based Light Sennsor
#define PHOTO_PIN A0

RTC_DS1307 RTC;
OneWire oneWireA(ONE_WIRE_BUS_A);
OneWire oneWireB(ONE_WIRE_BUS_B);
OneWire oneWireC(ONE_WIRE_BUS_C);

DallasTemperature sensorsA(&oneWireA);
DallasTemperature sensorsB(&oneWireB);
DallasTemperature sensorsC(&oneWireC);

char temp = 'F';
char alt = 'F';
short sleepSeconds = DEFAULT_SLEEP_SECONDS;

#define CSV ","
#define DATA_FILENAME "data.csv"
#define RTC_FILENAME "config/rtcset.txt"
#define CONFIG_DIR "config"
#define TIME_FILENAME "config/sleepsec.txt"
#define TEMP_FILENAME "config/temp.txt"
#define ALT_FILENAME "config/alt.txt"
#define RTC_EXAMPLE "config/_rtcset.txt"
#define README_FILENAME "BOBino.txt"

#define LOW_MEMORY_FREE_REBOOT_THRESHOLD 350
#define EEPROM_ADDR_Reset_Counter 0

//https://gist.github.com/ntbrock/4d0bec130f943861a0ba
byte resetCount = 0;
byte freeMemCount = 0;
uint16_t freeMem() {
  char top;
  extern char *__brkval;
  extern char __bss_end;
  int freeMem = __brkval ? &top - __brkval : &top - &__bss_end;

  if ( freeMemCount % 100 == 0 ) { 
  Serial.print(F("Debug: ["));
  Serial.print( freeMemCount, DEC );
  Serial.print(F("] SRAM Free: (Resetting at 350 free with NO warning)  "));
  Serial.println( freeMem );
  }
  freeMemCount++;
  
  if ( freeMem < LOW_MEMORY_FREE_REBOOT_THRESHOLD ) { 
   Serial.print(F("FATAL: SRAM Free: (RESETTING NOW)  "));
    Serial.println( freeMem, DEC );
    Serial.flush();
    delay(50);
    resetFunc();  //call reset - no room for death squawk
  }
}

void setup() {

  Serial.begin(9600);
  
  Serial.println(F("BOBino V1.1 ==[setup]===="));
  freeMem();
  countResets();
  delay(50);


  //This is the chipselect pin, change based on SD shield documentation.
  if ( ! SD.begin(10) ) { 
    Serial.println(F("FATAL: Unable to begin SD Card."));
    Serial.flush();
    deathFlash();
  } else {     

    sensorsA.begin();
    sensorsB.begin();
    sensorsC.begin();
    delay(50);

    Wire.begin();
    RTC.begin();

    delay(50);
    configure();

    createHeader();
    RTC_set(); 

    sensorsA.setResolution(10);
    sensorsB.setResolution(10);
    sensorsC.setResolution(10);

    Serial.print(F("BOBino V1.1 ==[loop]====  Go! Loop sleeps seconds: "));
    Serial.println(sleepSeconds);
  }
}

int loopCount = 0;
long lastReadMs = -1;

void loop(){
  loopCount += 1;
  freeMem();

  lastReadMs = millis() + Narcoleptic.millis();
  DateTime now = RTC.now();

  // Acquire All readings
  sensorsA.requestTemperatures(); 
  sensorsB.requestTemperatures(); 
  sensorsC.requestTemperatures(); 
  byte photoRead = analogRead(PHOTO_PIN);
    
  // Acquisition Time off temperature sensors
  delay(650);

  float tempA = -1;
  float tempB = -1;
  float tempC = -1; 
  
  if(temp=='f'||temp=='F'){
    tempA = sensorsA.getTempFByIndex(0);
    tempB = sensorsB.getTempFByIndex(0);
    tempC = sensorsC.getTempFByIndex(0);
  } else {
    tempA = sensorsA.getTempCByIndex(0);
    tempB = sensorsB.getTempCByIndex(0);
    tempC = sensorsC.getTempCByIndex(0);
  }

  // 32.00 indicates error, missing device on bus
  if ( tempA == 32.00 ) { tempA = -1; }
  if ( tempB == 32.00 ) { tempB = -1; }
  if ( tempC == 32.00 ) { tempC = -1; }

  File data=SD.open(DATA_FILENAME, FILE_WRITE);
  if ( data ) { 

    timestamp(data, now);
    data.print(CSV);
    data.print(lastReadMs, DEC);
    data.print(CSV);
    data.print(loopCount, DEC);
    data.print(CSV);
    data.print(resetCount, DEC);
    data.print(CSV);
    data.print(tempA, 2);
    data.print(CSV);
    data.print(tempB, 2);
    data.print(CSV);
    data.print(tempC, 2);
    data.print(CSV);
    data.print(photoRead, DEC);
    data.print(CSV);
    photoQual(photoRead,data);
    data.println();
    data.close();

    // Also debug to serial console
    timestampSerial(now);
    Serial.print(CSV);
    Serial.print(lastReadMs, DEC);
    Serial.print(CSV);
    Serial.print(loopCount, DEC);
    Serial.print(CSV);
    Serial.print(resetCount, DEC);
    Serial.print(CSV);
    Serial.print(tempA, 2);
    Serial.print(CSV);
    Serial.print(tempB, 2);
    Serial.print(CSV);
    Serial.print(tempC, 2);
    Serial.print(CSV);
    Serial.print(photoRead, DEC);
    Serial.print(CSV);
    Serial.println();
    Serial.flush();
  } else { 
    Serial.print(F("FATAL: Unable to write to SD, File: "));
    Serial.println(DATA_FILENAME);
    Serial.flush();
    deathFlash();
  }
  
  sleep();

}

// Death Flash
void deathFlash() { 
  cli();
  sleep_enable();
  sleep_cpu();
}


void createHeader(){ //This function creates a header at the top of the data.csv file.
  if(SD.exists(DATA_FILENAME) == false){  //only creates header if file doesn't exist
    File data=SD.open(DATA_FILENAME, FILE_WRITE);
    data.print("Time");
    data.print(CSV);
    data.print("LastReadMs");
    data.print(CSV);
    data.print("LoopCount");
    data.print(CSV);
    data.print("RebootCount");
    data.print(CSV);
    if(temp=='f'||temp=='F'){
      data.print(F("TemperatureA (F)"));
    }else{
      data.print(F("TemperatureA (C)"));
    }
    data.print(CSV);
    if(temp=='f'||temp=='F'){
      data.print(F("TemperatureB (F)"));
    }else{
      data.print(F("TemperatureB (C)"));
    }
    data.print(CSV);
    if(temp=='f'||temp=='F'){
      data.print(F("TemperatureC (F)"));
    }else{
      data.print(F("TemperatureC (C)"));
    }
    data.print(CSV);
    data.print(F("Light (0-1023)"));
    data.print(CSV);
    data.print(F("Light (Descriptor)"));
    data.println();
    data.close();
  }
  else{
    // File handle is refereshed with every loop, not held in memory.
    // data=SD.open("data.csv", FILE_WRITE);
  }
}

void photoQual(float reading, File data){ 
  //below prints qualitative descriptors of light intensity
  if(reading < 10){
    data.print(F("Dark"));
    //data.print(F("dark"));
  }
  else if(reading<100){
    data.print(F("Dim"));
    //Serial.print(F("dim"));
  }
  else if(reading<300){
    data.print(F("Light"));
    //Serial.print(F("light"));
  }
  else if(reading<600){
    data.print(F("Bright"));
    //Serial.print(F("bright"));
  }
  else{
    data.print(F("Very Bright"));
    //Serial.print(F("very bright"));
  }
}

void timestamp(File data, DateTime &now){ //Current time is read from the RTC, and printed to the SD card.
  //timestamp from rtc
  data.print(now.year(), DEC);
  data.print('/');
  data.print(now.month(), DEC);
  data.print('/');
  data.print(now.day(), DEC);
  data.print(' ');
  if ( now.hour() < 10 ) { data.print("0"); }
  data.print(now.hour(), DEC);
  data.print(':');
  if ( now.minute() < 10 ) { data.print("0"); }
  data.print(now.minute(), DEC );
  data.print(':');
  if ( now.second() < 10 ) { data.print("0"); }
  data.print(now.second(), DEC);
}

void timestampSerial(DateTime &now){ //Current time is read from the RTC, and printed to the SD card.
  Serial.print(now.unixtime());
}


/**
 * Read the beginning of the EEPROM to get the number of resets and + 1.
 */
void countResets() {

  resetCount  = EEPROM.read(EEPROM_ADDR_Reset_Counter);
    Serial.print(F("Debug: Total Reset Count: "));
    Serial.println(resetCount);
  EEPROM.write(EEPROM_ADDR_Reset_Counter, resetCount+1);  
}


/**
 * Read the contents of the config folder on the SD card,
 * set memory config for data collection accordingly.
 */
  
void configure(){ 
  //Serial.println(F("BOBino V1.1 - Configure Start"));

  // Create the config directory on the card if doesn't exist
  if ( SD.exists( CONFIG_DIR ) ) {
      Serial.println(F("Info: SD Card Config directory: CONFIG exists."));
  } else { 
     if ( SD.mkdir( CONFIG_DIR ) ) { 
       Serial.println(F("Created Directory: CONFIG_DIR"));
     } else { 
      Serial.println(F("ERROR: Unable to create directory: CONFIG"));
    }
  }
  
  Serial.print(F("Info: Initializing Time. Contents of SLEEPSEC.TXT: "));
  
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
      sleepSeconds = DEFAULT_SLEEP_SECONDS;
    }else{
    sleepSeconds=atoi(time.c_str());
    }
    
    if ( sleepSeconds <= 0 ) { sleepSeconds = DEFAULT_SLEEP_SECONDS; } // Defensive.
    
    Serial.print(F("Info: Configured Sleep Seconds: "));
    Serial.print(sleepSeconds);
    Serial.println();
    
  } else { 
     // Create the default value file on the new SD card. 
    // Create the default value file on the new SD card. 
    File fh=SD.open(TIME_FILENAME, FILE_WRITE);
    if ( fh ) { 
      fh.print( sleepSeconds );
      fh.print(";");
      fh.println();
      fh.flush();
      fh.close();
      Serial.print(F("Wrote file: "));
      Serial.println(TIME_FILENAME);
    } else { 
      Serial.print(F("ERROR: Unable to create file: "));
      Serial.println(TIME_FILENAME);
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
      fh.print( F("2015 01 01 00 00 00;"));
      fh.println();
      fh.flush();
      fh.close();
      Serial.println(F("Wrote file: RTC_EXAMPLE"));
    } else { 
      Serial.println(F("ERROR: Unable to create file: RTC_EXAMPLE"));
    }
  }
    
  if ( ! SD.exists( README_FILENAME ) ) {
    // Create the default value file on the new SD card. 
    File fh=SD.open(README_FILENAME, FILE_WRITE);
    if ( fh ) {
    fh.print(F("BOBino V1.1 2015-Jun-18"));
    fh.println();
    fh.println();
    fh.print(F("Edit files in the CONFIG Directory on this SD Card to set config: "));
    fh.println();
    fh.println(F("   CONFIG/TEMP.TXT -- Set Temperature Units to F or C"));
    fh.println(F("   CONFIG/SLEEPSEC.TXT -- Set Time Sleep Interval in Seconds"));
    //fh.println(F("   CONFIG/ALT.TXT -- Set Altitude Units and Activate"));
    fh.println(F("   CONFIG/RTCSET.TXT -- Set To the current calendar information to adjust system time."));
    fh.println(F("   CONFIG/_RTCSET.TXT -- Example RTC File to make clock setting easy."));
    fh.print("");
    fh.println();
//    fh.flush();
    fh.close();
    Serial.println(F("Wrote file: README.txt"));
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

   long unixTime = getUnixTime();
   Serial.print(F("Info: Unix Time is: "));
   Serial.println(unixTime);
   
  //Serial.println(F("BOBino V1.1 - Configure complete"));
}


uint8_t narcolepticIsWorking = 1;
long narcolepticAlternateTime = 0;

void sleep() { //Sleeps the arduino for the ammount of time specified by the config file, using the Narcoleptic sleep library.
//  long currentTime = getUnixTime();


  // Sensor Reads take ~ 900ms
  // Go off last read to avoid adding reading delays each time.
  long wakeMs = (float)lastReadMs + ( (float)sleepSeconds * 1000.0 ); // Version 1.1 - Changing timesleep to seconds
  long nextMs = -1;
  
  long currentMs = millis() + Narcoleptic.millis();
  while ( currentMs < wakeMs ){

    // Sleep 1 second less than expected
    long sleepMs = wakeMs - currentMs;
    if ( sleepMs < 1000 ) { sleepMs = 100; }

/*
    Serial.print(F("::: BEG current: " ));
    Serial.print(currentMs);
    Serial.print(F("  lastRead: " ));
    Serial.print(lastReadMs);
    Serial.print(F("  sleep: " ));
    Serial.print(sleepMs);
//    Serial.print(F("  sleepSec: "));
//    Serial.print(sleepSeconds);
    Serial.print(F("  wake: "));
    Serial.println(wakeMs);
    Serial.flush();
*/
    if ( sleepMs < 1000 ) { 
      // CPU Narrow in the last second.
      delay(sleepMs);
    } else if ( narcolepticIsWorking ) { 
      Narcoleptic.delay( sleepMs - 450 );
      delay(200);
    } else { 
      // Narco is not working, use our alternate counter.
      delay(sleepMs);
    }


    // Only set this at the end of the loop
    nextMs = millis() + Narcoleptic.millis();  

/*
    Serial.print(F("::: END current: " ));
    Serial.print(currentMs);
    Serial.print(F("  next: " ));
    Serial.println(nextMs);
    Serial.flush();
*/
    // Handle the case where no RTC exists and we need to advance the clock
    
 /*   if ( newTime <= currentTime && narcolepticIsWorking ) {
        Serial.println(F("ERROR - Real Time Clock / Narcoleptic not working. Falling back to CPU delay."));

        Serial.print(F("Logic Path: "));
        Serial.print(path);
        Serial.print(F("  Wake Time: "));
        Serial.print(wakeTime);
        Serial.print(F("  New Time: "));
        Serial.print(newTime);
        Serial.print(F("  Current Time: "));
        Serial.println(currentTime);
        
        narcolepticIsWorking = 0;  
    }
*/

    currentMs = nextMs;
    
  }
}

long getUnixTime(){
  if ( narcolepticIsWorking ) { 
    DateTime now=RTC.now();
    if( now.year() > 2100 ) { 
      Serial.println("ERROR: Real Time Clock not working, year > 2100."); 
      narcolepticIsWorking = 0;
      return narcolepticAlternateTime;        
    } else { 
      return now.unixtime();
    }
  } else { 
    return narcolepticAlternateTime;    
  }
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

    Serial.print(F("INFO: Set Real Time Clock: "));
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



