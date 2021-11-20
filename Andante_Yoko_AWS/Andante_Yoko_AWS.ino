
/*
   Project:Andante_Yoko_AWS
   CodeName:Preparation_stage_AX10
   Build:2021/11/20
   Author:torinosubako
   Status:Unverified
   Duties:Edge Processing Node
*/

#include <M5EPD.h>
#include "BLEDevice.h"
#include <Ambient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
WiFiClient client;
Ambient ambient;

/// Wi-Fi設定用基盤情報(2.4GHz帯域のみ)
const char *ssid = //Your Network SSID//;
const char *password = //Your Network Password//;

// Ambient連携用基盤情報
unsigned int channelId = //Your Ambient Channel ID//; // AmbientのチャネルID
const char* writeKey =   //Your Ambient Write key//; // ライトキー

// 東京公共交通オープンデータチャレンジ向け共通基盤情報
const String api_key = "&acl:consumerKey=test_key";//Your API Key//
const String base_url = "https://api-tokyochallenge.odpt.org/api/v4/odpt:TrainInformation?odpt:railway=odpt.Railway:";

// AWS IoT設定情報
const char* AWS_ENDPOINT = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.amazonaws.com";
const int   AWS_PORT     = 8883;
const char* PUB_TOPIC    = "test_core/fromDevice";
const char* SUB_TOPIC    = "test_core/fromCloud";
const char* CLIENT_ID    = "myM5StickCp";

const char* ROOT_CA = R"EOF(-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----)EOF";

const char* CERTIFICATE = R"KEY(-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----)KEY";

const char* PRIVATE_KEY = R"KEY(-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----)KEY";



// MQTT設定
#define QOS 1
WiFiClientSecure httpsClient;
PubSubClient mqttClient(httpsClient);

// LovyanGFX設定情報基盤
#define LGFX_M5PAPER
#include <LovyanGFX.hpp>

// デバイス制御
uint8_t seq;                      // RTC Memory シーケンス番号
#define MyManufacturerId 0xffff   // test manufacturer ID
uint32_t cpu_clock = 240;          // CPUクロック指定
M5EPD_Canvas canvas(&M5.EPD);
BLEScan* pBLEScan;
LGFX gfx;
LGFX_Sprite sp(&gfx);
uint16_t Node_ID;
int Node_IDs;
int Restart_token = 0;


// タイマー用データ
unsigned long getDataTimer = 0;
float Battery_voltage;

float Node00_StatusA[4];  // 気温・湿度・電圧・WBGT
int Node00_StatusB[2];    // 二酸化炭素濃度・気圧

String JY_Sta = "テストデータ";
// ODPT連接-JR
int JR_Line_num = 3;
String JR_Line_name[] = {"山手線", "埼京・川越線", "湘南新宿ライン"};
String JR_Line_key[] = {"JR-East.Yamanote", "JR-East.SaikyoKawagoe", "JR-East.ShonanShinjuku"};
String JR_Status[3];
// ODPT連接-地下鉄
int TRTA_Line_num = 3;
String TRTA_Line_name[] = {"地下鉄有楽町線", "地下鉄丸ノ内線", "地下鉄副都心線"};
String TRTA_Line_key[] = {"TokyoMetro.Yurakucho", "TokyoMetro.Marunouchi", "TokyoMetro.Fukutoshin"};
String TRTA_Status[3];
// ODPT連接-東武
int TOBU_Line_num = 1;
String TOBU_Line_name[] = {"東武東上本線"};
String TOBU_Line_key[] = {"Tobu.Tojo"};
String TOBU_Status[1];

int ODPT_X[] = {10, 235};
int ODPT_y[] = {10, 50, 90, 130, 170, 210, 250, 290, 330, 370, 410};
int JSN_y[] = {482, 508, 484};

void setup() {
  M5.begin();
  M5.RTC.begin();
  Serial.begin(115200);
  M5.BatteryADCBegin();
  bool setCpuFrequencyMhz(cpu_clock);
  Wireless_Access();
  RTC_time_sync();

  // LovyanGFX_EPD
  gfx.init();
  gfx.setRotation(1);
  gfx.setEpdMode(epd_mode_t::epd_text);

  // Wi-Fi・BLEセットアップ
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(false);


  
  // LovyanGFX_描画テスト
  train_rcv_joint();
  gfx.setFont(&lgfxJapanGothicP_32);
  gfx.fillScreen(TFT_BLACK);
  gfx.fillScreen(TFT_WHITE);
  gfx.fillScreen(TFT_BLACK);
  gfx.fillScreen(TFT_WHITE);
  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.startWrite();//描画待機モード
  EPD_Test();
  train_draw();
  alert_draw();
  jsn_draw();
  gfx.endWrite();//描画待機解除・描画実施
  
  
  //WiFi.disconnect(true);
  ambient.begin(channelId, writeKey, &client);
}

