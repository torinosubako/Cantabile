
/*
   Project:Andante_Yoko_AWS_Kai
   CodeName:Preparation_stage_AX16_s23
   Build:2021/12/19
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
#include <Preferences.h>
Preferences preferences;
WiFiClient client;
Ambient ambient;

// Wi-Fi設定用基盤情報(2.4GHz帯域のみ)
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
//Your ROOT CA//
-----END CERTIFICATE-----)EOF";

const char* CERTIFICATE = R"KEY(-----BEGIN CERTIFICATE-----
//Your CERTIFICATE//
-----END CERTIFICATE-----)KEY";

const char* PRIVATE_KEY = R"KEY(-----BEGIN RSA PRIVATE KEY-----
//Your PRIVATE_KEY//
-----END RSA PRIVATE KEY-----)KEY";




// MQTT設定
#define QOS 0
WiFiClientSecure httpsClient;
PubSubClient mqttClient(httpsClient);


// LovyanGFX設定情報基盤
#define LGFX_M5PAPER
#include <LovyanGFX.hpp>


// 内部処理用データ
uint8_t seq;                      // NVS連接無
float common_temp;
float common_humid;
float common_WBGT;
float common_vbat = 0.0;          // NVS連接無
int common_press;
int common_co2;
//int Resend_Tag;
uint16_t Node_ID;                 // NVS連接無
int Node_IDs;                     // NVS連接無
float Battery_voltage;            // NVS連接無
int receive_counter;              // NVS連接無


// デバイス制御
#define MyManufacturerId 0xffff   // test manufacturer ID
//uint32_t cpu_clock = 240;         // CPUクロック指定
M5EPD_Canvas canvas(&M5.EPD);
BLEScan* pBLEScan;
LGFX gfx;
LGFX_Sprite sp(&gfx);


// タイマー用データ
unsigned long getDataTimer = 0;
hw_timer_t * timer0 = NULL;
hw_timer_t * timer1 = NULL;
hw_timer_t * timer2 = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
// AWS接続制御用タイムアップ(30秒)
void IRAM_ATTR onTimer0() {
  portENTER_CRITICAL_ISR(&timerMux);    // 割り込みタスクセット
  mqttClient.disconnect();
  WiFi.disconnect(true);
  pBLEScan->clearResults();
  pBLEScan->stop();
  preferences.putFloat("hold_temp", common_temp);
  preferences.putFloat("hold_humid", common_humid);
  preferences.putShort("hold_co2", common_co2);
  preferences.putShort("hold_press", common_press);
  preferences.putFloat("hold_WBGT", common_WBGT);
  // preferences.putShort("resend_tags", 1);
  preferences.end();
  // Serial.printf("Free heap(Minimum) after TLS %u\r\n", heap_caps_get_minimum_free_size(MALLOC_CAP_EXEC));
  Serial.println("ReStart(for_Refresh(timer0))..");
  timerEnd(timer0);
  timerEnd(timer2);
  portEXIT_CRITICAL_ISR(&timerMux);   // 割り込みタスク解放
  delay(10000);
  ESP.restart();
}
// ODPTデータセット取得時タイムアップ(60秒)
void IRAM_ATTR onTimer1() {
  portENTER_CRITICAL_ISR(&timerMux);    // 割り込みタスクセット
  preferences.end();
  WiFi.disconnect(true);
  // Serial.printf("Free heap(Minimum) after TLS %u\r\n", heap_caps_get_minimum_free_size(MALLOC_CAP_EXEC));
  Serial.println("ReStart(for_Refresh(timer1))..");
  delay(10000);
  timerEnd(timer1);
  timerEnd(timer2);
  portEXIT_CRITICAL_ISR(&timerMux);   // 割り込みタスク解放
  delay(10000);
  ESP.restart();
}
// 全体処理タイムアップ(8分)
void IRAM_ATTR onTimer2() {
  portENTER_CRITICAL_ISR(&timerMux);    // 割り込みタスクセット
  WiFi.disconnect(true);
  pBLEScan->clearResults();
  pBLEScan->stop();
  preferences.putFloat("hold_temp", common_temp);
  preferences.putFloat("hold_humid", common_humid);
  preferences.putShort("hold_co2", common_co2);
  preferences.putShort("hold_press", common_press);
  preferences.putFloat("hold_WBGT", common_WBGT);
  // preferences.putShort("resend_tags", 1);
  preferences.end();
  // Serial.printf("Free heap(Minimum) after TLS %u\r\n", heap_caps_get_minimum_free_size(MALLOC_CAP_EXEC));
  Serial.println("ReStart(for_Timeout)..");
  timerEnd(timer2);
  portEXIT_CRITICAL_ISR(&timerMux);   // 割り込みタスク解放
  delay(10000);
  ESP.restart();
}


// ODPT関連一式
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

  // 基幹設定
  preferences.begin("hold_data", false);
  M5.BatteryADCBegin();
  //bool setCpuFrequencyMhz(cpu_clock);
  // 全体タイマー
  timer2 = timerBegin(2, 80, true);
  timerAttachInterrupt(timer2, &onTimer2, true);
  timerAlarmWrite(timer2, 480000000, false);
  timerAlarmEnable(timer2);
  // 外部接続系統
  Wireless_Access();
  RTC_time_sync();

  // NVS内のデータからcommonデータ群へキャスト
  common_temp = preferences.getFloat("hold_temp", 0);
  common_humid = preferences.getFloat("hold_humid", 0);
  common_WBGT = preferences.getFloat("hold_WBGT", 0);
  common_press = preferences.getShort("hold_press", 0);
  common_co2 = preferences.getShort("hold_co2", 0);
  //Resend_Tag = preferences.getShort("resend_tags", 0);
  preferences.remove("resend_tags");
  // Serial.printf("NVS_Readed!\r\n"); // デバッグ用

  // LovyanGFX_EPDセットアップ
  gfx.init();
  gfx.setRotation(1);
  gfx.setEpdMode(epd_mode_t::epd_text);
  // Serial.printf("LovyanGFX_EPD_Set!\r\n"); // デバッグ用

  // ODPTデータセット取得
  timer1 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer1, &onTimer1, true);
  timerAlarmWrite(timer1, 60000000, false);
  timerAlarmEnable(timer1);
  train_rcv_joint();
  timerEnd(timer1);
  // Serial.printf("ODPT_Data_Got!\r\n"); // デバッグ用

  // LovyanGFX_描画
  gfx.setFont(&lgfxJapanGothicP_32);
  gfx.fillScreen(TFT_BLACK);
  gfx.fillScreen(TFT_WHITE);
  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.startWrite(); // 描画待機モード
  train_draw();
  alert_draw();
  jsn_draw();
  gfx.endWrite(); // 描画待機解除・描画実施
  // Serial.printf("EPD_imprinting!\r\n"); // デバッグ用

  // AWSデータ再送モード(開発中)
  /*
  if(Resend_Tag == 1 && common_WBGT != 0.0){
    Serial.printf("AWS_Resend!\r\n"); // デバッグ用
    // AWS関係セットアップ
    setup_AWS_MQTT();
    // AWSタイマー
    timer0 = timerBegin(0, 80, true);
    timerAttachInterrupt(timer0, &onTimer0, true);
    timerAlarmWrite(timer0, 30000000, false);
    timerAlarmEnable(timer0);
    connect_AWS();
    timerEnd(timer0);
    AWS_Upload();
    // WiFi切断
    mqttClient.disconnect();
  }
  */
  
  // NVS領域解放
  preferences.clear();
  // Serial.printf("NVS_Cleared!\r\n"); // デバッグ用

  // BLE・Ambientセットアップ
  ambient.begin(channelId, writeKey, &client);
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(false);
}


