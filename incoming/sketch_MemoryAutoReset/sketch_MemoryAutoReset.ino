/**
 * Say hello to Sainsmart UNO China clone. Cheap on Ebay, free USB A-B charger cord.
 * 30 of them arrived in a box this week!
 * 
 * 
 * Sketch also works well as a memory leak reset detector example, resets on < 250 free.
 * 
 * @github ntbrock
 * @gist: Arduino UNO Sketch - Memory Auto Reset low limit SRM malloc debugging
 * @repo: https://github.com/ntbrock/bobino
 * 2015Jun20 Brockman
 *
 */

//-------------------------------------------------------------------------
// Build Time #DEF Configuration

#define LOW_MEMORY_FREE_REBOOT_THRESHOLD 450
#define LOOP_DELAY_MS 50
#define EEPROM_ADDR_Reset_Counter 0


/***
 * SAMPLE Serial 9600 output:
 
  :21 [36.101] SRAM Free: (Resetting at 250 free w/o warning)  499
  :21 [36.102] SRAM Free: (Resetting at 250 free w/o warning)  499
  :72 ~~~ Leaking memory 1/13th of time
  :21 [36.103] SRAM Free: (Resetting at 250 free w/o warning)  369
  :25 SRAM Free: (RESEï¿½ï¿½ï¿½ï¿½ï¿½MQï¿½-------------------------------------------
  :66 address[0]  Total Reset Count: 37
  :21 [37.1] SRAM Free: (Resetting at 250 free w/o warning)  1669
  :21 [37.2] SRAM Free: (Resetting at 250 free w/o warning)  1669
  :72 ~~~ Leaking memory 1/13th of time
  :21 [37.3] SRAM Free: (Resetting at 250 free w/o warning)  1539
  :21 [37.4] SRAM Free: (Resetting at 250 free w/o warning)  1539
  :21 [37.5] SRAM Free: (Resetting at 250 free w/o warning)  1539
 */



//-------------------------------------------------------------------------
// .H

//http://www.arduino.cc/en/Tutorial/EEPROMWrite
#include <EEPROM.h>

//http://www.instructables.com/id/two-ways-to-reset-arduino-in-software/step2/using-just-software/
void(* resetFunc) (void) = 0; //declare reset function @ address 0

//-------------------------------------------------------------------------
// Constants

// the current address in the EEPROM (i.e. which byte
// we're going to write to next)
byte resetCount = 0;

uint8_t freeMemCount = 0;
uint16_t freeMem() {
  char top;
  extern char *__brkval;
  extern char __bss_end;
  int freeMem = __brkval ? &top - __brkval : &top - &__bss_end;
  freeMemCount += 1;

  Serial.print(F(":66 ["));
  Serial.print(resetCount);
  Serial.print(F("."));
  Serial.print( freeMemCount );
  Serial.print(F("] SRAM Free: (Resetting at 450 free w/o warning)  "));
  Serial.println( freeMem );

  if ( freeMem < LOW_MEMORY_FREE_REBOOT_THRESHOLD ) { 
   Serial.print(F(":74 SRAM Free: (RESETTING NOW)  "));
    Serial.println( freeMem );
    delay(LOOP_DELAY_MS);
    resetFunc();  //call reset - no room for death squawk
  }
}


void setup() {
   Serial.begin(9600);
   Serial.println(":84 RESET -------------------------------------------"); 

  // Check eeprom


   resetCount  = EEPROM.read(EEPROM_ADDR_Reset_Counter);
    Serial.print(F(":90 address["));
    Serial.print(EEPROM_ADDR_Reset_Counter);
    Serial.print(F("]  Total Reset Count: "));
    Serial.println(resetCount);
    
  EEPROM.write(EEPROM_ADDR_Reset_Counter, resetCount+1);

}


void loop() {
  // 2015-Jun-20
  freeMem();
  delay(LOOP_DELAY_MS); 

  int i = 127;
  int n = 0;
  char * buffer;

  //Serial.println(":51 Pre malloc");
  
  buffer = (char*) malloc (i+1);
  if (buffer==NULL) {
    Serial.println(":13 !!! BUFFER == null ; Allocation of bytes size=" + i);
    exit (1);
  }

   // Serial.println(":59 Pre Random write");

  for (n=0; n<i; n++) {
    buffer[n]=rand()%26+'a';
  }
  buffer[i]='\0';


  //Serial.print(F(":64 Random string: "));
  //Serial.println(n);

  if ( rand() % 13 == 0 ) {
    //Create Memory Leak 1/13th of time
    Serial.println(":30 ~~~ Leaking memory 1/13th of time");
  } else {
    free (buffer);
  }
   
}
