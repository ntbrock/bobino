#include <TimeLib.h>
#include <Time.h>


#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>


//U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, 13, 11, 10, 8);
// U8G2_SSD1306_128X64_NONAME_HW_I2C u8g2()


U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE); // All Boards without Reset of the Display




/** 
 *  2018Feb25 Oled Demo Brockman Seatop Bobino
 */


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  Serial.print("setup\n");
  
  u8g2.begin();

}

int loopCount = 0;

char l1[13] = "_____";
char l2[13] = "";
char l31[4] = "";
char l32[4] = "";
char l33[4] = "";

time_t t = now(); 


void loop() {
  // put your main code here, to run repeatedly:

  t = now(); 
 
  sprintf(l1, "Time %d:%02d:%02d", hour(t), minute(t), second(t) );
  sprintf(l2, "Photo  %d", 255);

  sprintf(l31, "%dF", 32);
  sprintf(l32, "%d", 102);
  sprintf(l33, "%d", 74);
  
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    
    u8g2.drawStr(0,14,l1 );
    u8g2.drawStr(0,38,l2 );

    u8g2.drawStr(0,63,l31 );
    u8g2.drawStr(50,63,l32 );
    u8g2.drawStr(100,63,l33 );
  } while ( u8g2.nextPage() );


/*
  Serial.print(l1);
  Serial.print("\n");
  Serial.print(l2);
  Serial.print("\n");
*/
  
  loopCount++;
  delay(1000);
}
