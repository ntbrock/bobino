#include <SD.h>
#include <Wire.h>
#include <RTClib.h>


RTC_DS1307 RTC;

  
void setup(){
SD.begin();
Serial.begin(9600);
RTC.begin();
Wire.begin();
RTC_set();
}

void loop(){
DateTime now = RTC.now();

    
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
    Serial.println();
    
    delay(3000);
}

void RTC_set(){
  File rtcset;
  String timebuffer;
  int timenow[5];
  int i=0;
  if(SD.exists("config/rtcset.txt")){
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
   
  }
}
   