// 定例実施
void loop() {
  //BLEデータ受信
  BLE_RCV();
  M5.update();
  //300秒毎に定期実行
  auto now = millis();
  if (now - getDataTimer >= 300000 && common_vbat != 0.0) {
    getDataTimer = now;
    RTC_time_Get();
    preferences.putFloat("hold_temp", common_temp);
    preferences.putFloat("hold_humid", common_humid);
    preferences.putShort("hold_co2", common_co2);
    preferences.putShort("hold_press", common_press);
    preferences.putFloat("hold_WBGT", common_WBGT);
    preferences.end();
    // Serial.printf("Free heap(Minimum) after TLS %u\r\n", heap_caps_get_minimum_free_size(MALLOC_CAP_EXEC)); // デバッグ用
    Serial.println("ReStart(for_Refresh)..");
    timerEnd(timer2);
    delay(10000);
    ESP.restart();
  }
}


// メイン関数ここまで
// 統合センサネットワーク・情報取得関数
void BLE_RCV() {
  float new_temp, new_humid, WBGT, vbat;
  int  new_press, new_co2;
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
        Node_IDs = (int)(data[3] << 8 | data[2]);
        seq = data[4];
        new_temp = (float)(data[6] << 8 | data[5]) / 100.0;
        new_humid = (float)(data[8] << 8 | data[7]) / 100.0;
        new_press = (int)(data[10] << 8 | data[9]) * 10.0 / 100.0;
        new_co2 = (int)(data[12] << 8 | data[11]);
        vbat = (float)(data[14] << 8 | data[13]) / 100.0;

        // データチェック領域
        // SCD41とDPS310に最適化。それ以外のセンサーでは調整する事。
        if (common_temp != new_temp && new_temp >= -40 && new_temp <= 120) {
          common_temp = new_temp;
        }
        if (common_humid != new_humid && new_humid >= 0 && new_humid <= 100) {
          common_humid = new_humid;
        }
        if (common_co2 != new_co2 && new_co2 > 300 && new_co2 <= 4500) {
          common_co2 = new_co2;
        }
        if (common_press != new_press && new_press >= 300 && new_press <= 1100) {
          common_press = new_press;
        }
        common_vbat = vbat;
        WBGT = 0.725 * common_temp + 0.0368 * common_humid + 0.00364 * common_temp * common_humid - 3.246;
        common_WBGT = WBGT;
        // デバッグ用
        Serial.printf("Now_Recieved >>> Node: %d, seq: %d, t: %.1f, h: %.1f, p: %.1d, c: %.1d, w: %.1f, v: %.1f\r\n", Node_IDs, seq, new_temp, new_humid, new_press, new_co2, WBGT, vbat);
        // 送信処理
        Wireless_Access_Check();
        main_communicator();
      }
    }
  }
}

