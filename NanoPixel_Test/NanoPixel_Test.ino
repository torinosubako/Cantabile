 
/*
 * Project:NanoPixel_Test
 * Build:2021/09/24
 * Author:torinosubako
 * Status:Impractical
*/

#include <Adafruit_NeoPixel.h>
#include <M5Core2.h>
#include <Wire.h>
#define PIN 25
#define NUMPIXELS 10
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int delayval = 150;


void setup() {
  M5.begin();  
  pixels.begin();
  M5.Lcd.setTextFont(4);
  M5.Lcd.println(("M5Stack Core2 for AWS"));
  M5.Lcd.println(("Nano Pixel LED Test"));
  //LCD試験用  
  M5.Lcd.drawString("Co2[ppm]", 0, 53, 4);
  M5.Lcd.drawRightString("0000", 319, 53, 7);
  M5.Lcd.drawString("Temp[C]", 0, 106, 4);
  M5.Lcd.drawRightString("00.00", 319, 106, 7);
  M5.Lcd.drawString("Hum[%]", 0, 159, 4);
  M5.Lcd.drawRightString("00.00", 319, 159, 7);
}

void loop() {
  Serial.println("ELECTRICAL COMMUNICATION");

  //1
  pixels.setPixelColor(0, pixels.Color(100,0,0));
  pixels.setPixelColor(1, pixels.Color(50,50,0));
  pixels.setPixelColor(2, pixels.Color(0,100,0));
  pixels.setPixelColor(3, pixels.Color(0,50,50));
  pixels.setPixelColor(4, pixels.Color(0,0,100));
  
  pixels.setPixelColor(5, pixels.Color(100,0,0));
  pixels.setPixelColor(6, pixels.Color(50,50,0));
  pixels.setPixelColor(7, pixels.Color(0,100,0));
  pixels.setPixelColor(8, pixels.Color(0,50,50));
  pixels.setPixelColor(9, pixels.Color(0,0,100));
  pixels.show();
  delay(delayval);

  //2
  pixels.setPixelColor(0, pixels.Color(50,50,0));
  pixels.setPixelColor(1, pixels.Color(0,100,0));
  pixels.setPixelColor(2, pixels.Color(0,50,50));
  pixels.setPixelColor(3, pixels.Color(0,0,100));
  pixels.setPixelColor(4, pixels.Color(50,0,50));
  
  pixels.setPixelColor(5, pixels.Color(50,50,0));
  pixels.setPixelColor(6, pixels.Color(0,100,0));
  pixels.setPixelColor(7, pixels.Color(0,50,50));
  pixels.setPixelColor(8, pixels.Color(0,0,100));
  pixels.setPixelColor(9, pixels.Color(50,0,50));
  pixels.show();
  delay(delayval);

  //3
  pixels.setPixelColor(0, pixels.Color(0,100,0));
  pixels.setPixelColor(1, pixels.Color(0,50,50));
  pixels.setPixelColor(2, pixels.Color(0,0,100));
  pixels.setPixelColor(3, pixels.Color(50,0,50));
  pixels.setPixelColor(4, pixels.Color(100,0,0));

  pixels.setPixelColor(5, pixels.Color(0,100,0));
  pixels.setPixelColor(6, pixels.Color(0,50,50));
  pixels.setPixelColor(7, pixels.Color(0,0,100));
  pixels.setPixelColor(8, pixels.Color(50,0,50));
  pixels.setPixelColor(9, pixels.Color(100,0,0));
  pixels.show();
  delay(delayval);

  //4
  pixels.setPixelColor(0, pixels.Color(0,50,50));
  pixels.setPixelColor(1, pixels.Color(0,0,100));
  pixels.setPixelColor(2, pixels.Color(50,0,50));
  pixels.setPixelColor(3, pixels.Color(100,0,0));
  pixels.setPixelColor(4, pixels.Color(100,100,100));

  pixels.setPixelColor(5, pixels.Color(0,50,50));
  pixels.setPixelColor(6, pixels.Color(0,0,100));
  pixels.setPixelColor(7, pixels.Color(50,0,50));
  pixels.setPixelColor(8, pixels.Color(100,0,0));
  pixels.setPixelColor(9, pixels.Color(100,100,100));
  pixels.show();
  delay(delayval);

  //5
  pixels.setPixelColor(0, pixels.Color(0,0,100));
  pixels.setPixelColor(1, pixels.Color(50,0,50));
  pixels.setPixelColor(2, pixels.Color(100,0,0));
  pixels.setPixelColor(3, pixels.Color(100,100,100));
  pixels.setPixelColor(4, pixels.Color(100,0,0));
  
  pixels.setPixelColor(5, pixels.Color(0,0,100));
  pixels.setPixelColor(6, pixels.Color(50,0,50));
  pixels.setPixelColor(7, pixels.Color(100,0,0));
  pixels.setPixelColor(8, pixels.Color(100,100,100));
  pixels.setPixelColor(9, pixels.Color(100,0,0));
  pixels.show();
  delay(delayval);

  //6
  pixels.setPixelColor(0, pixels.Color(50,0,50));
  pixels.setPixelColor(1, pixels.Color(100,0,0));
  pixels.setPixelColor(2, pixels.Color(100,100,100));
  pixels.setPixelColor(3, pixels.Color(100,0,0));
  pixels.setPixelColor(4, pixels.Color(50,50,0));
  
  pixels.setPixelColor(5, pixels.Color(50,0,50));
  pixels.setPixelColor(6, pixels.Color(100,0,0));
  pixels.setPixelColor(7, pixels.Color(100,100,100));
  pixels.setPixelColor(8, pixels.Color(100,0,0));
  pixels.setPixelColor(9, pixels.Color(50,50,0));
  pixels.show();
  delay(delayval);

  //7
  pixels.setPixelColor(0, pixels.Color(100,0,0));
  pixels.setPixelColor(1, pixels.Color(100,100,100));
  pixels.setPixelColor(2, pixels.Color(100,0,0));
  pixels.setPixelColor(3, pixels.Color(50,50,0));
  pixels.setPixelColor(4, pixels.Color(0,100,0));

  pixels.setPixelColor(5, pixels.Color(100,0,0));
  pixels.setPixelColor(6, pixels.Color(100,100,100));
  pixels.setPixelColor(7, pixels.Color(100,0,0));
  pixels.setPixelColor(8, pixels.Color(50,50,0));
  pixels.setPixelColor(9, pixels.Color(0,100,0));
  pixels.show();
  delay(delayval);

  //8
  pixels.setPixelColor(0, pixels.Color(100,100,100));
  pixels.setPixelColor(1, pixels.Color(100,0,0));
  pixels.setPixelColor(2, pixels.Color(50,50,0));
  pixels.setPixelColor(3, pixels.Color(0,100,0));
  pixels.setPixelColor(4, pixels.Color(0,50,50));

  pixels.setPixelColor(5, pixels.Color(100,100,100));
  pixels.setPixelColor(6, pixels.Color(100,0,0));
  pixels.setPixelColor(7, pixels.Color(50,50,0));
  pixels.setPixelColor(8, pixels.Color(0,100,0));
  pixels.setPixelColor(9, pixels.Color(0,50,50));
  pixels.show();
  delay(delayval);

}