// 定例実施
void loop() {
  //BLEデータ受信
  BLE_RCV();
  M5.update();
  //180秒毎に定期実行
  auto now = millis();
  //M5.update();
  if (now - getDataTimer >= 180000) {
    getDataTimer = now;
    Battery_sta();
    // 無線接続開始
    //Wireless_Access();

    // ODPT関連関数
    train_rcv_joint();
    //描画関連
    gfx.startWrite();//描画待機モード
    gfx.fillScreen(TFT_BLACK);
    gfx.fillScreen(TFT_WHITE);
    gfx.fillScreen(TFT_BLACK);
    gfx.fillScreen(TFT_WHITE);
    train_draw();
    alert_draw();
    jsn_draw();
    jsn_upload();
    RTC_time_Get();
    gfx.endWrite();//描画待機解除・描画実施
    Serial.printf("Now_imprinting\r\n");
    

    // AWS関係
    setup_AWS_MQTT();
    connect_AWS();

    // WiFi切断
    mqttClient.disconnect();
    httpsClient.stop();
    //WiFi.disconnect(true);

    // デバッグ
    Serial.printf("Free heap after TLS %u\r\n", xPortGetFreeHeapSize());
    Serial.printf("Free heap(Minimum) after TLS %u\r\n", heap_caps_get_minimum_free_size(MALLOC_CAP_EXEC));
    Restart_token ++;
    if (Restart_token >= 11){
      Serial.println("ReStart(for_Refresh)..");
      ESP.restart();
    }
  }

  if (now - getDataTimer >= 360000) {
    Serial.println("ReStart(for_Timeout)..");
    ESP.restart();
  }

}



// メイン関数ここまで

// 統合センサネットワーク・情報取得関数
void BLE_RCV() {
  float new_temp, new_humid, WBGT, vbat;
  int  new_press, new_co2;
  bool found = false;
  //uint16_t Node_ID;
  BLEScanResults foundDevices = pBLEScan->start(3);
  int count = foundDevices.getCount();
  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice d = foundDevices.getDevice(i);
    if (d.haveManufacturerData()) {
      std::string data = d.getManufacturerData();
      int manu = data[1] << 8 | data[0];
      if (manu == MyManufacturerId && seq != data[4]) {
        found = true;
        Node_IDs = (int)(data[3] << 8 | data[2]);
        seq = data[4];
        new_temp = (float)(data[6] << 8 | data[5]) / 100.0;
        new_humid = (float)(data[8] << 8 | data[7]) / 100.0;
        new_press = (int)(data[10] << 8 | data[9]) * 10.0 / 100.0;
        new_co2 = (int)(data[12] << 8 | data[11]);
        vbat = (float)(data[14] << 8 | data[13]) / 100.0;

        // データチェック領域
        // SCD41とDPS310に最適化。それ以外のセンサーでは調整する事。
        if (Node00_StatusA[0] != new_temp && new_temp >= -40 && new_temp <= 120) {
          Node00_StatusA[0] = new_temp;
        }
        if (Node00_StatusA[1] != new_humid && new_humid >= 0 && new_humid <= 100) {
          Node00_StatusA[1] = new_humid;
        }
        if (Node00_StatusB[0] != new_co2 && new_co2 > 300 && new_co2 <= 4500) {
          Node00_StatusB[0] = new_co2;
        }
        if (Node00_StatusB[1] != new_press && new_press >= 300 && new_press <= 1100) {
          Node00_StatusB[1] = new_press;
        }
        Node00_StatusA[2] = vbat;
        WBGT = 0.725 * Node00_StatusA[0] + 0.0368 * Node00_StatusA[1] + 0.00364 * Node00_StatusA[0] * Node00_StatusA[1] - 3.246;
        Node00_StatusA[3] = WBGT;
        Serial.printf("Now_Recieved >>> Node: %.1d, seq: %d, t: %.1f, h: %.1f, p: %.1d, c: %.1d, w: %.1f, v: %.1f\r\n", Node_IDs, seq, new_temp, new_humid, new_press, new_co2, WBGT, vbat);
      }
    }
  }
}

