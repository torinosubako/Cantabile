/*
 * Project:Andante_Yoko
 * CodeName:Preparation_stage_007
 * Build:2021/06/11
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
//#define LGFX_M5PAPER
//#include <LovyanGFX.hpp>

uint8_t seq;                      // RTC Memory シーケンス番号
#define MyManufacturerId 0xffff   // test manufacturer ID

M5EPD_Canvas canvas(&M5.EPD);
BLEScan* pBLEScan;


// センサネットワーク受信用データ
unsigned long getDataTimer = 0;
float new_temp, new_humid;
int  new_press;
float temp = 10.24;
float humid = 10.24;
int press = 8111;
int co2 = 300;
int new_co2;
float vbat;



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
    
    //train_draw();
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
        Serial.printf(">>>Node: %.1d, seq: %d, t: %.1f, h: %.1f, p: %.1d, c: %.1d, v: %.1f\r\n", Node_ID, seq, new_temp, new_humid, new_press, new_co2, vbat);
        
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


////情報表示部関数(東武鉄道)
////東武向け情報取得関数
//String odpt_train_info_tobu(String line_name) {
//  String result; //返答用変数作成
//  String file_header = "";
//  String file_address = "";
//
////  if (line_name == "Tobu.Tojo") {
////    file_header = "/img/TB_TJ/";
////  } else if (line_name == "Tobu.Ogose") {
////    file_header = "/img/TB_OG/";
////  } else if (line_name == "Tobu.TobuUrbanPark") {
////    file_header = "/img/TB_UP/";
////  } else if (line_name == "Tobu.TobuSkytree") {
////    file_header = "/img/TB_TS/";
////  } else if (line_name == "TobuSkytreeBranch") { //東武スカイツリーライン(押上-曳舟)
////    file_header = "/img/TB_TSB/";
////  } else if (line_name == "Tobu.Kameido") {
////    file_header = "/img/TB_TSK/";
////  } else if (line_name == "Tobu.Daishi") {
////    file_header = "/img/TB_TSD/";    
////  } else if (line_name == "Tobu.Nikko") {
////    file_header = "/img/TB_TN/";
////  } else if (line_name == "Tobu.Kinugawa") {
////    file_header = "/img/TB_TNK/";
////  } else if (line_name == "Tobu.Utsunomiya") {
////    file_header = "/img/TB_TNU/";
////  } else if (line_name == "Tobu.Isesaki") {
////    file_header = "/img/TB_TI/";
////  } else if (line_name == "Tobu.Sano") {
////    file_header = "/img/TB_TIS/";
////  } else if (line_name == "Tobu.Kiryu") {
////    file_header = "/img/TB_TII/";
////  } else if (line_name == "Tobu.Koizumi") {
////    file_header = "/img/TB_TIO/";
////  } else if (line_name == "Tobu.KoizumiBranch") { //東武小泉線(東小泉-太田)
////    file_header = "/img/TB_TIO/";
////  } else {
////    file_header = "/img/system/403.jpg";
////    return file_header;
////  }
//  //受信開始
//  HTTPClient http;
//  http.begin(base_url + line_name + api_key); //URLを指定
////  http.begin(url); //URLを指定
//  int httpCode = http.GET();  //GETリクエストを送信
//
//  if (httpCode > 0) { //返答がある場合
//    String payload = http.getString();  //返答（JSON形式）を取得
//    //Serial.println(base_url + line_name + api_key);
//    //Serial.println(httpCode);
//    //Serial.println(payload);
//
//    //jsonオブジェクトの作成
//    String json = payload;
//    DynamicJsonDocument besedata(4096);
//    deserializeJson(besedata, json);
//
//    //データ識別・判定
//    const char* deta1 = besedata[0]["odpt:trainInformationText"]["ja"];
//    const char* deta2 = besedata[0]["odpt:trainInformationStatus"]["ja"];
//    const char* deta3 = besedata[0];
//    const String point1 = String(deta1).c_str();
//    const String point2 = String(deta2).c_str();
//    //Serial.println("データ受信結果");
//    //Serial.println(deta1);
//    //Serial.println(deta2);
//    //Serial.println(deta3);
//
//    //判定論理野（開発中）
//    if (point1 == "平常どおり運転しています。") {
//      //　平常運転
//      file_address = file_header + "01.jpg";
//    } else if (point2 == "運行情報あり") {
//      if (-1 != point1.indexOf("運転を見合わせています")) {
//        //　運転見合わせ
//        file_address = file_header + "04.jpg";
//      } if (-1 != point1.indexOf("遅れがでています")) {
//        // 遅れあり
//        file_address = file_header + "03.jpg";
//      } else if (-1 != point1.indexOf("直通運転を中止しています")) {
//        //　直通運転中止
//        file_address = file_header + "05.jpg";
//      } else {
//        //　情報有り
//        file_address = file_header + "02.jpg";
//      }
//    } else if (point2 == NULL) {
//      //取得時間外？
//      file_address = file_header + "00.jpg";
//    } else {
//      file_address = "/img/system/403.jpg";
//    }
//  }
//  else {
//    Serial.println("Error on HTTP request");
//    file_address = "/img/system/404.jpg";
//  }
//  result = file_address;
//  return result;
//  http.end(); //リソースを解放
//}

void train_draw(){
  
}


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