// メインコミュニケータ
void main_communicator() {
  
  Battery_sta();

  // 通信制御関数
  jsn_upload();
  RTC_time_Get();
  
  // AWS関係セットアップ
  setup_AWS_MQTT();
  // AWSタイマー
  timer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer0, &onTimer0, true);
  timerAlarmWrite(timer0, 30000000, false);
  timerAlarmEnable(timer0);
  connect_AWS();
  timerEnd(timer0);
  //portENTER_CRITICAL_ISR(&timerMux);    // 割り込みタスクセット
  AWS_Upload();
  //portEXIT_CRITICAL_ISR(&timerMux);   // 割り込みタスク解放

  // WiFi切断
  mqttClient.disconnect();
  WiFi.disconnect(true);

  // BLEリセット
  pBLEScan->clearResults();

  // デバッグ
  //Serial.printf("Free heap after TLS %u\r\n", xPortGetFreeHeapSize());
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

   // Serial.println("Connecting to WiFi..");
    if (wifi_cont >= 3){
      // Serial.println("ReStart(for_Wifi)..");
      timerEnd(timer2);
      //ESP.restart();
    }
  }
  // デバッグ用
  // Serial.println(WiFi.localIP());
}

// 接続確認関数
void Wireless_Access_Check() {
  int wifi_cont = 0;
  while (WiFi.status() != WL_CONNECTED) {
    wifi_cont ++;
    WiFi.begin(ssid, password);
    delay(10 * 1000);
    // Serial.println("Connecting to WiFi(AC)..");
    if (wifi_cont >= 3){
      preferences.putShort("resend_tags", 1);
      hold_data_upload();
      WiFi.disconnect(true);
      // Serial.println("ReStart(for_Wifi(AC))..");
      timerEnd(timer2);
      delay(1000);
      ESP.restart();
    }
  }
}

