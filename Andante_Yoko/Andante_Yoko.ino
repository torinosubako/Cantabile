/*
 * Project:Andante_Yoko
 * CodeName:Preparation_stage_009
 * Build:2021/06/15
 * Author:torinosubako
 * Status:Impractical
*/

#include "BLEDevice.h"
#include <M5EPD.h>
#include <WiFi.h>
#include "ArduinoJson.h"
WiFiClient client;

//Wi-Fi設定用基盤情報(2.4GHz帯域のみ)
//const char *ssid = //Your Network SSID//;
//const char *password = //Your Network Password//;

//LovyanGFX改修準備
#define LGFX_M5PAPER
#include <LovyanGFX.hpp>

uint8_t seq;                      // RTC Memory シーケンス番号
#define MyManufacturerId 0xffff   // test manufacturer ID

M5EPD_Canvas canvas(&M5.EPD);
BLEScan* pBLEScan;
LGFX gfx;
LGFX_Sprite sp(&gfx);


// センサネットワーク受信用データ
unsigned long getDataTimer = 0;
float new_temp, new_humid, WBGT;
int  new_press;
float temp = 51.20;
float humid = 10.24;
int press = 8111;
int co2 = 8000;
int new_co2;
float vbat;
String JY_Sta = "テストデータ";

int ODPT_X[] = {10, 235};
int ODPT_y[] = {10, 50, 90, 130, 170, 210, 250, 290, 330, 370, 410};
int JSN_y[] = {482, 508, 484};

void setup() {
    M5.begin();
    // LovyanGFX_EPD
    gfx.init();
    gfx.setRotation(1);
    gfx.setEpdMode(epd_mode_t::epd_text);

    // LovyanGFX_描画テスト
    gfx.setFont(&lgfxJapanGothicP_32);
    gfx.fillScreen(TFT_WHITE);
    gfx.setTextColor(TFT_BLACK, TFT_WHITE); 
    gfx.startWrite();//描画待機モード
    EPD_Test();
    train_draw();
    alert_draw();
    jsn_draw();
    gfx.endWrite();//描画待機解除・描画実施
 
    // BLEセットアップ
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(false);

}

// 定例実施
void loop() {
  //BLEデータ受信
  BLE_RCV();
  M5.update();
  //120秒毎に定期実行
  auto now = millis();
  //M5.update();
  if (now - getDataTimer >= 120000) {
    getDataTimer = now;
    //refresh
    gfx.fillScreen(TFT_BLACK);
    gfx.fillScreen(TFT_WHITE);
    gfx.startWrite();//描画待機モード
    train_draw();
    alert_draw();
    jsn_draw();
    gfx.endWrite();//描画待機解除・描画実施
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
      uint16_t Node_ID = (int)(data[3] << 8 | data[2]);
      int manu = data[1] << 8 | data[0];
      if (manu == MyManufacturerId && seq != data[4]) {
        found = true;
        seq = data[4];
        new_temp = (float)(data[6] << 8 | data[5]) / 100.0;
        new_humid = (float)(data[8] << 8 | data[7]) / 100.0;
        new_press = (int)(data[10] << 8 | data[9]) * 10.0 / 100.0;
        new_co2 = (int)(data[12] << 8 | data[11]);
        vbat = (float)(data[14] << 8 | data[13]) / 100.0;
        
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
        WBGT = 0.725*temp + 0.0368*humid + 0.00364*temp*humid - 3.246;
        Serial.printf(">>>Node: %.1d, seq: %d, t: %.1f, h: %.1f, p: %.1d, c: %.1d, w: %.1f, v: %.1f\r\n", Node_ID, seq, new_temp, new_humid, new_press, new_co2, WBGT, vbat);
      }
    }
  }
}

// ODPTデータ取得関数


