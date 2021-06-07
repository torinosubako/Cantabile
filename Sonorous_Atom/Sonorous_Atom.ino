

/*
 * Project:Sonorous_Atom
 * CodeName:Preparation_stage_007
 * Build:2021/06/08
 * Author:torinosubako
 * Status:Impractical
*/

#include <M5Atom.h>
#include <Adafruit_NeoPixel.h>
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
#define TX_PIN 32                     // GROVE端子 TX
#define RX_PIN 26                     // GROVE端子 RX
#define BAUDRATE 9600                 // デバイス<=>センサー間リンクスピード

// デバイス関連の各種定義
uint16_t Node_ID = 0000;　// センサー固有ID
#define S_PERIOD 170　    // 間欠動作間隔指定
uint32_t cpu_clock = 80;　// CPUクロック指定

// nanopixel_LED制御
#define LEDPIN 27
#define NUMPIXELS 1
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

//センサー関連の各種定義
Adafruit_SHT31 sht31;                 // センサーライブラリのコンストラクタ定義
Adafruit_BMP280 bmp;                  // センサーライブラリのコンストラクタ定義
MHZ19 CO2Sens;                        // センサーライブラリのコンストラクタ定義
HardwareSerial mySerial(1);           // デバイス<=>センサー間リンク定義
RTC_DATA_ATTR static uint8_t seq;     // シーケンス番号

// 搬送用データ設定
uint16_t temp, humid, press, co2, vbat;

//BLEデータセット
void setAdvData(BLEAdvertising *pAdvertising) { // アドバタイジングパケットを整形する
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    oAdvertisementData.setFlags(0x06); // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

    std::string strServiceData = "";
    strServiceData += (char)0x10;                   // 長さ(16バイト)
    strServiceData += (char)0xff;                   // AD Type 0xFF: Manufacturer specific data
    strServiceData += (char)0xff;                   // Test manufacture ID low byte<0>
    strServiceData += (char)0xff;                   // Test manufacture ID high byte<1>
    strServiceData += (char)(Node_ID & 0xff);       // センサーノード固有ID(下位)<2>
    strServiceData += (char)((Node_ID >> 8) & 0xff);// センサーノード固有ID(上位)<3>
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
  pixels.begin();
  Serial.begin(9600);
  pinMode(19, INPUT_PULLUP); //SDAピンのプルアップの指定
  pinMode(22, INPUT_PULLUP); //SCLピンのプルアップの指定
  Wire.begin(19,22);
   // センサーの初期化
    while (!bmp.begin(0x76)) {
        Serial.println("BMP280 init fail");
    }
    while (!sht31.begin(0x44)) {
        Serial.println("SHT31 init fail");
    }
  pixels.setPixelColor(0, pixels.Color(0,90,0));
  pixels.show();
  
  // デバイス<=>センサー間リンク開始
  mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);   
  CO2Sens.begin(mySerial);
  CO2Sens.autoCalibration(true);

  //データ収集
  //温度
  temp = (uint16_t)(sht31.readTemperature() * 100);
  //temp = (uint16_t)(CO2Sens.getTemperature(false, true)* 100);
  //湿度
  humid = (uint16_t)(sht31.readHumidity() * 100);
  //humid = (uint16_t)(0);//ダミー
  //気圧
  press = (uint16_t)(bmp.readPressure() / 100 * 10);
  //press = (uint16_t)(0);//ダミー
  //Co2データ
  co2 = (uint16_t)(CO2Sens.getCO2());
  //電圧監視
  //vbat = (uint16_t)(M5.Axp.GetVinVoltage() * 1.1 / 1000 * 100);
  vbat = (uint16_t)(0);//ダミー
  //Serial.printf(">>> seq: %d, t: %.1d, h: %.1d, p: %.1d, c: %.1d, v: %.1d\r\n", seq, temp, humid, press, co2, vbat);
  
  // BLEデータ送信プラットフォーム
  pixels.setPixelColor(0, pixels.Color(0,0,90));
  pixels.show();
  BLEDevice::init("Sonorous-1000");                           // 初期化
  BLEServer *pServer = BLEDevice::createServer();             // サーバー生成
  BLEAdvertising *pAdvertising = pServer->getAdvertising();   // オブジェクト取得
  setAdvData(pAdvertising);                                   // データ設定
  pAdvertising->start();                                      // アドバタイズ開始
  delay(10 * 1000);                                           // 10秒間データを流す
  pAdvertising->stop();                                       // アドバタイズ停止
  seq++;                                                      // シーケンス番号を更新
  
  pixels.setPixelColor(0, pixels.Color(0,0,0));
  pixels.show();
//  delay(10);

  // deepSleep！
  pixels.clear();
  pixels.show();
  esp_bt_controller_disable();
  delay(10);
  esp_deep_sleep(1000000LL * S_PERIOD);
  esp_sleep_enable_timer_wakeup(1000000LL * S_PERIOD);              // S_PERIOD秒Deep Sleepする
  esp_deep_sleep_start();
}
void loop() {

}