// 無線制御関数
void Wireless_Access() {
  WiFi.disconnect(true);
  delay(1000);
  int wifi_cont = 0;
  int times = 5;
  WiFi.begin(ssid, password);
  delay(10 * 1000);
  while (WiFi.status() != WL_CONNECTED) {
    wifi_cont ++;
    delay(10 * 1000);
    Serial.println("Connecting to WiFi..");
    if (wifi_cont >= 5){
      Serial.println("ReStart(for_Wifi)..");
      ESP.restart();
    }
  }
  Serial.println(WiFi.localIP());
}

// AWSセットアップ
void setup_AWS_MQTT(){
  delay(1000);
  httpsClient.setCACert(ROOT_CA);
  httpsClient.setCertificate(CERTIFICATE);
  httpsClient.setPrivateKey(PRIVATE_KEY);
  mqttClient.setServer(AWS_ENDPOINT, AWS_PORT);
  Serial.println("Setup MQTT...");
}

void connect_AWS(){
  int retryCount = 0;
  while (!mqttClient.connect(CLIENT_ID)){
    Serial.println("Failed, state=" + String(mqttClient.state()));
    if (retryCount++ > 3){
      Serial.println("ReStart(for_AWS)..");
      ESP.restart();
      //return;
    }
    Serial.println("Try again in 10 sec");
    delay(10 * 1000);
  }
  Serial.println("Connected.");
  AWS_Upload();
}

// AWS-MQTTアップロード(Message生成含む)
void AWS_Upload() {
  if(Node00_StatusA[2]!=0.0 && Node00_StatusA[3]!=0.0){
    StaticJsonDocument<192> AWSdata;
    char json_string[192];
    AWSdata["Node_id"] = Node_IDs;
    //AWSdata["Node_id"] = 1;
    AWSdata["Seq_no"] = seq;
    AWSdata["Temp"] = Node00_StatusA[0];
    AWSdata["Humi"] = Node00_StatusA[1];
    AWSdata["WBGT"] = Node00_StatusA[3];
    AWSdata["CO2"] = Node00_StatusB[0];
    AWSdata["Press"] = Node00_StatusB[1];
    AWSdata["Node_Volt"] = Node00_StatusA[2];
    AWSdata["Core_Volt"] = Battery_voltage;
    serializeJson(AWSdata, json_string);
    mqttClient.publish(PUB_TOPIC, json_string);
    delay(10 * 1000);
    Serial.printf("AWS_imprinting\r\n");
  }
}

// ODPTデータセット実行関数
void train_rcv_joint() {
  JR_Status[0] = train_rcv_jr(JR_Line_key[0]);
  JR_Status[1] = train_rcv_jr(JR_Line_key[1]);
  JR_Status[2] = train_rcv_jr(JR_Line_key[2]);
  TRTA_Status[0] = train_rcv_trta(TRTA_Line_key[0]);
  TRTA_Status[1] = train_rcv_trta(TRTA_Line_key[1]);
  TRTA_Status[2] = train_rcv_trta(TRTA_Line_key[2]);
  TOBU_Status[0] = train_rcv_tobu(TOBU_Line_key[0]);
}