// AWSセットアップ
void setup_AWS_MQTT(){
  delay(1000);
  httpsClient.setCACert(ROOT_CA);
  httpsClient.setCertificate(CERTIFICATE);
  httpsClient.setPrivateKey(PRIVATE_KEY);
  mqttClient.setServer(AWS_ENDPOINT, AWS_PORT);
  //mqttClient.setCallback(callback);
  // デバッグ用
  // Serial.println("Setup MQTT...");
}

// AWS接続制御
void connect_AWS(){
  int retryCount = 0;
  while (!mqttClient.connect(CLIENT_ID)){
    // Serial.println("Failed, state=" + String(mqttClient.state()));
    if (retryCount++ > 2){
      preferences.putShort("resend_tags", 1);
      hold_data_upload();
      timerEnd(timer2);
      delay(1000);
      // Serial.println("ReStart(for_AWS)..");
      ESP.restart();
    }
    // Serial.println("Try again in 5 sec");
    delay(5 * 1000);
  }
  // デバッグ用
  // Serial.println("Connected...");
}

// AWS-MQTTアップロード(Message生成含む)
void AWS_Upload() {
  if(common_vbat!=0.0 && common_WBGT!=0.0){
    StaticJsonDocument<192> AWSdata;
    char json_string[192];
    AWSdata["Node_id"] = Node_IDs;
    //AWSdata["Node_id"] = 1;
    AWSdata["Seq_no"] = seq;
    AWSdata["Temp"] = common_temp;
    AWSdata["Humi"] = common_humid;
    AWSdata["WBGT"] = common_WBGT;
    AWSdata["CO2"] = common_co2;
    AWSdata["Press"] = common_press;
    AWSdata["Node_Volt"] = common_vbat;
    AWSdata["Core_Volt"] = Battery_voltage;
    serializeJson(AWSdata, json_string);
    mqttClient.publish(PUB_TOPIC, json_string);
    delay(10 * 1000);
    // デバッグ用
    // Serial.printf("AWS_imprinting\r\n");
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
    //// Serial.println(payload);

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
    // Serial.println("Error on HTTP request");
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
    // Serial.println("Error on HTTP request");
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
    // Serial.println("Error on HTTP request");
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
  if (common_co2 >= 1050) {
    gfx.setTextColor(TFT_WHITE);
    gfx.fillRoundRect(490, 390, 225, 80, 10, gfx.color888(201, 17, 23));
    gfx.drawString("CO2濃度:警報", 500, 410);
  } else if (common_co2 >= 850) {
    gfx.setTextColor(TFT_BLACK);
    gfx.fillRoundRect(490, 390, 225, 80, 10, gfx.color888(250, 249, 200));
    gfx.drawRoundRect(490, 390, 225, 80, 10, gfx.color888(105, 105, 105));
    gfx.drawString("CO2濃度:注意", 500, 410);
  }

  if (common_WBGT >= 28.0) {
    gfx.setTextColor(TFT_WHITE);
    gfx.fillRoundRect(725, 390, 225, 80, 10, gfx.color888(201, 17, 23));
    gfx.drawString("熱中症:危険", 755, 410);
  } else if (common_WBGT >= 25.0) {
    gfx.setTextColor(TFT_WHITE);
    gfx.fillRoundRect(725, 390, 225, 80, 10, gfx.color888(255, 150, 0));
    gfx.drawString("熱中症:警戒", 755, 410);
  } else if (common_WBGT >= 21.0) {
    gfx.setTextColor(TFT_BLACK);
    gfx.fillRoundRect(725, 390, 225, 80, 10, gfx.color888(250, 249, 200));
    gfx.drawRoundRect(725, 390, 225, 80, 10, gfx.color888(105, 105, 105));
    gfx.drawString("熱中症:注意", 755, 410);
  }
}

// バッテリー電圧取得
void Battery_sta() {
   Battery_voltage = ((float)M5.getBatteryVoltage()) / 1000.0;
   // デバッグ用
   // Serial.printf("Now >>> Edge v: %.1f\r\n", Battery_voltage);
}

// 統合センサネットワーク・情報表示関数
void jsn_draw() {
  gfx.setTextColor(TFT_BLACK);
  // 気温
  gfx.drawString("Temp", 10, JSN_y[0], 4);
  gfx.drawString("[deg C]", 10, JSN_y[1], 4);
  gfx.drawString(String(common_temp), 100, JSN_y[2], 7);
  // 湿度
  gfx.drawString("Hum", 255, JSN_y[0], 4);
  gfx.drawString("[%]", 255, JSN_y[1], 4);
  gfx.drawString(String(common_humid), 315, JSN_y[2], 7);
  // CO2
  gfx.drawString("CO2", 495, JSN_y[0], 4);
  gfx.drawString("[ppm]", 495, JSN_y[1], 4);
  gfx.drawString(String(common_co2), 575, JSN_y[2], 7);
  // 気圧
  gfx.drawString("Prs", 730, JSN_y[0], 4);
  gfx.drawString("[hPa]", 730, JSN_y[1], 4);
  gfx.drawString(String(common_press), 805, JSN_y[2], 7);
}

// 情報集約関数
void jsn_upload(){
  if(common_vbat!=0.0 && common_WBGT!=0.0){
    ambient.set(1, common_temp);
    ambient.set(2, common_humid);
    ambient.set(3, common_co2);
    ambient.set(4, common_press);
    ambient.set(5, common_vbat);
    ambient.set(6, common_WBGT);
    ambient.set(7, Battery_voltage);
    ambient.send();
    delay(1000);
  }
}

// commonデータ群からNVS内のデータへキャスト
void hold_data_upload(){
  preferences.putFloat("hold_temp", common_temp);
  preferences.putFloat("hold_humid", common_humid);
  preferences.putShort("hold_co2", common_co2);
  preferences.putShort("hold_press", common_press);
  preferences.putFloat("hold_WBGT", common_WBGT);
  preferences.end();
  // Serial.printf("Free heap(Minimum) after TLS %u\r\n", heap_caps_get_minimum_free_size(MALLOC_CAP_EXEC));
}

// NTP->RTC 時刻取得
void RTC_time_sync() {
  configTime(3600L*9, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  struct tm timeInfo;                             // tmオブジェクトをtimeinfoとして生成
  if (getLocalTime(&timeInfo)) {                  // timeinfoに現在時刻を格納
    // Serial.print("NTP_Server : ntp.nict.jp\n");

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

    // Serial.printf("RTC_Set : %d/%02d/%02d %02d:%02d:%02d\n",RTCDate.year, RTCDate.mon, RTCDate.day,RTCtime.hour, RTCtime.min, RTCtime.sec);

  }
  else {
    // Serial.print("NTP Sync Error\n");              // シリアルモニターに表示
  }
}

// RTC->EPD 時刻表示
void RTC_time_Get(){
  rtc_date_t DateStruct;
  rtc_time_t TimeStruct;
  M5.RTC.getDate(&DateStruct);
  M5.RTC.getTime(&TimeStruct);
  // Serial.printf("RTC_Time : %d/%02d/%02d %02d:%02d:%02d\n",DateStruct.year, DateStruct.mon, DateStruct.day,TimeStruct.hour, TimeStruct.min, TimeStruct.sec);
  // gfx.drawString("更新時間:" + %d/%02d/%02d %02d:%02d:%02d\n", ODPT_X[0], 446);
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
