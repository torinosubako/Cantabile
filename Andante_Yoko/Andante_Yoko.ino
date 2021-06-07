/*
 * Project:Andante_Yoko
 * CodeName:Preparation_stage_006
 * Build:2021/06/07
 * Author:torinosubako
 * Status:Impractical
*/

#include "BLEDevice.h"
#include <M5EPD.h>


//LovyanGFX改修準備
//#define LGFX_M5PAPER
//#include <LovyanGFX.hpp>

uint8_t seq;                      // RTC Memory シーケンス番号
#define MyManufacturerId 0xffff   // test manufacturer ID

M5EPD_Canvas canvas(&M5.EPD);
BLEScan* pBLEScan;

unsigned long getDataTimer = 0;
float new_temp, new_humid;
int  new_press;
float temp = 10.24;
float humid = 10.24;
int press = 8111;
int co2 = 300;
int new_co2;
float vbat;
//
//int CO2 = 1024;
//float temp = 10.24;


int JSN_y[] = {482, 508, 484};

void setup() {
    M5.begin();
    //Serial.begin(9600);
    //Serial.print("CCS");

    //ディスプレイセットアップ
    M5.EPD.SetRotation(0);
    M5.EPD.Clear(true);
    M5.RTC.begin();
    canvas.createCanvas(960, 540);
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
  //120秒毎に定期実行
  auto now = millis();
  //M5.update();
  if (now - getDataTimer >= 120000) {
    getDataTimer = now;
    canvas.fillCanvas(0);
    //モニター部
    temp_draw();
    hum_draw();
    co2_draw();
    prs_draw();
    canvas.pushCanvas(0,0,UPDATE_MODE_GC16);
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
        new_press = (int)(data[10] << 8 | data[9]) * 10.0 / 100.0;
        new_co2 = (int)(data[12] << 8 | data[11]);
        vbat = (float)(data[14] << 8 | data[13]) / 100.0;
        Serial.printf(">>> seq: %d, t: %.1f, h: %.1f, p: %.1d, c: %.1d, v: %.1f\r\n", seq, new_temp, new_humid, new_press, new_co2, vbat);
        // SHT30とBMP280とMH-Z19Cに最適化。それ以外のセンサーでは調整する事。
        if(temp != new_temp && new_temp >= -40 && new_temp <= 120){
          temp = new_temp;
        }
        if(humid != new_humid && new_humid >= 10 && new_humid <= 90){
          humid = new_humid;
        }
        if(press != new_press && new_press >= 300 && new_press <= 1100){
          press = new_press;
        }
        if(co2 != new_co2 && new_co2 > 300 && new_co2 <= 5000){
          co2 = new_co2;
        }
      }
    }
  }
}


//情報表示部関数（鉄道？）



//統合センサネットワーク(JSN)・情報表示関数
void temp_draw(){
  canvas.drawString("Temp", 10, JSN_y[0], 4);
  canvas.drawString("[deg C]", 10, JSN_y[1], 4);
  canvas.drawRightString(String(temp), 250, JSN_y[2], 7);
}
void hum_draw(){
  canvas.drawString("Hum", 260, JSN_y[0], 4);
  canvas.drawString("[%]", 260, JSN_y[1], 4);
  canvas.drawRightString(String(humid), 510, JSN_y[2], 7);
}
void co2_draw(){
  canvas.drawString("CO2", 520, JSN_y[0], 4);
  canvas.drawString("[ppm]", 520, JSN_y[1], 4);
  canvas.drawRightString(String(co2), 730, JSN_y[2], 7);
}
void prs_draw(){
  canvas.drawString("Prs", 740, JSN_y[0], 4);
  canvas.drawString("[hPa]", 740, JSN_y[1], 4);
  canvas.drawRightString(String(press), 950, JSN_y[2], 7);
}
