/*
 * Project:Andante
 * CodeName:Preparation_stage_003
 * Build:2021/06/06
 * Author:torinosubako
 * Status:Impractical
*/

#include <M5EPD.h>
#include "BLEDevice.h"

//LovyanGFX改修準備
//#define LGFX_M5PAPER
//#include <LovyanGFX.hpp>

uint8_t seq;                      // RTC Memory シーケンス番号
#define MyManufacturerId 0xffff   // test manufacturer ID

M5EPD_Canvas canvas(&M5.EPD);
BLEScan* pBLEScan;

unsigned long getDataTimer = 0;
float temps, new_temp;
float humid, new_humid;
float press, new_press;
int co2, new_co2;
float vbat;
//
int CO2 = 1024;
float temp = 10.24;

void setup() {
    M5.begin();
    Serial.begin(115200);
    Serial.print("CCS");

    //ディスプレイセットアップ
    M5.EPD.SetRotation(90);
    M5.EPD.Clear(true);
    M5.RTC.begin();
    canvas.createCanvas(540, 960);
    canvas.setTextFont(4);
    canvas.println(("M5Stack for M5Paper"));
    canvas.println(("e-Paper Touch display Test"));
    canvas.pushCanvas(0,0,UPDATE_MODE_GC16);
 
    // BLEセットアップ
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(false);

}

void loop() {
  //BLEデータ受信
  BLE_RCV();
  M5.update();
  //60秒毎に定期実行(ここから下が動かない？？？)
  auto now = millis();
  //M5.update();
  if (now - getDataTimer >= 60000) {
    getDataTimer = now;
    canvas.fillCanvas(0);
    //モニター部
    temp_draw();
    hum_draw();
    co2_draw();
    prs_draw();
    canvas.pushCanvas(0,0,UPDATE_MODE_GC16);

    //ダミーデータ制御用
    CO2 += 1;
    temp += 0.01;
  }
}

// 統合センサネットワーク・情報取得関数
void BLE_RCV(){
  bool found = false;
  BLEScanResults foundDevices = pBLEScan->start(3);
  int count = foundDevices.getCount();
  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice d = foundDevices.getDevice(i);
    if (d.haveManufacturerData()) {
      std::string data = d.getManufacturerData();
      int manu = data[1] << 8 | data[0];
      if (manu == MyManufacturerId && seq != data[4]) {
        found = true;
        seq = data[4];
        new_temp = (float)(data[6] << 8 | data[5]) / 100.0;
        new_humid = (float)(data[8] << 8 | data[7]) / 100.0;
        new_press = (float)(data[10] << 8 | data[9]) * 10.0 / 100.0;
        new_co2 = (int)(data[12] << 8 | data[11]);
        vbat = (float)(data[14] << 8 | data[13]) / 100.0;
        Serial.printf(">>> seq: %d, t: %.1f, h: %.1f, p: %.1f, c: %.1d, v: %.1f\r\n", seq, new_temp, new_humid, new_press, new_co2, vbat);
      }
    }
  }
}


//情報表示部関数（鉄道？）



//統合センサネットワーク・情報表示関数
void temp_draw(){
  canvas.drawString("Temp", 10, 831, 4);
  canvas.drawString("[deg C]", 10, 858, 4);
  canvas.drawRightString(String(temp), 260, 833, 7);
}
void hum_draw(){
  canvas.drawString("Hum", 270, 831, 4);
  canvas.drawString("[%]", 270, 858, 4);
  canvas.drawRightString(String(temp), 530, 833, 7);
}
void co2_draw(){
  canvas.drawString("CO2", 10, 892, 4);
  canvas.drawString("[ppm]", 10, 918, 4);
  canvas.drawRightString(String(CO2), 260, 894, 7);
}
void prs_draw(){
  canvas.drawString("Prs", 270, 892, 4);
  canvas.drawString("[hPa]", 270, 918, 4);
  canvas.drawRightString(String(CO2), 530, 894, 7);
}
