/*
   Project:AWS_TEST2
   CodeName:Preparation_stage_002
   Build:2021/11/06
   Author:torinosubako
   Status:Impractical
*/

#include <M5StickCPlus.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Wi-Fi設定用基盤情報(2.4GHz帯域のみ)
const char *ssid = //Your Network SSID//;
const char *password = //Your Network Password//;

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

// デバイス制御
RTC_DATA_ATTR static uint8_t seq;                     // RTC Memory シーケンス番号
uint16_t Node_ID = 0000;
unsigned long getDataTimer = 0;

// Wi-Fi制御
void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // ESP32でWiFiに繋がらなくなるときのための対策
  WiFi.disconnect(true);
  delay(1000);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// AWSセットアップ
void setup_AWS_MQTT(){
  httpsClient.setCACert(ROOT_CA);
  httpsClient.setCertificate(CERTIFICATE);
  httpsClient.setPrivateKey(PRIVATE_KEY);
  mqttClient.setServer(AWS_ENDPOINT, AWS_PORT);

  Serial.println("Connecting MQTT...");
  int retryCount = 0;
  while (!mqttClient.connect(CLIENT_ID)){
    Serial.println("Failed, state=" + String(mqttClient.state()));
    if (retryCount++ > 3)
      ESP.restart();
    Serial.println("Try again in 5 seconds");
    delay(5000);
  }
  Serial.println("Connected.");
}

// AWS-MQTTアップロード(Message生成含む)
void AWS_Upload(){
  StaticJsonDocument<192> data;
  char json_string[192];
  data["Node_id"] = Node_ID;
  data["Seq_no"] = seq;
  data["Temp"] = 25.68;
  data["Humi"] = 33.4;
  data["WBGT"] = 11.4;
  data["CO2"] = 514;
  data["Press"] = 1024;
  data["Node_Volt"] = (float)(M5.Axp.GetVBusVoltage());
  data["Core_Volt"] = (float)(M5.Axp.GetVBusVoltage());
  serializeJson(data, json_string);
  mqttClient.publish(PUB_TOPIC, json_string);
}

void setup() {
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextFont(7);
  setup_wifi();
    
  // AWS関係関数
  setup_AWS_MQTT();
  AWS_Upload();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.printf("%d", seq);

  WiFi.disconnect();
}

void loop() {
  M5.update();

  //120秒毎に定期実行
  auto now = millis();
  M5.update();
  if (now - getDataTimer >= 120000) {
    getDataTimer = now;
    seq++;
    setup_wifi();
    
    // AWS関係関数
    setup_AWS_MQTT();
    AWS_Upload();

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(5, 5);
    M5.Lcd.printf("%d", seq);

    Serial.printf("Now_imprinting\r\n");
    //WiFi切断
    WiFi.disconnect();
    
  }
}
