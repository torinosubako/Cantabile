 
/*
 * Project:SCD41x_Test
 * Build:2021/09/25
 * Author:torinosubako
 * Status:Impractical
*/

#include <M5Core2.h>
#include <Dps310.h>
#include <Wire.h>

SensirionI2CScd4x scd4x;

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
  Dps310PressureSensor.begin(Wire);

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

  Serial.println("Waiting for first measurement... (5 sec)");

//  M5.Lcd.drawString("Co2[ppm]", 0, 53, 4);
//  M5.Lcd.drawRightString("0000", 319, 53, 7);
//  M5.Lcd.drawString("Temp[C]", 0, 106, 4);
//  M5.Lcd.drawRightString("00.00", 319, 106, 7);
//  M5.Lcd.drawString("Hum[%]", 0, 159, 4);
//  M5.Lcd.drawRightString("00.00", 319, 159, 7);
  delay(5000);
}

void loop() {
    uint16_t error;
    char errorMessage[256];
    
    // Read Measurement
    uint16_t co2;
    float SCD41_temperature;
    float SCD41_humidity;
    error = scd4x.readMeasurement(co2, SCD41_temperature, SCD41_humidity);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else {
        Serial.print("Co2:");
        Serial.print(co2);
        Serial.print("\t");
        Serial.print("SCD41_Temperature:");
        Serial.print(temperature);
        Serial.print("\t");
        Serial.print("SCD41_Humidity:");
        Serial.println(humidity);
    }
    
    M5.Lcd.begin();
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextFont(4);
    M5.Lcd.drawString("Sensor Test", 0, 1, 4);
    M5.Lcd.drawString("Co2[ppm]", 0, 27, 4);
    M5.Lcd.drawRightString(String(co2), 319, 27, 7);
    M5.Lcd.drawString("Temp[C]", 0, 80, 4);
    M5.Lcd.drawRightString(String(temperature), 319, 80, 7);
    M5.Lcd.drawString("Hum[%]", 0, 133, 4);
    M5.Lcd.drawRightString(String(humidity), 319, 133, 7);

    delay(10000);
}
