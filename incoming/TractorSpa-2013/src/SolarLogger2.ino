// A simple data logger for the Arduino analog pins with optional DS1307
// uses RTClib from https://github.com/adafruit/RTClib
#include <SdFat.h>
#include <SdFatUtil.h>  // define FreeRam()

const int ledPin = 13;

#define SD_CHIP_SELECT  SS  // SD chip select pin
#define USE_DS1307       0  // set nonzero to use DS1307 RTC
//#define LOG_INTERVAL  300000  // mills between entries = 300,000 = 5 minute reads
#define SENSOR_COUNT     5  // number of analog pins to log
#define ECHO_TO_SERIAL   1  // echo data to serial port if nonzero
#define WAIT_TO_START    0  // Wait for serial input in setup()
#define ADC_DELAY       10  // ADC delay for high impedence sensors

// file system object
SdFat sd;

// text file for logging
ofstream logfile;

// Serial print stream
ArduinoOutStream cout(Serial);

// Funtion prototypes for later
void sleepAndFlashStatusLEDIfValueExceeds(int sensorValue);


// buffer to format data - makes it eaiser to echo to Serial
char buf[80];
//------------------------------------------------------------------------------
#if SENSOR_COUNT > 6
#error SENSOR_COUNT too large
#endif  // SENSOR_COUNT
//------------------------------------------------------------------------------
// store error strings in flash to save RAM
#define error(s) sd.errorHalt_P(PSTR(s))
//------------------------------------------------------------------------------
#if USE_DS1307
// use RTClib from Adafruit
// https://github.com/adafruit/RTClib

// The Arduino IDE has a bug that causes Wire and RTClib to be loaded even
// if USE_DS1307 is false.

#error remove this line and uncomment the next two lines.
//#include <Wire.h>
//#include <RTClib.h>
RTC_DS1307 RTC;  // define the Real Time Clock object
//------------------------------------------------------------------------------
// call back for file timestamps
void dateTime(uint16_t* date, uint16_t* time) {
    DateTime now = RTC.now();

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}
//------------------------------------------------------------------------------
// format date/time
ostream& operator << (ostream& os, DateTime& dt) {
  os << dt.year() << '/' << int(dt.month()) << '/' << int(dt.day()) << ',';
  os << int(dt.hour()) << ':' << setfill('0') << setw(2) << int(dt.minute());
  os << ':' << setw(2) << int(dt.second()) << setfill(' ');
  return os;
}
#endif  // USE_DS1307
//------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial){}  // wait for Leonardo


  // 2013-Aug-11, Added initialization from AnalogInputTb project

  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);  

  Serial.println("SolarLogger: Setting input reference to external comparator");  
  analogReference(EXTERNAL);

  
  // pstr stores strings in flash to save RAM
  cout << endl << pstr("FreeRam: ") << FreeRam() << endl;

#if WAIT_TO_START
  cout << pstr("Type any character to start\n");
  while (Serial.read() <= 0) {}
  delay(400);  // catch Due reset problem
#endif  // WAIT_TO_START

#if USE_DS1307
  // connect to RTC
  Wire.begin();
  if (!RTC.begin()) error("RTC failed");

  // set date time callback function
  SdFile::dateTimeCallback(dateTime);
  DateTime now = RTC.now();
  cout  << now << endl;
#endif  // USE_DS1307

  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  if (!sd.begin(SD_CHIP_SELECT, SPI_HALF_SPEED)) sd.initErrorHalt();

  // create a new file in root, the current working directory
  char name[] = "LOGGER00.CSV";

  for (uint8_t i = 0; i < 100; i++) {
    name[6] = i/10 + '0';
    name[7] = i%10 + '0';
    if (sd.exists(name)) continue;
    logfile.open(name);
    break;
  }
  if (!logfile.is_open()) error("file.open");

  cout << pstr("Logging to: ") << name << endl;
//  cout << pstr("DISABLED: Type any character to stop\n\n");
  cout << pstr("Power Off Device to close file\n\n");
  
  // format header in buffer
  obufstream bout(buf, sizeof(buf));

  bout << pstr("millis");

#if USE_DS1307
  bout << pstr(",date,time");
#endif  // USE_DS1307

  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    bout << pstr(",sens") << int(i);
  }
  logfile << buf << endl;

#if ECHO_TO_SERIAL
  cout << buf << endl;
#endif  // ECHO_TO_SERIAL
}
//------------------------------------------------------------------------------
int sensorValue = 0;
void loop() {
  uint32_t m;

  // wait for time to be a multiple of interval
  do {
    m = millis();
  } while (m % LOG_INTERVAL);

  // use buffer stream to format line
  obufstream bout(buf, sizeof(buf));

  // start with time in millis
  bout << m;

#if USE_DS1307
  DateTime now = RTC.now();
  bout << ',' << now;
#endif  // USE_DS1307


  // read analog pins and format data
  for (uint8_t ia = 0; ia < SENSOR_COUNT; ia++) {
#if ADC_DELAY
    analogRead(ia);
    delay(ADC_DELAY);
#endif  // ADC_DELAY
    int value = analogRead(ia);
    bout << ',' << value;
    if ( ia == 0 ) { sensorValue = value; } // Brockman Record sensor zero for flash lights.
  }
  bout << endl;

  // log data and flush to SD
  logfile << buf << flush;

  // check for error
  if (!logfile) error("write data failed");

#if ECHO_TO_SERIAL
  cout << buf;
#endif  // ECHO_TO_SERIAL

  // don't log two points in the same millis
  if (m == millis()) delay(1);

  // Brockman: Enable the 'voltage high range flaser'
//  sleepAndFlashStatusLEDIfValueExceeds(sensorValue);

  
//  if (!Serial.available()) return;
//  logfile.close();
//  cout << pstr("Done!");
//  while (1);
}

//-------------------------------------------------------------------------

/**
 * Method takes in a 0 - 1023 Analog to Digital converted value and flashes the 
 * primary statusLed if values exceed the maximum. This lets an installer tune the
 * potentiometers within good range.
 *  Code cloned from AnalogInputTb Ardrunio project from 2013 July, Brockman
 */

void sleepAndFlashStatusLEDIfValueExceeds(int sensorValue) {
  Serial.print("SolarLogger: sleep value : ");
  Serial.println(sensorValue);
  
   if ( sensorValue >= 1023 ) {
     // turn the ledPin on solid
     digitalWrite(ledPin, HIGH);  
     delay(1000);

   } else if ( sensorValue >= 900 ) { 
     // blink the LED letting us know we are within good max, 90%
         
     // turn the ledPin on solid
     digitalWrite(ledPin, HIGH);  
     delay(100);
     digitalWrite(ledPin, LOW);  
     delay(100);
     digitalWrite(ledPin, HIGH);  
     delay(100);
     digitalWrite(ledPin, LOW);  
     delay(100);
     digitalWrite(ledPin, HIGH);  
     delay(100);
     digitalWrite(ledPin, LOW);  
     delay(100);
     digitalWrite(ledPin, HIGH);  
     delay(100);
     digitalWrite(ledPin, LOW);  
     delay(100);
     digitalWrite(ledPin, HIGH);  
     delay(100);
     digitalWrite(ledPin, LOW);  
     delay(100);
     
   } else { 
     // turn the ledPin off:        
     digitalWrite(ledPin, LOW);   
     // no light - within regular bounds
     delay(1000);

   }
 
  return; 
}