// ODPTデータ取得実行関数(JR)
String train_rcv_jr(String line_name) {
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
String train_rcv_trta(String line_name) {
  String result; //返答用変数作成

  //受信開始
  HTTPClient http;
  http.begin(base_url + line_name + api_key); //URLを指定
  //  http.begin(url); //URLを指定
  int httpCode = http.GET();  //GETリクエストを送信

  if (httpCode > 0) { //返答がある場合
    String payload = http.getString();  //返答（JSON形式）を取得

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

    //判定論理野
    if (point1 == "現在、平常どおり運転しています。") {
      //　平常運転
      result = "平常運転";
    } else if (point2 == "運行情報あり") {
      if (-1 != point1.indexOf("遅れがでています")) {
        // 遅れあり
        result = "列車遅延";
      } else if (-1 != point1.indexOf("直通運転を中止しています")) {
        //　直通運転中止
        result = "直通運転中止";
      } else if (-1 != point1.indexOf("で運転を見合わせています。")) {
        //　運転見合わせ
        result = "運転見合わせ";
      } else {
        //　情報有り
        result = "情報あり";
      }
    } else if (point2 == "ダイヤ乱れ") {
      //ダイヤ乱れ
      result = "ダイヤ乱れ";  
    } else if (point2 == "直通運転中止") {
      //直通運転中止
      result = "直通運転中止";  
    } else if (point2 == "一部列車遅延") {
      //一部列車遅延
      result = "一部列車遅延";
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
String train_rcv_tobu(String line_name) {
  String result; //返答用変数作成

  //受信開始
  HTTPClient http;
  http.begin(base_url + line_name + api_key); //URLを指定
  //  http.begin(url); //URLを指定
  int httpCode = http.GET();  //GETリクエストを送信

  if (httpCode > 0) { //返答がある場合
    String payload = http.getString();  //返答（JSON形式）を取得

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

    //判定論理野
    if (point1 == "平常どおり運転しています。") {
      //　平常運転
      result = "平常運転";
    } else if (point2 == "運行情報あり") {
      if (-1 != point1.indexOf("遅れがでています")) {
        // 遅れあり
        result = "列車遅延";
      } else if (-1 != point1.indexOf("直通運転を中止しています")) {
        //　直通運転中止
        result = "直通運転中止";
      } else if (-1 != point1.indexOf("で運転を見合わせています。")) {
        //　運転見合わせ
        result = "運転見合わせ";
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

// ODPTデータ取得実行関数(西武)(開発不能)
//String train_rcv_Seibu(String line_name){}

// ODPTデータ表示関数
void train_draw() {
  gfx.setFont(&lgfxJapanGothicP_32);
  gfx.drawString("鉄道各線の運行情報", ODPT_X[0], 10);
  gfx.drawString("<JR線>", ODPT_X[0], 50);
  gfx.drawString(JR_Line_name[0], ODPT_X[0], 90);
  gfx.drawString("：" + JR_Status[0], ODPT_X[1], 90);
  gfx.drawString(JR_Line_name[1], ODPT_X[0], 130);
  gfx.drawString("：" + JR_Status[1], ODPT_X[1], 130);
  gfx.drawString(JR_Line_name[2], ODPT_X[0], 170);
  gfx.drawString("：" + JR_Status[2], ODPT_X[1], 170);
  gfx.drawString("<地下鉄・私鉄各線>", ODPT_X[0], 210);
  gfx.drawString(TRTA_Line_name[0], ODPT_X[0], 250);
  gfx.drawString("：" + TRTA_Status[0], ODPT_X[1], 250);
  gfx.drawString(TRTA_Line_name[1], ODPT_X[0], 290);
  gfx.drawString("：" + TRTA_Status[1], ODPT_X[1], 290);
  gfx.drawString(TRTA_Line_name[2], ODPT_X[0], 330);
  gfx.drawString("：" + TRTA_Status[2], ODPT_X[1], 330);
  gfx.drawString(TOBU_Line_name[0], ODPT_X[0], 370);
  gfx.drawString("：" + TOBU_Status[0], ODPT_X[1], 370);
  gfx.drawString("都営三田線", ODPT_X[0], 410);
  gfx.drawString("：" + JY_Sta, ODPT_X[1], 410);


}

// アラート表示関数
void alert_draw() {
  gfx.setFont(&lgfxJapanGothicP_32);
  if (Node00_StatusB[0] >= 1050) {
    gfx.setTextColor(TFT_WHITE);
    gfx.fillRoundRect(490, 390, 225, 80, 10, gfx.color888(201, 17, 23));
    gfx.drawString("CO2濃度:警報", 500, 410);
  } else if (Node00_StatusB[0] >= 850) {
    gfx.setTextColor(TFT_BLACK);
    gfx.fillRoundRect(490, 390, 225, 80, 10, gfx.color888(250, 249, 200));
    gfx.drawRoundRect(490, 390, 225, 80, 10, gfx.color888(105, 105, 105));
    gfx.drawString("CO2濃度:注意", 500, 410);
  }

  if (Node00_StatusA[3] >= 28.0) {
    gfx.setTextColor(TFT_WHITE);
    gfx.fillRoundRect(725, 390, 225, 80, 10, gfx.color888(201, 17, 23));
    gfx.drawString("熱中症:危険", 755, 410);
  } else if (Node00_StatusA[3] >= 25.0) {
    gfx.setTextColor(TFT_WHITE);
    gfx.fillRoundRect(725, 390, 225, 80, 10, gfx.color888(255, 150, 0));
    gfx.drawString("熱中症:警戒", 755, 410);
  } else if (Node00_StatusA[3] >= 21.0) {
    gfx.setTextColor(TFT_BLACK);
    gfx.fillRoundRect(725, 390, 225, 80, 10, gfx.color888(250, 249, 200));
    gfx.drawRoundRect(725, 390, 225, 80, 10, gfx.color888(105, 105, 105));
    gfx.drawString("熱中症:注意", 755, 410);
  }
}

// バッテリー電圧取得
void Battery_sta() {
   Battery_voltage = ((float)M5.getBatteryVoltage()) / 1000.0;
   Serial.printf("Now >>> Edge v: %.1f\r\n", Battery_voltage);
}

// 統合センサネットワーク・情報表示関数
void jsn_draw() {
  gfx.setTextColor(TFT_BLACK);
  // 気温
  gfx.drawString("Temp", 10, JSN_y[0], 4);
  gfx.drawString("[deg C]", 10, JSN_y[1], 4);
  gfx.drawString(String(Node00_StatusA[0]), 100, JSN_y[2], 7);
  // 湿度
  gfx.drawString("Hum", 255, JSN_y[0], 4);
  gfx.drawString("[%]", 255, JSN_y[1], 4);
  gfx.drawString(String(Node00_StatusA[1]), 315, JSN_y[2], 7);
  // CO2
  gfx.drawString("CO2", 495, JSN_y[0], 4);
  gfx.drawString("[ppm]", 495, JSN_y[1], 4);
  gfx.drawString(String(Node00_StatusB[0]), 575, JSN_y[2], 7);
  // 気圧
  gfx.drawString("Prs", 730, JSN_y[0], 4);
  gfx.drawString("[hPa]", 730, JSN_y[1], 4);
  gfx.drawString(String(Node00_StatusB[1]), 805, JSN_y[2], 7);
}

void jsn_upload(){
  if(Node00_StatusA[2]!=0.0 && Node00_StatusA[3]!=0.0){
    ambient.set(1, Node00_StatusA[0]);
    ambient.set(2, Node00_StatusA[1]);
    ambient.set(3, Node00_StatusB[0]);
    ambient.set(4, Node00_StatusB[1]);
    ambient.set(5, Node00_StatusA[2]);
    ambient.set(6, Node00_StatusA[3]);
    ambient.set(7, Battery_voltage);
    ambient.send();
    delay(2000);
  }
}

// NTP->RTC 時刻取得
void RTC_time_sync() {
  configTime(3600L*9, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  struct tm timeInfo;                             // tmオブジェクトをtimeinfoとして生成
  if (getLocalTime(&timeInfo)) {                  // timeinfoに現在時刻を格納
    Serial.print("NTP_Server : ntp.nict.jp\n");

    // 時刻の取り出し
    rtc_time_t RTCtime;                   // 時刻格納用の構造体を生成
    RTCtime.hour = timeInfo.tm_hour;        // 時を格納
    RTCtime.min = timeInfo.tm_min;         // 分を格納
    RTCtime.sec = timeInfo.tm_sec;         // 秒を格納
    M5.RTC.setTime(&RTCtime);                  // 時刻の書き込み

    rtc_date_t RTCDate;                   // 日付格納用の構造体を生成
    RTCDate.mon = timeInfo.tm_mon + 1;       // 月（0-11）を格納※1を足す
    RTCDate.day = timeInfo.tm_mday;           // 日を格納
    RTCDate.year = timeInfo.tm_year + 1900;    // 年を格納（1900年からの経過年を取得するので1900を足す）
    M5.RTC.setDate(&RTCDate);                 // 日付を書き込み

    Serial.printf("RTC_Set : %d/%02d/%02d %02d:%02d:%02d\n",RTCDate.year, RTCDate.mon, RTCDate.day,RTCtime.hour, RTCtime.min, RTCtime.sec);

  }
  else {
    Serial.print("NTP Sync Error ");              // シリアルモニターに表示
  }
}

// RTC->EPD 時刻表示
void RTC_time_Get(){
  rtc_date_t DateStruct;
  rtc_time_t TimeStruct;
  M5.RTC.getDate(&DateStruct);
  M5.RTC.getTime(&TimeStruct);
  Serial.printf("RTC_Time : %d/%02d/%02d %02d:%02d:%02d\n",DateStruct.year, DateStruct.mon, DateStruct.day,TimeStruct.hour, TimeStruct.min, TimeStruct.sec);
  //gfx.drawString("更新時間:" + %d/%02d/%02d %02d:%02d:%02d\n", ODPT_X[0], 446);
}


// テストパターン
void EPD_Test() {
  gfx.drawString("M5Stack for M5Paper", 490, 10);
  gfx.drawString("LovyanGFX_Test", 490, 50);
  gfx.drawString("500", 500, 90);
  gfx.drawString("600", 600, 90);
  gfx.drawString("700", 700, 90);
  gfx.drawString("800", 800, 90);
  gfx.drawString("900", 900, 90);
  gfx.drawString("550", 550, 130);
  gfx.drawString("650", 650, 130);
  gfx.drawString("750", 750, 130);
  gfx.drawString("850", 850, 130);

  //    gfx.fillRoundRect(10, 390, 225, 80, 10, gfx.color888(201,17,23));
  //    gfx.fillRoundRect(245, 390, 225, 80, 10, gfx.color888(201,17,23));
}
