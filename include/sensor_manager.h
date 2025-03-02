#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

extern float currentTemperature;
extern float currentHumidity;
extern float currentPressure;
extern int currentAirQuality;

void initializeSensors();
void readSensors();

#endif // SENSOR_MANAGER_H
