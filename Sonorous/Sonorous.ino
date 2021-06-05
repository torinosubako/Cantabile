

/*
 * Project:Sonorous
 * CodeName:Preparation_stage_004
 * Build:2021/06/05
 * Author:torinosubako
 * Status:Impractical
*/


#include <M5StickCPlus.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SHT31.h>
#include "MHZ19.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"

//シリアル通信制御系
#define RX_PIN 33                     // GROVE端子 RX
#define TX_PIN 32                     // GROVE端子 TX
#define BAUDRATE 9600                 // デバイス<=>センサー間リンクスピード

#define BRIGHTNESS 7
#define S_PERIOD 48 

//センサーライブラリのコンストラクタ定義
Adafruit_SHT31 sht31;                // センサーライブラリのコンストラクタ定義
//Adafruit_BMP280 bmp;               // センサーライブラリのコンストラクタ定義
MHZ19 CO2Sens;                       // センサーライブラリのコンストラクタ定義

HardwareSerial mySerial(1);          // デバイス<=>センサー間リンク定義

// タイマー関係
unsigned long getDataTimer = 0;      // -廃止予定-
unsigned long Sleeptime = 60000;    // 2.5分150000DeepSleepする。

RTC_DATA_ATTR static uint8_t seq; // シーケンス番号


uint16_t temp;
uint16_t humid;
uint16_t press;
uint16_t co2;
uint16_t vbat;

uint32_t cpu_clock = 80;
//BLEデータセット既定
void setAdvData(BLEAdvertising *pAdvertising) { // アドバタイジングパケットを整形する
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    oAdvertisementData.setFlags(0x06); // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

    std::string strServiceData = "";
    strServiceData += (char)0x10;                   // 長さ(16バイト)
    strServiceData += (char)0xff;                   // AD Type 0xFF: Manufacturer specific data
    strServiceData += (char)0xff;                   // Test manufacture ID low byte<0>
    strServiceData += (char)0xff;                   // Test manufacture ID high byte<1>
    strServiceData += (char)0x00;                   // センサーノード固有ID(下位)<2>
    strServiceData += (char)0x00;                   // センサーノード固有ID(上位)<3>
    strServiceData += (char)seq;                    // シーケンス番号<4>
    strServiceData += (char)(temp & 0xff);          // 温度(下位)<5>
    strServiceData += (char)((temp >> 8) & 0xff);   // 温度(上位)<6>
    strServiceData += (char)(humid & 0xff);         // 湿度(下位)<7>
    strServiceData += (char)((humid >> 8) & 0xff);  // 湿度(上位)<8>
    strServiceData += (char)(press & 0xff);         // 気圧(下位)<9>
    strServiceData += (char)((press >> 8) & 0xff);  // 気圧(上位)<10>
    strServiceData += (char)(co2 & 0xff);           // CO2濃度(下位)<11>
    strServiceData += (char)((co2 >> 8) & 0xff);    // CO2濃度(上位)<12>
    strServiceData += (char)(vbat & 0xff);          // 電池電圧(下位)<13>
    strServiceData += (char)((vbat >> 8) & 0xff);   // 電池電圧(上位)<14>

    oAdvertisementData.addData(strServiceData);
    pAdvertising->setAdvertisementData(oAdvertisementData);
}



void setup() {
  M5.begin();
  bool setCpuFrequencyMhz(cpu_clock);
  //M5.Axp.ScreenBreath(BRIGHTNESS);
  Serial.begin(9600);
  Wire.begin(0,26);
  //M5.Lcd.setRotation(1);
  pinMode(M5_LED, OUTPUT);
  
  // デバイス<=>センサー間リンク開始
  mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);   
  CO2Sens.begin(mySerial);
  CO2Sens.autoCalibration(true);

  //データ収集
  //温度
  //temp = (uint16_t)(sht31.readTemperature() * 100);
  temp = (uint16_t)(CO2Sens.getTemperature(false, true)* 100);
  //湿度
  //humid = (uint16_t)(sht31.readHumidity(); * 100);
  humid = (uint16_t)(0);//ダミー
  //気圧
  //press = (uint16_t)(bme.readPressure() / 100 * 10);
  press = (uint16_t)(0);//ダミー
  //Co2データ
  co2 = (uint16_t)(CO2Sens.getCO2());
  //バッテリー電圧
  vbat = (uint16_t)(M5.Axp.GetVbatData() * 1.1 / 1000 * 100);
  

  // LCD情報表示
