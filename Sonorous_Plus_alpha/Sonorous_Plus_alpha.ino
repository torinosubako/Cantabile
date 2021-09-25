

/*
 * Project:Sonorous_Plus_alpha
 * CodeName:Preparation_stage_020EPX
 * Build:2021/09/25
 * Author:torinosubako
 * Status:Impractical
*/


#include <M5StickCPlus.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"

//I2C通信制御系

// デバイス関連の各種定義
uint16_t Node_ID = 0001; // センサー固有ID
#define S_PERIOD 160     // 間欠動作間隔指定
uint32_t cpu_clock = 80; // CPUクロック指定                                                                                                                                                                                                                                 

//センサー関連の各種定義
SensirionI2CScd4x scd4x;
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
  M5.Axp.SetLDO2(false);
  Serial.begin(9600);
  Serial.println("test");
  pinMode(0, INPUT_PULLUP);   //SDAピンのプルアップの指定
  pinMode(26, INPUT_PULLUP);  //SCLピンのプルアップの指定
  Wire.begin(0,26);

  // 電流スイッチングコア(SuperWingSで統合試験未了)
//  gpio_pulldown_dis(GPIO_NUM_36); // pin25を用いる為にpin36をfloating化
//  gpio_pullup_dis(GPIO_NUM_36);   // pin25を用いる為にpin36をfloating化
//  pinMode(GPIO_NUM_25, OUTPUT);
//  digitalWrite(GPIO_NUM_25, HIGH);

//  // センサーの初期化
  while (!bmp.begin(0x76)) {
    Serial.println("BMP280 init fail");
  }while (!sht31.begin(0x44)) {
    Serial.println("SHT31 init fail");
  }
  
  // プレヒートタイム(60秒以上/きっちり60秒だと数値が安定しない？(要検討))
//  pinMode(M5_LED, OUTPUT);
//  digitalWrite(M5_LED, HIGH);
//  delay(65 * 1000); 
 
  // デバイス<=>センサー間リンク開始
  mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);   
  CO2Sens.begin(mySerial);
  CO2Sens.autoCalibration(false); //自動キャリブレーション無効化

  // データ収集
  // 温度
  temp = (uint16_t)(sht31.readTemperature() * 100);
  // 湿度
  humid = (uint16_t)(sht31.readHumidity() * 100);
  // 気圧
  press = (uint16_t)(bmp.readPressure() / 100 * 10);
  // Co2データ
  co2 = (uint16_t)(CO2Sens.getCO2());
  // 電圧監視(外部電源系-電圧監視(AXP192))
  vbat = (uint16_t)(M5.Axp.GetVBusVoltage() * 100);
  
  // BLEデータ送信プラットフォーム()
  BLEDevice::init("Sonorous-0001");                           // 初期化
  BLEServer *pServer = BLEDevice::createServer();             // サーバー生成
  BLEAdvertising *pAdvertising = pServer->getAdvertising();   // オブジェクト取得
  setAdvData(pAdvertising);                                   // データ設定
  pAdvertising->start();                                      // アドバタイズ開始
  delay(10 * 1000);                                           // 10秒間データを流す
  pAdvertising->stop();                                       // アドバタイズ停止
  seq++;                                                      // シーケンス番号を更新
  
  // deepSleep！
  esp_bt_controller_disable();
  // digitalWrite(GPIO_NUM_25, LOW);                             // CO2センサモジュールへの給電を遮断
  delay(10);
  esp_deep_sleep(1000000LL * S_PERIOD);
  esp_sleep_enable_timer_wakeup(1000000LL * S_PERIOD);        // S_PERIOD秒Deep Sleepする
  esp_deep_sleep_start();
}
void loop() {

}
