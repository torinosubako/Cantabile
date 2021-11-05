/*
   Project:AWS_TEST
   CodeName:Preparation_stage_001
   Build:2021/11/05
   Author:torinosubako
   Status:Impractical
*/

#include <M5StickCPlus.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Arduino_JSON.h>

#define DISP_X         5
#define DISP_TITLE_Y   5
#define DISP_STATUS_Y 35
#define DISP_TX_Y     65
#define DISP_RX_Y     95

// Wi-Fi設定用基盤情報(2.4GHz帯域のみ)
const char *ssid = //Your Network SSID//;
const char *password = //Your Network Password//;

const char* AWS_ENDPOINT = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.amazonaws.com";
const int   AWS_PORT     = 8883;
const char* PUB_TOPIC    = "test_core/fromDevice";
const char* SUB_TOPIC    = "test_core/fromCloud";
const char* CLIENT_ID    = "M5StickCPlus";

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

// M5StickCのAボタンを押すたびに加算した値をPublishする
int  pubValue = 0;
char pubMessage[128];

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

void setup_awsiot() {
  httpsClient.setCACert(ROOT_CA);
  httpsClient.setCertificate(CERTIFICATE);
  httpsClient.setPrivateKey(PRIVATE_KEY);
  mqttClient.setServer(AWS_ENDPOINT, AWS_PORT);
  mqttClient.setCallback(mqttCallback);
}

void connect_awsiot() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(CLIENT_ID)) {
      Serial.println("Connected.");
      mqttClient.subscribe(SUB_TOPIC, QOS);
      Serial.println("Subscribed.");
      M5.Lcd.setCursor(DISP_X, DISP_STATUS_Y);
      M5.Lcd.print("Connected to AWS.");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Try again in 5 seconds");
      // リトライ
      delay(5000);
    }
  }
}

void mqttCallback (char* topic, byte* payload, unsigned int length) {
  Serial.print("Received. topic=");
  Serial.println(topic);

  char subMessage[length];
  for (int i = 0; i < length; i++) {
    subMessage[i] = (char)payload[i];
  }
  Serial.println(subMessage);
  JSONVar obj = JSON.parse(subMessage);

  M5.Lcd.setCursor(DISP_X, DISP_RX_Y);
  M5.Lcd.printf("RX: %s", (const char*)obj["message"]);
}

void setup() {
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextFont(4);
  M5.Lcd.setCursor(DISP_X, DISP_TITLE_Y);
  M5.Lcd.println("AWS IoT MQTT Test");
  M5.Lcd.setCursor(DISP_X, DISP_TX_Y);
  M5.Lcd.print("TX:");
  M5.Lcd.setCursor(DISP_X, DISP_RX_Y);
  M5.Lcd.print("RX:");

  setup_wifi();
  setup_awsiot();
}

void loop() {
  // ボタン全体の状態更新
  M5.update();

  if (!mqttClient.connected()) {
    connect_awsiot();
  }

  // データの取得待ち
  mqttClient.loop();

  if (M5.BtnA.wasPressed()) {
    // Publishするメッセージの作成
    sprintf(pubMessage, "{\"message\": \"%d\"}", pubValue);
    Serial.print("Publishing message to topic ");
    Serial.println(PUB_TOPIC);
    Serial.println(pubMessage);
    mqttClient.publish(PUB_TOPIC, pubMessage);
    Serial.println("Published.");
    // ディスプレイへ送信内容を表示。併せて受信内容が既にあれば適当にクリア
    M5.Lcd.setCursor(DISP_X, DISP_TX_Y);
    M5.Lcd.printf("TX: %d", pubValue);
    M5.Lcd.setCursor(DISP_X, DISP_RX_Y);
    M5.Lcd.print("RX:                              ");

    pubValue++;
  }

  delay(50) ;
}
