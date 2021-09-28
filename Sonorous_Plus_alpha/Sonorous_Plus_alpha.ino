

/*
   Project:Sonorous_Plus_alpha
   CodeName:Preparation_stage_024EPX
   Build:2021/09/28
   Author:torinosubako
   Status:Impractical
*/


#include <M5StickCPlus.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_DPS310.h>
#include <Wire.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"

// デバイス関連の各種定義
uint16_t Node_ID = 0001; // センサー固有ID
#define S_PERIOD 270     // 間欠動作間隔指定
uint32_t cpu_clock = 80; // CPUクロック指定
RTC_DATA_ATTR static uint8_t seq;     // シーケンス番号

// センサー関連の各種定義
SensirionI2CScd4x scd4x;
Adafruit_DPS310 dps;

// 搬送・引出用データ設定
uint16_t temp, humid, press, co2, vbat;
float SCD_temp, SCD_humid, DPS310_press, DPS310_temp;

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
  // 基幹起動テスト
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  M5.begin();
  Wire.begin();
  bool setCpuFrequencyMhz(cpu_clock);
  M5.Axp.SetLDO2(false);
  Serial.println("Sonorous_Startup:Ready");

  uint16_t error;
  char errorMessage[256];

  // センサーの初期化
  // SCD41系統
  Serial.println("SCD41_Test_start");
  scd4x.begin(Wire);
  error = scd4x.stopPeriodicMeasurement(); //定期測定の停止
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  error = scd4x.startLowPowerPeriodicMeasurement(); //低消費電力モード
  if (error) {
    Serial.print("Error trying to execute startLowPowerPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  delay(5000);
  //測定開始
  error = scd4x.measureSingleShot();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  Serial.println("SCD41 OK!");

  // DPS310系統
  Serial.println("DPS310_Test_start");
  if (! dps.begin_I2C()) {
    Serial.println("Failed to find DPS");
    while (1) yield();
  }
  dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  dps.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);
  Serial.println("DPS310 OK!");


  Serial.println("All_Sensor_Status:All Green");
  
  //挙動確認用
  pinMode(M5_LED, OUTPUT);
  digitalWrite(M5_LED, LOW);

  // デバイス<=>センサー間リンク
  // DPS310系統
  sensors_event_t temp_event, pressure_event;
    while (!dps.temperatureAvailable() || !dps.pressureAvailable()) {
      return;
    }
    dps.getEvents(&temp_event, &pressure_event);
    Serial.print("DPS_Temp:");
    Serial.print(temp_event.temperature);
    Serial.print("*C");
    Serial.print("\t");
    Serial.print("DPS_Press:");
    Serial.print(pressure_event.pressure);
    Serial.print("hPa");
    Serial.print("\n");
  
  // SCD41系統
  error = scd4x.readMeasurement(co2, SCD_temp, SCD_humid);
  if (error) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else if (co2 == 0) {
    Serial.println("Invalid sample detected, skipping.");
  } else {
    //SCD41_co2 = co2;
    Serial.print("SCD_Co2:");
    Serial.print(co2);
    Serial.print("rpm");
    Serial.print("\t");
    Serial.print("SCD_Temp:");
    Serial.print(SCD_temp);
    Serial.print("*C");
    Serial.print("\t");
    Serial.print("SCD_Hum:");
    Serial.print(SCD_humid);
    Serial.print("pph");
  }
  Serial.print("\n");
  Serial.println("All_Sensor_Status:Recieved");

  // データ収集
  // 温度
  temp = (uint16_t)(SCD_temp * 100);
  // 湿度
  humid = (uint16_t)(SCD_humid * 100);
  // 気圧
  press = (uint16_t)(pressure_event.pressure * 10);
  // press = (uint16_t)(8111);
  // Co2データ(自動生成)
  // 電圧監視(外部電源系-電圧監視(AXP192))
  vbat = (uint16_t)(M5.Axp.GetVBusVoltage() * 100);

  // BLEデータ送信プラットフォーム()
  digitalWrite(M5_LED, HIGH);
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
  delay(10);
  esp_deep_sleep(1000000LL * S_PERIOD);
  esp_sleep_enable_timer_wakeup(1000000LL * S_PERIOD);        // S_PERIOD秒Deep Sleepする
  esp_deep_sleep_start();
}
void loop() {

}
