#include "sensor_manager.h"
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <Adafruit_BME280.h>

// I2C bus pins
#define SDA_PIN 27   
#define SCL_PIN 22

SensirionI2cScd4x scd4x;
Adafruit_BME280 bme;

float currentTemperature;
float currentHumidity;
float currentPressure;
int currentAirQuality;

// Variables for reading data from SCD40
uint16_t co2;
float scd_temp, scd_humidity;

static char errorMessage[64];
static int16_t error;

#define NO_ERROR 0

void initializeSensors() {
    // I2C initialization
    Wire.begin(SDA_PIN, SCL_PIN);
  
    // SCD40 initialization - no changes here
    scd4x.begin(Wire, 0x62);
    scd4x.startPeriodicMeasurement();
  
    // BME280 initialization - no changes here
    if (!bme.begin(0x76)) {
      Serial.println("Ошибка BME280!");
    }
  }

  void readSensors() {
    bool dataReady = false;
  
    // Check if the data is ready from the SCD40 sensor
    error = scd4x.getDataReadyStatus(dataReady);
    if (error != NO_ERROR) {
      Serial.print("Error trying to execute getDataReadyStatus(): ");
      errorToString(error, errorMessage, sizeof errorMessage); // Keep error string for better error handling
      Serial.println(errorMessage);
    }
  
    // Wait until data is ready
    while (!dataReady) {
      delay(100);
      error = scd4x.getDataReadyStatus(dataReady);
      if (error != NO_ERROR) {
        Serial.print("Error trying to execute getDataReadyStatus(): ");
        errorToString(error, errorMessage, sizeof errorMessage); // Keep error string for better error handling
        Serial.println(errorMessage);
      }
    }
  
    // Read the measurements from the SCD40 sensor
    error = scd4x.readMeasurement(co2, scd_temp, scd_humidity);
    if (error != NO_ERROR) {
      Serial.print("Error trying to execute readMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage); // Keep error string for better error handling
      Serial.println(errorMessage);
    }
  
    // Assign the values to the global variables
    currentAirQuality = co2;
    currentTemperature = bme.readTemperature();
    currentHumidity = bme.readHumidity();
    currentHumidity = (currentHumidity > 100 ? 100 : currentHumidity); // Ensure humidity is within the correct range
    currentPressure = bme.readPressure() / 100.0F; // Pressure in hPa
  }