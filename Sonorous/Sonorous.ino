 
/*
 * Project:Sonorous
 * CodeName:Preparation_stage001
 * Build:2021/05/27
 * Author:torinosubako
 * Status:Impractical
*/


#include <M5StickCPlus.h>
#include "MHZ19.h"

//シリアル通信制御系
#define RX_PIN 33                     // GROVE端子 RX
#define TX_PIN 32                     // GROVE端子 TX
#define BAUDRATE 9600                 // デバイス<=>センサー間リンクスピード

#define BRIGHTNESS 8
MHZ19 CO2Sens;                                             // センサーライブラリのコンストラクタ定義
HardwareSerial mySerial(1);                                // デバイス<=>センサー間リンク定義

unsigned long nextLedOperationTime = 0;
unsigned long getDataTimer = 0;

int history[160] = {};
int historyPos = 0;
//int history_CO2[160] = {};
//int historyPos_CO2 = 0;
int history_Tenp[160] = {};
int historyPos_Tenp = 0;

void setup() {
  M5.begin();
  M5.Axp.ScreenBreath(BRIGHTNESS);
  Serial.begin(9600);
  M5.Lcd.setRotation(1);
  pinMode(M5_LED, OUTPUT);
  
  // デバイス<=>センサー間リンク開始
  mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);   
  CO2Sens.begin(mySerial);
  CO2Sens.autoCalibration(true);
}

void loop() {
  auto now = millis();
  M5.update();
  
  //情報取得系
  if (now - getDataTimer >= 30000) {
    // 情報取得系
    int CO2 = CO2Sens.getCO2();
    int8_t temp = CO2Sens.getTemperature(false, true);
    //Serial.print("CO2 (ppm): ");          // センサーデバック用
    //Serial.print(CO2);                    // センサーデバック用
    //Serial.print(", Temperature (C): ");  // センサーデバック用
    //Serial.println(temp);                 // センサーデバック用

    // 測定結果表示系
    int height = M5.Lcd.height();
    int width = M5.Lcd.width();
    M5.Lcd.fillRect(0, 0, width, height, BLACK);
    M5.Lcd.drawString("CO2", 10, 10, 4);
    M5.Lcd.drawString("[ppm]", 10, 36, 4);
    M5.Lcd.drawRightString(String(CO2), 235, 12, 7);
    M5.Lcd.drawString("Temp", 10, 77, 4);
    M5.Lcd.drawString("[deg C]", 10, 103, 4);
    M5.Lcd.drawRightString(String(temp), 235, 77, 7);

    // アラート判定論理?
    if (CO2 >= 2500){
      for (int i=0; i < 5; i++){
        digitalWrite(M5_LED, LOW);
        M5.Beep.tone(3300);
        delay(150);
        M5.Beep.end(); 
        digitalWrite(M5_LED, HIGH);
        delay(50);
      }
    }else if (CO2 >= 2000){
      for (int i=0; i < 3; i++){
        digitalWrite(M5_LED, LOW);
        M5.Beep.tone(3300);
        delay(300);
        M5.Beep.end(); 
        digitalWrite(M5_LED, HIGH);
        delay(33);
      }
    }else if (CO2 >= 1500){
      digitalWrite(M5_LED, LOW);
      M5.Beep.tone(3300);
      delay(1000);
      M5.Beep.end(); 
      digitalWrite(M5_LED, HIGH);
    }else if (CO2 >= 1000){
      digitalWrite(M5_LED, LOW);
      M5.Beep.tone(2500);
      delay(1000);
      M5.Beep.end(); 
      digitalWrite(M5_LED, HIGH);
    }
       
    getDataTimer = now;
  }
}
