#include <Arduino.h>
// Function Declarations for meteo.cpp

void setup();
void loop();
String getDate(unsigned long epochTime);
bool isLeapYear(int year);
void setBrightness();
void addToHistory(int avgTemp, int avgHum, int avgPres, int avgAirQ);
void checkWiFi();
void checkNTP();
void showNetworks();
void switchToAPMode();
void drawGraph();
void handleRoot();
void handleSave();
void loadSettings();
void publishDeviceDiscovery();
void sendMQTTData(unsigned long timestamp);
void publishSensorData(String sensor, float value, int numDecimals);
void messageReceived(String &topic, String &payload);
void connectMQTT();
void checkSerialInput();
void serveHTML();