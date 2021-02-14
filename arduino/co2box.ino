#include <Arduino.h>
#include <U8g2lib.h>
#include <SoftwareSerial.h>
#include "DHT.h"
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// CO2 sensor
const int CO2_SENSOR_RX_PIN = 3;
const int CO2_SENSOR_TX_PIN = 2;
const int CALIBRATION_BUTTON_PIN = 12;

byte bufferCO2[9];
int co2ppm;
char co2ppmAsChar[5];

const byte SetResolutionTo2000PPM[] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x07, 0xD0, 0x8F};
const byte SetResolutionTo10000PPM[] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x27, 0x10, 0x2F};
const byte selfCalibrationZeroPointON[] = {0xFF, 0x01, 0x79, 0xA0, 0x00, 0x00, 0x00, 0x00, 0xE6};
const byte selfCalibrationZeroPointOFF[] = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};
const byte startZeroPointCalibration[] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};
const byte readCO2[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

SoftwareSerial CO2Sensor(CO2_SENSOR_RX_PIN, CO2_SENSOR_TX_PIN);

// DHT sensor
const int DHT_PIN = 5;
const int DHT_TYPE = DHT22;

DHT dht(DHT_PIN, DHT_TYPE);

float temperature;
float humidity;
char temperatureAsChar[5];
char humidityAsChar[5];

// Semaphore
const int SEMAPHORE_RED_PIN = 8;
const int SEMAPHORE_ORANGE_PIN = 9;
const int SEMAPHORE_GREEN_PIN = 11;

const int CO2_LIMIT_ORANGE = 600;
const int CO2_LIMIT_RED = 800;

// OLED
boolean toggleOLEDMarker = false;

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void draw(int co2ppm, float temperature, float humidity) {
  dtostrf(co2ppm, 4, 0, co2ppmAsChar);
  dtostrf(temperature, 2, 1, temperatureAsChar);
  dtostrf(humidity, 2, 1, humidityAsChar);

  if (true == toggleOLEDMarker) {
    u8g2.drawDisc( 120, 3, 3);
  }

  u8g2.setFont(u8g_font_profont15);
  u8g2.drawStr( 95, 22, "CO");
  u8g2.setFont(u8g_font_profont10);
  u8g2.drawStr( 110, 25, "2");

  u8g2.setFont(u8g_font_profont29);
  u8g2.drawStr( 15, 22, co2ppmAsChar);
  u8g2.setFont(u8g_font_profont10);
  u8g2.drawStr( 80, 10, "PPM");

  u8g2.setFont(u8g_font_profont15);
  u8g2.drawStr( 15, 45, "Temp.");
  u8g2.drawStr( 55, 45, temperatureAsChar);
  u8g2.drawStr( 85, 45, "\260C");
  u8g2.drawStr( 15, 60, " Hum.");
  u8g2.drawStr( 55, 60, humidityAsChar);
  u8g2.drawStr( 90, 60, "%");
}

void changeSemaphore(int co2ppm) {
  digitalWrite(SEMAPHORE_RED_PIN, LOW);
  digitalWrite(SEMAPHORE_ORANGE_PIN, LOW);
  digitalWrite(SEMAPHORE_GREEN_PIN, LOW);

  if (co2ppm >= CO2_LIMIT_RED) {
    digitalWrite(SEMAPHORE_RED_PIN, HIGH);
  }

  if (co2ppm >= CO2_LIMIT_ORANGE && co2ppm < CO2_LIMIT_RED ) {
    digitalWrite(SEMAPHORE_ORANGE_PIN, HIGH);
  }

  if (co2ppm < CO2_LIMIT_ORANGE) {
    digitalWrite(SEMAPHORE_GREEN_PIN, HIGH);
  }
}

int readCO2ppm() {
  CO2Sensor.write(readCO2, 9);
  delay(500);
  CO2Sensor.readBytes(bufferCO2, 9);

  co2ppm = bufferCO2[2] * 256 + bufferCO2[3];

  return co2ppm;
}


void checkCalibrationButton() {
  if (LOW == digitalRead(CALIBRATION_BUTTON_PIN)) {
    CO2Sensor.write(startZeroPointCalibration, 9);
  }
}

void setup(void) {
  Serial.begin(9600);

  CO2Sensor.begin(9600);
  CO2Sensor.write(SetResolutionTo2000PPM, 9);
  CO2Sensor.write(selfCalibrationZeroPointOFF, 9);

  dht.begin();

  u8g2.begin();

  pinMode(SEMAPHORE_RED_PIN, OUTPUT);
  pinMode(SEMAPHORE_ORANGE_PIN, OUTPUT);
  pinMode(SEMAPHORE_GREEN_PIN, OUTPUT);

  pinMode(CALIBRATION_BUTTON_PIN, INPUT_PULLUP);
}

void loop(void) {
  co2ppm = readCO2ppm();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  changeSemaphore(co2ppm);

  checkCalibrationButton();

  toggleOLEDMarker = !toggleOLEDMarker;
  u8g2.firstPage();
  do {
    draw(co2ppm, temperature, humidity);
  } while ( u8g2.nextPage() );

  delay(2000);
}
