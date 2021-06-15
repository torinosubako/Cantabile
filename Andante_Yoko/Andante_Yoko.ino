/*
 * Project:Andante_Yoko
 * CodeName:Preparation_stage_010
 * Build:2021/06/16
 * Author:torinosubako
 * Status:Impractical
*/

#include "BLEDevice.h"
#include <M5EPD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "ArduinoJson.h"
WiFiClient client;

//Wi-Fi設定用基盤情報(2.4GHz帯域のみ)
//const char *ssid = //Your Network SSID//;
//const char *password = //Your Network Password//;

//LovyanGFX設定情報基盤
#define LGFX_M5PAPER
#include <LovyanGFX.hpp>

//東京公共交通オープンデータチャレンジ向け共通基盤情報
const String api_key = "&acl:consumerKey=test_key";//Your API Key//
const String base_url = "https://api-tokyochallenge.odpt.org/api/v4/odpt:TrainInformation?odpt:railway=odpt.Railway:";

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
String JY_Status = "";
String JK_Status = "";
String JS_Status = "";
String JR_Line[] = {"JR-East.Yamanote","JR-East.SaikyoKawagoe","JR-East.ShonanShinjuku"};

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




// ODPTデータ取得実行関数(JR)
String train_rcv_jr(String line_name){
  String result; //返答用変数作成
  
  //受信開始
  HTTPClient http;
  http.begin(base_url + line_name + api_key); //URLを指定
  int httpCode = http.GET();  //GETリクエストを送信

  if (httpCode > 0) { //返答がある場合
    String payload = http.getString();  //返答（JSON形式）を取得
    //Serial.println(payload);

    //jsonオブジェクトの作成
    String json = payload;
    DynamicJsonDocument besedata(4096);
    deserializeJson(besedata, json);

    //データ識別・判定
    const char* deta1 = besedata[0]["odpt:trainInformationText"]["en"];
    const char* deta2 = besedata[0]["odpt:trainInformationStatus"]["en"];
    const char* deta3 = besedata[0];
    const char* deta4 = besedata[0]["odpt:trainInformationStatus"]["ja"];
    const String point1 = String(deta1).c_str();
    const String point2 = String(deta2).c_str();
    const String point4 = String(deta4).c_str();

    if (point1 == "Service on schedule") {
      //　平常運転
      result = "平常運転";
    } else if (point2 == "Notice") {
      //　情報有り
      result = "情報あり";
    } else if (point2 == "Delay") {
      //　遅れあり
      result = "列車遅延";
    } else if (point2 == "Operation suspended") {
      //　運転見合わせ
      result = "運転見合わせ";
    } else if (point4 == "運転見合わせ") {
      result = "運転見合わせ";
    } else if (point2 == "Direct operation cancellation") {
      //　直通運転中止
      result = "直通運転中止";
    } else if (point2 == NULL) {
      //取得時間外？
      result = "取扱時間外";
    } else {
      result = "取得エラー";
    }
  }
  else {
    Serial.println("Error on HTTP request");
    result = "通信エラー";
  }
  return result;
  http.end(); //リソースを解放
}

// ODPTデータ取得実行関数(東京メトロ)
// 旧名称:営団地下鉄の英略(Teito Rapid Transit Authority)のTRTAで呼び出し
String odpt_train_info_trta(String line_name) {
  String result; //返答用変数作成
  
  //受信開始
  HTTPClient http;
  http.begin(base_url + line_name + api_key); //URLを指定
//  http.begin(url); //URLを指定
  int httpCode = http.GET();  //GETリクエストを送信

  if (httpCode > 0) { //返答がある場合
    String payload = http.getString();  //返答（JSON形式）を取得
    //Serial.println(base_url + line_name + api_key);
    //Serial.println(httpCode);
    //Serial.println(payload);

    //jsonオブジェクトの作成
    String json = payload;
    DynamicJsonDocument besedata(4096);
    deserializeJson(besedata, json);

    //データ識別・判定(開発中)
    const char* deta1 = besedata[0]["odpt:trainInformationText"]["ja"];
    const char* deta2 = besedata[0]["odpt:trainInformationStatus"]["ja"];
    const char* deta3 = besedata[0];
    const String point1 = String(deta1).c_str();
    const String point2 = String(deta2).c_str();
    //Serial.println("データ受信結果");
    //Serial.println(deta1);
    //Serial.println(deta2);
    //Serial.println(deta3);

    //判定論理野（開発中）
    if (point1 == "平常どおり運転しています。") {
      //　平常運転
      result = "平常運転";
    } else if (point2 == "運行情報あり") {
      if (-1 != point1.indexOf("運転を見合わせています")) {
        //　運転見合わせ
        result = "運転見合わせ";
      } if (-1 != point1.indexOf("遅れがでています")) {
        // 遅れあり
        result = "列車遅延";
      } else if (-1 != point1.indexOf("直通運転を中止しています")) {
        //　直通運転中止
        result = "直通運転中止";
      } else {
        //　情報有り
        result = "情報あり";
      }
    } else if (point2 == NULL) {
      //取得時間外？
      result = "取扱時間外";
    } else {
      result = "取得エラー";
    }
  }
  else {
    Serial.println("Error on HTTP request");
    result = "通信エラー";
  }
  return result;
  http.end(); //リソースを解放
}

// ODPTデータ取得実行関数(東武)
String train_rcv_tobu(String line_name){
  String result; //返答用変数作成
  
  //受信開始
  HTTPClient http;
  http.begin(base_url + line_name + api_key); //URLを指定
//  http.begin(url); //URLを指定
  int httpCode = http.GET();  //GETリクエストを送信

  if (httpCode > 0) { //返答がある場合
    String payload = http.getString();  //返答（JSON形式）を取得
    //Serial.println(base_url + line_name + api_key);
    //Serial.println(httpCode);
    //Serial.println(payload);

    //jsonオブジェクトの作成
    String json = payload;
    DynamicJsonDocument besedata(4096);
    deserializeJson(besedata, json);

    //データ識別・判定
    const char* deta1 = besedata[0]["odpt:trainInformationText"]["ja"];
    const char* deta2 = besedata[0]["odpt:trainInformationStatus"]["ja"];
    const char* deta3 = besedata[0];
    const String point1 = String(deta1).c_str();
    const String point2 = String(deta2).c_str();
    //Serial.println("データ受信結果");
    //Serial.println(deta1);
    //Serial.println(deta2);
    //Serial.println(deta3);

    //判定論理野（開発中）
    if (point1 == "平常どおり運転しています。") {
      //　平常運転
      result = "平常運転";
    } else if (point2 == "運行情報あり") {
      if (-1 != point1.indexOf("運転を見合わせています")) {
        //　運転見合わせ
        result = "運転見合わせ";
      } if (-1 != point1.indexOf("遅れがでています")) {
        // 遅れあり
        result = "列車遅延";
      } else if (-1 != point1.indexOf("直通運転を中止しています")) {
        //　直通運転中止
        result = "直通運転中止";
      } else {
        //　情報有り
        result = "情報あり";
      }
    } else if (point2 == NULL) {
      //取得時間外？
      result = "取扱時間外";
    } else {
      result = "取得エラー";
    }
  }
  else {
    Serial.println("Error on HTTP request");
    result = "通信エラー";
  }
  return result;
  http.end(); //リソースを解放  
}

// ODPTデータ取得実行関数(西武)
//String train_rcv_Seibu(String line_name){}

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
