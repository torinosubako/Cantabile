
/*
   Project:Sensor_Test_SP
   Build:2021/09/25
   Author:torinosubako
   Status:Experimental
*/

#include <M5Core2.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_DPS310.h>
#include <Wire.h>
#include <Ambient.h>
#include <WiFi.h>
#include <HTTPClient.h>
WiFiClient client;
Ambient ambient;

// Wi-Fi設定用基盤情報(2.4GHz帯域のみ)
const char *ssid = //Your Network SSID//;
const char *password = //Your Network Password//;

// Ambient連携用基盤情報
unsigned int channelId = //Your Ambient Channel ID//; // AmbientのチャネルID
const char* writeKey = //Your Ambient Write key//; // ライトキー

// デバイス制御
//uint32_t cpu_clock = 80;
SensirionI2CScd4x scd4x;
Adafruit_DPS310 dps;

//細々とした奴
unsigned long getDataTimer = 0;
float Battery_voltage, USB_supply_voltage, SCD41_temperature, SCD41_humidity, DPS310_pressure, DPS310_temperature;
int SCD41_co2;


void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(100);
  }
  M5.begin();
  Wire.begin();

  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);
//  bool setCpuFrequencyMhz(cpu_clock);
  M5.Axp.ScreenBreath(7);

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement(); //定期測定の停止
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  error = scd4x.getSerialNumber(serial0, serial1, serial2);  //センサーID取得
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    printSerialNumber(serial0, serial1, serial2);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement(); //定期測定開始
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  // error = scd4x.measureSingleShot(co2, temperature, humidity);

  Serial.println("DPS310");
  if (! dps.begin_I2C()) {
    Serial.println("Failed to find DPS");
    while (1) yield();
  }
  Serial.println("DPS OK!");

  dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  dps.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);

  Serial.println("Waiting for first measurement... (5 sec)");

  Wireless_Access();
  WiFi.disconnect();
  ambient.begin(channelId, writeKey, &client);
  delay(5000);
}

void loop() {
  M5.update();
  //300秒毎に定期実行
  auto now = millis();
  if (now - getDataTimer >= 300000) {
    getDataTimer = now;
    Wireless_Access();
    Power_supply_sta();
    
    // SCD41系統
    uint16_t error;
    char errorMessage[256];
    // Read Measurement
    uint16_t co2, SCD41_co2;
    error = scd4x.readMeasurement(co2, SCD41_temperature, SCD41_humidity);
    if (error) {
      Serial.print("Error trying to execute readMeasurement(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
    } else if (co2 == 0) {
      Serial.println("Invalid sample detected, skipping.");
    } else {
      SCD41_co2 = co2;
      Serial.print("Co2:");
      Serial.print(co2);
      Serial.print("\t");
      Serial.print("SCD41_Temp:");
      Serial.print(SCD41_temperature);
      Serial.print("\t");
      Serial.print("SCD41_Humidity:");
      Serial.println(SCD41_humidity);
    }

    // DPS310系統
    sensors_event_t temp_event, pressure_event;
    while (!dps.temperatureAvailable() || !dps.pressureAvailable()) {
      return;
    }
    dps.getEvents(&temp_event, &pressure_event);
    Serial.print(F("Temperature = "));
    Serial.print(temp_event.temperature);
    Serial.println(" *C");
    Serial.print(F("Pressure = "));
    Serial.print(pressure_event.pressure);
    Serial.println(" hPa");

// 共通系統
  // 液晶表示系統
    M5.Lcd.begin();
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextFont(4);
    M5.Lcd.drawString("Sensor Test", 0, 1, 4);
    M5.Lcd.drawString("Co2[ppm]", 0, 27, 4);
    M5.Lcd.drawRightString(String(co2), 319, 27, 7);
    M5.Lcd.drawString("Temp[C]", 0, 80, 4);
    M5.Lcd.drawRightString(String(SCD41_temperature), 319, 80, 7);
    M5.Lcd.drawString("Hum[%]", 0, 133, 4);
    M5.Lcd.drawRightString(String(SCD41_humidity), 319, 133, 7);
    M5.Lcd.drawString("Prs[hPa]", 0, 186, 4);
    M5.Lcd.drawRightString(String(pressure_event.pressure), 319, 186, 7);

    // アップロード
    ambient.set(1, SCD41_co2);
    ambient.set(2, SCD41_temperature);
    ambient.set(3, SCD41_humidity);
    ambient.set(4, temp_event.temperature);
    ambient.set(5, pressure_event.pressure);
    ambient.set(6, USB_supply_voltage);
    ambient.set(7, Battery_voltage);
    ambient.send();
    delay(2000);

//    jsn_upload(SCD41_co2, SCD41_temperature, SCD41_humidity, pressure_event.pressure, temp_event.temperature, USB_supply_voltage, Battery_voltage);
    Serial.printf("Now_imprinting\r\n");
    //WiFi切断
    WiFi.disconnect();
  }
}

//// 無線制御関数
void Wireless_Access() {
  int wifi_cont = 0;
  int times = 5;
  WiFi.begin(ssid, password);
  delay(10 * 1000);
  while (WiFi.status() != WL_CONNECTED) {
    wifi_cont ++;
    delay(10 * 1000);
    Serial.println("Connecting to WiFi..");
    if (wifi_cont >= 5) {
      M5.shutdown(times);
    }
  }
  Serial.println(WiFi.localIP());
}

// バッテリー電圧取得(USB電源取得含む・Sonorous_Next時には改修のこと)
void Power_supply_sta() {
  Battery_voltage = M5.Axp.GetBatVoltage();
  USB_supply_voltage = M5.Axp.GetVBusVoltage();
  Serial.printf("Power_supply_sta >>> Edge v: %.1f\r USB v: %.1f\r\n", Battery_voltage, USB_supply_voltage);
}

// SCD4x情報取得関数
// Sonorous_Nextまでに関数化したい

////　アップロード関数
//void jsn_upload(SCD41_co2, SCD41_temperature, SCD41_humidity, DPS310_pressure, DPS310_temperature, USB_supply_voltage, Battery_voltage){
//  ambient.set(1, SCD41_co2);
//  ambient.set(2, SCD41_temperature);
//  ambient.set(3, SCD41_humidity);
//  ambient.set(4, DPS310_temperature);
//  ambient.set(5, DPS310_pressure);
//  ambient.set(6, USB_supply_voltage);
//  ambient.set(7, Battery_voltage);
//  ambient.send();
//  delay(2000);
//}
