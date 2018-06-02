
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

char s[256] = "";

void loop() {
  // put your main code here, to run repeatedly:

  sprintf(s, "Loop %d", loopCount);
  
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(0,24,s );
  } while ( u8g2.nextPage() );


  Serial.print(s);
  Serial.print("\n");
  
  loopCount++;
  delay(1000);
}