// ODPTデータ表示関数
void train_draw(){
    gfx.setFont(&lgfxJapanGothicP_32);
    gfx.drawString("鉄道各線の運行情報", ODPT_X[0],10);
    gfx.drawString("<JR線>", ODPT_X[0],50);
    gfx.drawString("山手線", ODPT_X[0],90);
    gfx.drawString("：" + JY_Sta, ODPT_X[1],90);
    gfx.drawString("埼京・川越線", ODPT_X[0],130);
    gfx.drawString("：" + JY_Sta, ODPT_X[1],130);
    gfx.drawString("湘南新宿ライン", ODPT_X[0],170);
    gfx.drawString("：" + JY_Sta, ODPT_X[1],170);
    gfx.drawString("<東京メトロ・私鉄各線>", ODPT_X[0],210);
    gfx.drawString("地下鉄有楽町線", ODPT_X[0],250);
    gfx.drawString("：" + JY_Sta, ODPT_X[1],250);
    gfx.drawString("地下鉄丸ノ内線", ODPT_X[0],290);
    gfx.drawString("：" + JY_Sta, ODPT_X[1],290);
    gfx.drawString("地下鉄副都心線", ODPT_X[0],330);
    gfx.drawString("：" + JY_Sta, ODPT_X[1],330);
    gfx.drawString("東武東上本線", ODPT_X[0],370);
    gfx.drawString("：" + JY_Sta, ODPT_X[1],370);
    gfx.drawString("西武池袋線", ODPT_X[0],410);
    gfx.drawString("：" + JY_Sta, ODPT_X[1],410);
  
}

// アラート表示関数
void alert_draw(){
  gfx.setFont(&lgfxJapanGothicP_32);
  if(co2 >= 1000){
    gfx.setTextColor(TFT_WHITE);
    gfx.fillRoundRect(490, 390, 225, 80, 10, gfx.color888(201,17,23));
    gfx.drawString("CO2濃度:警報",500,410);
  }else if (co2 >= 800){
    gfx.setTextColor(TFT_BLACK);  
    gfx.fillRoundRect(490, 390, 225, 80, 10, gfx.color888(250,249,200));
    gfx.drawRoundRect(490, 390, 225, 80, 10, gfx.color888(105,105,105));
    gfx.drawString("CO2濃度:注意",500,410);
  }
  if(WBGT >= 28.0){
    gfx.setTextColor(TFT_WHITE);
    gfx.fillRoundRect(725, 390, 225, 80, 10, gfx.color888(201,17,23));
    gfx.drawString("熱中症:危険",755,410);
  }else if (WBGT >= 25.0){
    gfx.setTextColor(TFT_WHITE); 
    gfx.fillRoundRect(725, 390, 225, 80, 10, gfx.color888(255,150,0));
    gfx.drawString("熱中症:警戒",755,410);
  }else if (WBGT >= 21.0){
    gfx.setTextColor(TFT_BLACK); 
    gfx.fillRoundRect(725, 390, 225, 80, 10, gfx.color888(250,249,200));
    gfx.drawRoundRect(725, 390, 225, 80, 10, gfx.color888(105,105,105));
    gfx.drawString("熱中症:注意",755,410);
  }
}


//統合センサネットワーク・情報表示関数
void jsn_draw(){
  gfx.setTextColor(TFT_BLACK); 
  // 気温
  gfx.drawString("Temp", 10, JSN_y[0], 4);
  gfx.drawString("[deg C]", 10, JSN_y[1], 4);
  gfx.drawString(String(temp), 100, JSN_y[2], 7);
  // 湿度
  gfx.drawString("Hum", 255, JSN_y[0], 4);
  gfx.drawString("[%]", 255, JSN_y[1], 4);
  gfx.drawString(String(humid), 315, JSN_y[2], 7);
  // CO2
  gfx.drawString("CO2", 495, JSN_y[0], 4);
  gfx.drawString("[ppm]", 495, JSN_y[1], 4);
  gfx.drawString(String(co2), 575, JSN_y[2], 7);
  // 気圧
  gfx.drawString("Prs", 730, JSN_y[0], 4);
  gfx.drawString("[hPa]", 730, JSN_y[1], 4);
  gfx.drawString(String(press), 805, JSN_y[2], 7);
}

// テストパターン
void EPD_Test(){
    gfx.drawString("M5Stack for M5Paper",490,10);
    gfx.drawString("LovyanGFX_Test",490,50);
    gfx.drawString("500",500,90);
    gfx.drawString("600",600,90);
    gfx.drawString("700",700,90);
    gfx.drawString("800",800,90);
    gfx.drawString("900",900,90);
    gfx.drawString("550",550,130);
    gfx.drawString("650",650,130);
    gfx.drawString("750",750,130);
    gfx.drawString("850",850,130);
    
    //    gfx.fillRoundRect(10, 390, 225, 80, 10, gfx.color888(201,17,23));
    //    gfx.fillRoundRect(245, 390, 225, 80, 10, gfx.color888(201,17,23));
}