//  M5.Lcd.fillRect(0, 0, 240, 135, BLACK);
//  M5.Lcd.drawString("CO2", 10, 10, 4);
//  M5.Lcd.drawString("[ppm]", 10, 36, 4);
//  M5.Lcd.drawRightString(String(co2), 235, 12, 7);
//  M5.Lcd.drawString("Temp", 10, 77, 4);
//  M5.Lcd.drawString("[deg C]", 10, 103, 4);
//  M5.Lcd.drawRightString(String(temp / 100), 235, 77, 7);

  // BLEデータ送信プラットフォーム
  BLEDevice::init("Sonorous-0000");                           // 初期化
  BLEServer *pServer = BLEDevice::createServer();             // サーバー生成
  BLEAdvertising *pAdvertising = pServer->getAdvertising();   // オブジェクト取得
  setAdvData(pAdvertising);                                   // データ設定
  pAdvertising->start();                                      // アドバタイズ開始
  delay(10 * 1000);                                           // 10秒間データを流す
  pAdvertising->stop();                                       // アドバタイズ停止
  seq++;                                                      // シーケンス番号を更新

  // deepSleep！
  delay(10);
  esp_sleep_enable_timer_wakeup(1000000LL * S_PERIOD);              // Sleeptime秒Deep Sleepする
  esp_deep_sleep_start();
}
void loop() {

}
//0x02010610FFFFFF000006F00A000000004706A0010201060E09536F6E6F726F75732D30303030020A03051220004000000000000000000000000000000000
//これ以下のプログラムは実行されない！
//void loop() {
//  auto now = millis();
//  M5.update();
//  
//  //情報取得系
//  if (now - getDataTimer >= 10000) {
//    // 情報取得系
//    // MHZ19C   
//    int CO2 = CO2Sens.getCO2();
//    int8_t MZ_temp = CO2Sens.getTemperature(false, true);
//    //SHT30   
//    //float s_t = sht31.readTemperature();
//    //float s_h = sht31.readHumidity();
//    // BMP280
//    //float b_t = bmp.readTemperature();;
//    //float b_p = bmp.readPressure() / 100.0F;
//
//    //デバック
//    //Serial.print("CO2 (ppm): ");          // センサーデバック用
//    //Serial.print(CO2);                    // センサーデバック用
//    //Serial.print("MHZ19_Temp(C): ");    // センサーデバック用
//    //Serial.println(MZ_temp);            // センサーデバック用
//    //センサ公正できてない？(要検証)
//    //Serial.print("SHT30_Temp(C): ");    // センサーデバック用
//    //Serial.println(s_t);                // センサーデバック用
//    //Serial.print("BMP280_Temp(C): ");   // センサーデバック用
//    //Serial.println(b_t);                // センサーデバック用  
//
//    // 測定結果表示系
//    int height = M5.Lcd.height();
//    int width = M5.Lcd.width();
//    M5.Lcd.fillRect(0, 0, width, height, BLACK);
//    M5.Lcd.drawString("CO2", 10, 10, 4);
//    M5.Lcd.drawString("[ppm]", 10, 36, 4);
//    M5.Lcd.drawRightString(String(CO2), 235, 12, 7);
//    M5.Lcd.drawString("Temp", 10, 77, 4);
//    M5.Lcd.drawString("[deg C]", 10, 103, 4);
//    M5.Lcd.drawRightString(String(MZ_temp), 235, 77, 7);
//
//    // アラート判定論理?
//    if (CO2 >= 2500){
//      for (int i=0; i < 5; i++){
//        digitalWrite(M5_LED, LOW);
//        M5.Beep.tone(3300);
//        delay(150);
//        M5.Beep.end(); 
//        digitalWrite(M5_LED, HIGH);
//        delay(50);
//      }
//    }else if (CO2 >= 2000){
//      for (int i=0; i < 3; i++){
//        digitalWrite(M5_LED, LOW);
//        M5.Beep.tone(3300);
//        delay(300);
//        M5.Beep.end(); 
//        digitalWrite(M5_LED, HIGH);
//        delay(33);
//      }
//    }else if (CO2 >= 1500){
//      digitalWrite(M5_LED, LOW);
//      M5.Beep.tone(3300);
//      delay(1000);
//      M5.Beep.end(); 
//      digitalWrite(M5_LED, HIGH);
//    }else if (CO2 >= 1000){
//      digitalWrite(M5_LED, LOW);
//      M5.Beep.tone(2500);
//      delay(1000);
//      M5.Beep.end(); 
//      digitalWrite(M5_LED, HIGH);
//    }
//       
//    getDataTimer = now;
//  }
//}
