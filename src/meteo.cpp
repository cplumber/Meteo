#include <Arduino.h>
#include <WiFi.h>         // First Wi-Fi, as it depends on the network stack
#include <WiFiUdp.h>      // Then UDP for NTP (works with Wi-Fi)
#include <NTPClient.h>    // NTP client (depends on WiFi and WiFiUdp)
#include <WebServer.h>    // Web server using Wi-Fi
#include <Wire.h>         // I2C (needed for sensors, displays, and memory)
#include <Preferences.h>  // EEPROM-like storage 

#include <FS.h>       
#include <SPIFFS.h>

#include <TFT_eSPI.h>     // Display (works with SPI, but does not depend on I2C)
#include <MQTT.h>

#include "Free_Fonts.h"   // Custom fonts for TFT (does not contain code)
#include "sensor_manager.h"
#include "meteo.h"


// To create a library from an image: 
// Generate a file with an image using https://notisrac.github.io/FileToCArray/
// Code format Hex (0x00)
// Palette mode 16-bit RRRRRGGGGGGBBBBB (2 bytes/pixel)
// Endianness - Big-endian
// Data type uint16_t
#include "template6.h"  // Generated image file

// Variables for data
String currentDate;
String currentTime;

// Declaring a structure for arrays of historical data
struct WeatherData {
  short int temperature;  // Temperature
  short int humidity;     // Humidity
  short int pressure;     // Pressure
  short int airQuality;   // CO2
};

// Declaring a structure for storing coordinates for text and graph outputs
struct UIElement {
  int x;            // X coordinate
  int y;            // Y coordinate
  uint16_t fgColor; // text/graphics color
};

// Min and max values for calculating graph scaling coefficients
int minTemp, maxTemp;         
int minHum, maxHum;
int minPres, maxPres;
int minAirQ, maxAirQ;

const int backlightPin = 21;  // Backlight pin
const int sensorPin = 34;     // Photoresistor pin
int backlight;                // Screen brightness value

unsigned long previousMillisClock = 0;
unsigned long previousMillisSensors = 0;
unsigned long previousMillisGraph = 0;
unsigned long previousMillisMQTT = 0;
unsigned long previousMillisNTP = 0;

const long intervalClock = 1000;        // Interval for updating date and time display in milliseconds (1 sec)
const long intervalSensors = 5000;      // Interval for reading sensors and updating display values in milliseconds (5 sec)
const long intervalNTP = 21600000;      // Interval for time synchronization with the NTP server in milliseconds (21600000 ms = 6 hours)
const int HISTORY_SIZE = 96;            // History array size - 96 values for 2 days (every 30 minutes)
int interval_graph = 80;                // Interval for updating graphs on the screen in seconds
int interval_MQTT = 1;                  // Interval for sending MQTT data in minutes (2 min = 120)

int dispRot = 3;         // Display orientation: 1 - power on the left, 3 - power on the right

WeatherData histogramData[HISTORY_SIZE];     // Array for storing averaged values for histogram - 96 values for 2 days (every 30 minutes)

// Coordinates for displaying text on the screen
UIElement tempText = {142, 40, 0xFEA0};   // sRGB: #FFD700
UIElement humText  = {142, 84, 0xACF2};   // sRGB: #A89C94
UIElement presText = {142, 128, 0xFAA4};  // sRGB: #FF5722
UIElement airText  = {142, 172, 0xFA20};  // sRGB: #FF4500

// Coordinates for displaying graphs on the screen
UIElement tempGraph = {210, 73, 0xFEA0};
UIElement humGraph  = {210, 117, 0xACF2};
UIElement presGraph = {210, 161, 0xFAA4};
UIElement airGraph  = {210, 205, 0xFA20};

// Coordinates for status lines
const int statusLine1x = 2;
const int statusLine1y = 215;
const int statusLine2x = 2;
const int statusLine2y = 228;

// Status line state colors
const uint16_t colorOK = 0x528A;
const uint16_t colorLost = 0xFFE0;

const int GRAPH_HEIGHT = 30;        // Height for all graphs

int ntpFailCount = 0;         // NTP connection failure counter
const int ntpMaxFails = 6;    // Number of attempts to connect to NTP before hiding the clock after 6 failures (3 hours), attempts continue
bool showClock = false;       // Clock display flag
bool mqttFail;                // MQTT connection failure flag
int wifiFailCount = 0;        // WiFi connection failure counter
const int wifiMaxFails = 9;   // Number of attempts to connect to WiFi before enabling AP mode
bool apModeActive = false;    // false - WiFi connection mode, true - access point mode

// Defining global colors
uint16_t bgColor = 0x001;      // Background color

int currentIndexAvg = 0;       // Index for accumulating elements to obtain average values

// Variables for accumulating raw data over 30 minutes for averaging
long sumTemperature = 0;
long sumHumidity = 0;
long sumPressure = 0;
long sumAirQuality = 0;

unsigned long lastForceSendMQTT = 0;   // Time of last forced MQTT transmission

// Previous sensor values for comparison before sending via MQTT
float lastTemp = -1000;
float lastHumidity = -1000;
float lastPressure = -1000;
float lastAirQuality = -1000;

// Thresholds for value changes (absolute values) to trigger immediate MQTT transmission
float ThresholdTemp;
float ThresholdHumidity;
float ThresholdPressure;
float ThresholdAirQuality;

// Screen setup
TFT_eSPI tft = TFT_eSPI();

// Wi-Fi setup
String wifi_ssid = "gia-kingdom-wi-fi";  // Wi-Fi network name
String wifi_password = "a1b2c3d4e5";     // Wi-Fi password
const char* APSSID = "gia-meteo";        // Access Point SSID
const char* APPassword = "12348765";     // Access Point password

// MQTT setup
String mqtt_server;
int mqtt_port;
String mqtt_username;
String mqtt_password;

WiFiClient net;
MQTTClient client;

// NTP setup
WiFiUDP udp;
NTPClient timeClient(udp, "pool.time.in.ua"); // NTP server 
int time_offset; // Time offset (UTC +2h) in minutes (must be passed in seconds)

Preferences prefs;
WebServer server(80);

String htmlFilePath = "/index.html";  // <-- Define path to HTML file

void setup() {
  Serial.begin(115200);   // Serial port initialization
  loadSettings();         // Load variables from non-volatile memory
  apModeActive = false;
  
  delay (500);
  WiFi.disconnect(true, true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_MODE_STA); // Access point mode
  
  // Attempt to connect to Wi-Fi if credentials are available
  Serial.print("Connecting to Wi-Fi");
  if (wifi_ssid != "" && wifi_password != "") {
    WiFi.begin(wifi_ssid, wifi_password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to Wi-Fi");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      apModeActive = false;
    } else {
      Serial.println("Failed to connect to Wi-Fi");
      apModeActive = true;
    }
  } else {
    Serial.println("No Wi-Fi credentials found. Starting AP...");
    apModeActive = true;
  }
  // Start access point
  if(apModeActive) {
    switchToAPMode();
  }

  timeClient.setTimeOffset(time_offset * 60);  //установка смещения времени относительно UTC в секундах
  timeClient.begin();                          // NTP initialization
  showClock=timeClient.forceUpdate();          // Update precise time readings
  
  //настройка управления яркостью 
  pinMode(backlightPin, OUTPUT);  // устанваливаем пин продсветки в режим выхода
  analogReadResolution(10);   // настраиваем вход для фоторезистора
  analogSetAttenuation(ADC_0db); // настройка чувствительности
  
  initializeSensors(); 

  tft.begin();               // Screen initialization
  tft.setRotation(dispRot);  // Screen orientation
  tft.fillScreen(bgColor);   // Fill screen with black color

  // Вывод изображения, хранящегося в PROGMEM
  // Преобразуем массив из PROGMEM в uint16_t*
  const uint16_t *img = (const uint16_t*)template6;
  tft.pushImage(0, 0, TEMPLATE6_WIDTH, TEMPLATE6_HEIGHT, img);  // Вывод изображения в координатах (0,0)

  // MQTT connection
  client.begin(mqtt_server.c_str(), mqtt_port, net);
  client.onMessage(messageReceived);
  client.setWill("homeassistant/sensor/esp32_sensor/availability", "offline", true, 1);
  connectMQTT();

  publishDeviceDiscovery();      // Send sensor configuration
  mqttFail = !client.connected(); // Flag for displaying status - check connection status  
  if (!SPIFFS.begin()) {  // <-- Add this to ensure SPIFFS is ready
    Serial.println("Failed to mount file system");
    return;
  }
 
  server.on("/api/weather", HTTP_GET, [](){
    String jsonResponse = "{\"temperature\": " + String(currentTemperature) + 
                          ", \"humidity\": " + String(currentHumidity) + 
                          ", \"pressure\": " + String(currentPressure) + 
                          ", \"air_quality\": " + String(currentAirQuality) + "}";
    server.send(200, "application/json", jsonResponse);  // <-- Send JSON response
  });

    server.on("/api/settings", HTTP_GET, [](){
      String jsonResponse = "{\"wifi_ssid\": \"" + wifi_ssid +
                            "\", \"wifi_password\": \"" + wifi_password +
                            "\", \"mqtt_server\": \"" + mqtt_server +
                            "\", \"mqtt_port\": " + String(mqtt_port) +
                            ", \"mqtt_username\": \"" + mqtt_username +
                            "\", \"mqtt_password\": \"" + mqtt_password +
                            "\", \"interval_MQTT\": " + String(interval_MQTT) +
                            ", \"ThresholdTemp\": " + String(ThresholdTemp) +
                            ", \"ThresholdHumidity\": " + String(ThresholdHumidity) +
                            ", \"ThresholdPressure\": " + String(ThresholdPressure) +
                            ", \"ThresholdAirQuality\": " + String(ThresholdAirQuality) +
                            ", \"time_offset\": " + String(time_offset) +
                            ", \"interval_graph\": " + String(interval_graph) +
                            ", \"dispRot\": " + String(dispRot) + "}";

      server.send(200, "application/json", jsonResponse);
  });

  // Запуск веб-сервера
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  showNetworks();    // Show Wi-Fi, NTP, MQTT statuses
}

void loop() {
  unsigned long currentMillis = millis();
  showClock = !apModeActive;              // не показываем часы в режиме AP

  //обновляем дату и время и выводим их на экран с периодичностью intervalClock (1 сек)
  if (currentMillis - previousMillisClock >= intervalClock) {
    previousMillisClock = currentMillis; // Сохраняем время срабатывания
    
    setBrightness();
    if(showClock) {
      unsigned long epochTime = timeClient.getEpochTime(); // Получение времени в формате Unix
      currentDate = getDate(epochTime);                    // Получаем дату
      currentTime = timeClient.getFormattedTime();         // Получаем текущее время в формате Unix (с секунд с 1 января 1970 года)

      tft.setTextColor(TFT_WHITE, bgColor); 
      tft.setTextDatum(TL_DATUM); 
      tft.setFreeFont(FSS9);
      tft.setTextPadding(215); 
      tft.drawString(currentDate, 1, 5);
      tft.setFreeFont(FSSB12);
      tft.setTextPadding(100); 
      tft.drawString(currentTime, 220, 4);
    } 
    else tft.fillRect(1, 1, 318, 27, bgColor);   // закрашиваем неактуальные показания часов
  }
 
  //обновляем показания датчиков и выводим их на экран с периодичностью intervalSensors (5 сек)
  if (currentMillis - previousMillisSensors >= intervalSensors) {
    previousMillisSensors = currentMillis; // Сохраняем время срабатывания

    checkWiFi();       // проверяем статус соединения WiFi, если не подключен, пытаемся переподключиться
    showNetworks();    // проверяем статусы wifi, ntp, mqtt и выводим сообщение на экран
    
    readSensors();
    
    sendMQTTData(currentMillis); // отправка данных по MQTT если необходимо 

    // Суммируем данные для вычисления средних значений и инкрементируем индекс
    sumTemperature += currentTemperature;
    sumHumidity += currentHumidity;
    sumPressure += currentPressure;
    sumAirQuality += currentAirQuality;
    currentIndexAvg++;

    tft.setTextPadding(95);
    tft.setTextDatum(TR_DATUM); // Центр текста относительно экрана
    tft.setFreeFont(FSSB18);
    tft.setTextColor(tempText.fgColor, bgColor);
    tft.drawFloat(currentTemperature , 1, tempText.x, tempText.y); // Выводим температуру, 1 знак после точки
    tft.setTextColor(humText.fgColor, bgColor);
    tft.drawFloat(currentHumidity , 1, humText.x, humText.y); // Выводим влажность, 1 знак после точки
    tft.setTextColor(presText.fgColor, bgColor);
    tft.drawFloat(currentPressure , 0, presText.x, presText.y); // Выводим давление, 0 знак после точки
    tft.setTextColor(airText.fgColor, bgColor);
    tft.drawFloat(currentAirQuality , 0, airText.x, airText.y); // Выводим качество воздуха, 0 знак после точки
  }

  //обновляем значения графиков и выводим их на экран с периодичностью interval_graph
  if (currentMillis - previousMillisGraph >= interval_graph * 1000) {
    previousMillisGraph = currentMillis; // Сохраняем время срабатывания

    //вычисляем средние значения
    if (currentIndexAvg > 0) {
      // Вычисление средних значений
      int avgTemperature = round(sumTemperature / (float)currentIndexAvg);
      int avgHumidity = round(sumHumidity / (float)currentIndexAvg);
      int avgPressure = round(sumPressure / (float)currentIndexAvg);
      int avgAirQuality = round(sumAirQuality / (float)currentIndexAvg);
      
      addToHistory(avgTemperature, avgHumidity, avgPressure, avgAirQuality); // добавляем исторические данные в массив
    }

    // обнуляем переменные для накопления данных с датчиков и счетчик
    sumTemperature = 0;
    sumHumidity = 0;
    sumPressure = 0;
    sumAirQuality = 0;
    currentIndexAvg = 0;
    
    drawGraph(); // отображаем графики на экране
  }

  //обновляем значения точного времени - синхронизация с NTP (раз в 6 часов)
  if (currentMillis - previousMillisNTP >= intervalNTP) {
    previousMillisNTP = currentMillis; // Сохраняем время срабатывания
    checkNTP();
  }

// тут выполняется код, не зависящий от вывода на экран
  server.handleClient();  //слушаем web-сервер

  mqttFail = !client.connected(); // Flag for displaying status - check connection status  
  if (mqttFail) connectMQTT();
  
  client.loop();      // слушаем MQTT-брокера и отправляем пакеты Time-Alive

  checkSerialInput(); // тестовая функция 0 откл, 1 вкл вайфай

}



// ****************************************************************************************************************************************
// дальше идут функции
// ****************************************************************************************************************************************

String getDate(unsigned long epochTime) {
  // Преобразование времени Unix в дату
  int currentYear = 1970;
  unsigned long days = epochTime / 86400; // Количество дней с 1 января 1970 года

  // Вычисляем год
  while (days >= 365) {
    if (isLeapYear(currentYear)) {
      if (days >= 366) {
        days -= 366;
      } else {
        break;
      }
    } else {
      days -= 365;
    }
    currentYear++;
  }

  // Вычисляем месяц и день
  int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (isLeapYear(currentYear)) {
    monthDays[1] = 29; // Февраль в високосный год
  }

  int currentMonth = 0;
  while (days >= monthDays[currentMonth]) {
    days -= monthDays[currentMonth];
    currentMonth++;
  }

  int currentDay = days + 1; // Добавляем 1, т.к. дни начинаются с 1

  // Вычисляем день недели
  const char* weekDays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  const char* currentWeekDay = weekDays[(epochTime / 86400 + 4) % 7]; // 1 января 1970 года — четверг

  // Формируем строку с датой
  char dateBuffer[22];
  snprintf(dateBuffer, sizeof(dateBuffer), "%s, %02d/%02d/%04d", currentWeekDay, currentDay, currentMonth + 1, currentYear);
  return String(dateBuffer); // Возвращаем строку
}

bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Функция регулировки яркости экрана
void setBrightness() {
  int sensorValue = analogRead(sensorPin);      // Получаем значение освещенности c фоторезистора. 0 - максимальная освещенность, 1023 - полная темнота
  float factor = 0.1;                           // Коэффициент плавности
  int brightness = 255 - pow(sensorValue, factor) * (255 / pow(1023, factor));
  brightness = constrain(brightness, 2, 255);   // Ограничение диапазона
  analogWrite(backlightPin, brightness);        // Устанавливаем яркость. Значение яркости. 0 - нет подсветки, 255 - максимальная яркость
}

// функция записи значений датчиков с предварительным сдвигом предыдущих и нахождением min и max значений каждого параметра
void addToHistory(int avgTemp, int avgHum, int avgPres, int avgAirQ) {
    // Сдвигаем элементы массива вправо и одновременно пересчитываем min/max
    minTemp = maxTemp = avgTemp;
    minHum = maxHum = avgHum;
    minPres = maxPres = avgPres;
    minAirQ = maxAirQ = avgAirQ;

    for (int i = HISTORY_SIZE - 1; i > 0; i--) {
      histogramData[i] = histogramData[i - 1];
      // Обновляем min/max, НЕ включая удалённое значение
      if (histogramData[i].temperature < minTemp) minTemp = histogramData[i].temperature;
      if (histogramData[i].temperature > maxTemp) maxTemp = histogramData[i].temperature;
      if (histogramData[i].humidity < minHum) minHum = histogramData[i].humidity;
      if (histogramData[i].humidity > maxHum) maxHum = histogramData[i].humidity;
      if (histogramData[i].pressure < minPres) minPres = histogramData[i].pressure;
      if (histogramData[i].pressure > maxPres) maxPres = histogramData[i].pressure;
      if (histogramData[i].airQuality < minAirQ) minAirQ = histogramData[i].airQuality;
      if (histogramData[i].airQuality > maxAirQ) maxAirQ = histogramData[i].airQuality;
    }
    // Вставляем новое значение в начало массива
    histogramData[0] = {
      static_cast<short int>(avgTemp),
      static_cast<short int>(avgHum),
      static_cast<short int>(avgPres),
      static_cast<short int>(avgAirQ)
  };
}

// Функция проверки соединения WiFi и попыток переподключиться
void checkWiFi() {
  if (apModeActive) return;

  if (wifiFailCount >= wifiMaxFails) {
    // Если не удается подключиться после заданного количества попыток, переключаемся в режим AP
    Serial.println("Too many failed attempts. Switching to AP mode.");
    apModeActive = true;        // Устанавливаем флаг, что режим AP активирован
    wifiFailCount = wifiMaxFails; // Обеспечиваем, что счетчик не выйдет за пределы
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiFailCount = 0;  // Если подключились, сбрасываем счетчик
    apModeActive = false;
  } else {
    wifiFailCount++;
    WiFi.begin(wifi_ssid, wifi_password);
  }
}

// Функция обновления показания точного времени. Если время не обновляется - скрываем часы
void checkNTP() {
  if (apModeActive) return;

  if (timeClient.update()) {
    ntpFailCount = 0;       // Сбрасываем счетчик, если время обновилось
    showClock = true;       // Показываем часы
  } else {
    ntpFailCount++;         // Увеличиваем счетчик неудач
    if (ntpFailCount >= ntpMaxFails) {
      showClock = false;          // Скрываем часы
      ntpFailCount = ntpMaxFails; // Обеспечиваем, что счетчик не выйдет за пределы
    }
  }
}

// Функция показа статуса сетей Wi-Fi, NTP и MQTT
void showNetworks() {
    tft.setFreeFont(NULL);
    tft.setTextPadding(180);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
  
    // Если Wi-Fi потерян — не показываем NTP и MQTT
    if (wifiFailCount != 0) {  
      tft.setTextColor(colorLost, bgColor);
      tft.drawString(wifiFailCount == wifiMaxFails ? "WiFi lost! Reboot to setup." : "WiFi lost!", statusLine1x, statusLine1y);
      tft.drawString(" ", statusLine2x, statusLine2y); // удаляем информацию со второй строки статуса
    } else {
    // Wi-Fi статус
      tft.setTextColor(colorOK, bgColor);
      tft.drawString("WiFi OK. IP: " + WiFi.localIP().toString(), statusLine1x, statusLine1y);
      // NTP статус
      tft.setTextColor((ntpFailCount == 0) ? colorOK : colorLost, bgColor);
      tft.drawString((ntpFailCount == 0) ? "NTP OK" : "NTP error!", statusLine2x, statusLine2y);
      // MQTT статус
      tft.setTextColor((mqttFail) ? colorLost : colorOK, bgColor);
      tft.drawString((mqttFail) ? "MQTT error!" : "MQTT OK", statusLine2x + 80, statusLine2y);
    }
    if (apModeActive && wifiFailCount == 0) {  // если загрузились в режиме AP, то wifiFailCount будет 0 - показываем настройки подключения - адрес, ssid и пароль
      tft.setFreeFont(NULL);
      tft.setTextPadding(180);
      tft.setTextSize(1);
      tft.setTextDatum(TL_DATUM); 
      tft.setTextColor(TFT_WHITE, bgColor);
      tft.drawString("AP mode. IP: " + WiFi.softAPIP().toString(), statusLine1x, statusLine1y);
      tft.drawString(String("SSID: ") + APSSID +", PW: " + APPassword, statusLine2x, statusLine2y);
    }
    

}

void switchToAPMode() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(APSSID, APPassword); // Запуск точки доступа
  
  Serial.println("AP IP address: ");
  Serial.println(WiFi.softAPIP()); // IP точки доступа
}

void drawGraph() {
    // Закрашиваем область графиков перед перерисовкой
    tft.fillRect(tempGraph.x-1, tempGraph.y-GRAPH_HEIGHT-1, HISTORY_SIZE+2, airGraph.y-tempGraph.y + GRAPH_HEIGHT+2, bgColor);

    for (int i = HISTORY_SIZE - 1; i > 0; i--) { // начинаем с последнего индекса
      // Проверка на деление на ноль
      int tempRange = maxTemp - minTemp;
      int humRange = maxHum - minHum;
      int presRange = maxPres - minPres;
      int airQRange = maxAirQ - minAirQ;

      // Если разница равна нулю, присваиваем безопасное значение
      if (tempRange == 0) tempRange = 1;
      if (humRange == 0) humRange = 1;
      if (presRange == 0) presRange = 1;
      if (airQRange == 0) airQRange = 1;

      // Нормализуем значения с учётом исправленных диапазонов
      int prevTempY = ((histogramData[i].temperature - minTemp) * GRAPH_HEIGHT) / tempRange;
      int currTempY = ((histogramData[i - 1].temperature - minTemp) * GRAPH_HEIGHT) / tempRange;
      int prevHumY = ((histogramData[i].humidity - minHum) * GRAPH_HEIGHT) / humRange;
      int currHumY = ((histogramData[i - 1].humidity - minHum) * GRAPH_HEIGHT) / humRange;
      int prevPresY = ((histogramData[i].pressure - minPres) * GRAPH_HEIGHT) / presRange;
      int currPresY = ((histogramData[i - 1].pressure - minPres) * GRAPH_HEIGHT) / presRange;
      int prevAirQY = ((histogramData[i].airQuality - minAirQ) * GRAPH_HEIGHT) / airQRange;
      int currAirQY = ((histogramData[i - 1].airQuality - minAirQ) * GRAPH_HEIGHT) / airQRange;

      tft.drawLine(tempGraph.x + (HISTORY_SIZE - 1 - i), tempGraph.y - prevTempY, tempGraph.x + (HISTORY_SIZE - i), tempGraph.y - currTempY, tempGraph.fgColor);
      tft.drawLine(humGraph.x + (HISTORY_SIZE - 1 - i), humGraph.y - prevHumY, humGraph.x + (HISTORY_SIZE - i), humGraph.y - currHumY, humGraph.fgColor);
      tft.drawLine(presGraph.x + (HISTORY_SIZE - 1 - i), presGraph.y - prevPresY, presGraph.x + (HISTORY_SIZE - i), presGraph.y - currPresY, presGraph.fgColor);
      tft.drawLine(airGraph.x + (HISTORY_SIZE - 1 - i), airGraph.y - prevAirQY, airGraph.x + (HISTORY_SIZE - i), airGraph.y - currAirQY, airGraph.fgColor);
    }
}

void handleRoot() {
  File htmlFile = SPIFFS.open("/index.html", "r");
  if (htmlFile) {
      String htmlContent = htmlFile.readString();
      htmlFile.close();
      server.send(200, "text/html", htmlContent);
  } else {
      server.send(404, "text/plain", "File Not Found");
  }
}

// функция формирования обработки "update" web-страницы
void handleSave() {
  // Получаем параметры с веб-страницы в постоянной памяти
  wifi_ssid = server.arg("wifi_ssid");
  wifi_password = server.arg("wifi_password");
  mqtt_server = server.arg("mqtt_server");
  mqtt_port = server.arg("mqtt_port").toInt();
  mqtt_username = server.arg("mqtt_username");
  mqtt_password = server.arg("mqtt_password");

  ThresholdTemp = server.arg("ThresholdTemp").toFloat();
  ThresholdHumidity = server.arg("ThresholdHumidity").toFloat();
  ThresholdPressure = server.arg("ThresholdPressure").toFloat();
  ThresholdAirQuality = server.arg("ThresholdAirQuality").toFloat();

  interval_MQTT = server.arg("interval_MQTT").toInt();
  time_offset = server.arg("time_offset").toInt();
  interval_graph = server.arg("interval_graph").toInt();
  dispRot = server.arg("dispRot").toInt();

  // Сохраняем все параметры в постоянной памяти
  prefs.begin("config", false); 

  prefs.putString("wifi_ssid", wifi_ssid);
  prefs.putString("wifi_password", wifi_password);
  prefs.putString("mqtt_server", mqtt_server);
  prefs.putInt("mqtt_port", mqtt_port);
  prefs.putString("mqtt_username", mqtt_username);
  prefs.putString("mqtt_password", mqtt_password);
  prefs.putFloat("ThTemp", ThresholdTemp);
  prefs.putFloat("ThHumidity", ThresholdHumidity);
  prefs.putFloat("ThPressure", ThresholdPressure);
  prefs.putFloat("ThAirQ", ThresholdAirQuality);
  prefs.putInt("interval_MQTT", interval_MQTT);
  prefs.putInt("time_offset", time_offset);
  prefs.putInt("interval_graph", interval_graph);
  prefs.putInt("dispRot", dispRot);

  prefs.end();

  server.send(200, "text/html", "<html><body><h1>Settings Saved! Rebooting...</h1></body></html>");
  delay(500);

  WiFi.mode(WIFI_OFF);     // Полностью отключаем Wi-Fi
  WiFi.disconnect();       // Удаляем все сети и сбрасываем Wi-Fi настройки

  ESP.restart();   // перезагрузка
}

// Функция загрузки всех настроек
void loadSettings() {
  prefs.begin("config", false);  // Открываем пространство "config"
  // Wi-Fi
  wifi_ssid = prefs.getString("wifi_ssid", "");
  wifi_password = prefs.getString("wifi_password", "");
  // MQTT
  mqtt_server = prefs.getString("mqtt_server", "");
  mqtt_port = prefs.getInt("mqtt_port", 1883);
  mqtt_username = prefs.getString("mqtt_username", "");
  mqtt_password = prefs.getString("mqtt_password", "");
  interval_MQTT = prefs.getInt("interval_MQTT", 2);   // если нет такого ключа, значение будет 2 минуты
  ThresholdTemp = prefs.getFloat("ThTemp", 1.0);
  ThresholdHumidity = prefs.getFloat("ThHumidity", 1.0);
  ThresholdPressure = prefs.getFloat("ThPressure", 1.0);
  ThresholdAirQuality = prefs.getFloat("ThAirQ", 1.0);
  // Прочее
  time_offset = prefs.getInt("time_offset", 0);
  interval_graph = prefs.getInt("interval_graph", 1);
  dispRot = prefs.getInt("dispRot", 1);
  prefs.end();  // Закрываем пространство
}

// Отправка конфигурации устройства в Home Assistant
void publishDeviceDiscovery() {
  // Описание устройства
  String deviceInfo = "\"device\": {"
                      "\"identifiers\": [\"esp32_sensor\"],"
                      "\"name\": \"ESP32 Sensor\","
                      "\"manufacturer\": \"DIY\","
                      "\"model\": \"ESP32 Weather Station\","
                      "\"sw_version\": \"1.4\""
                      "}";

  String deviceSettings =  "\"availability_topic\": \"homeassistant/sensor/esp32_sensor/availability\","
                           "\"payload_available\": \"online\","
                           "\"payload_not_available\": \"offline\",";

  // Конфигурация для температуры
  String configTempTopic = "homeassistant/sensor/esp32_temperature/config";
  String configTempPayload = "{\"name\": \"Temperature\","
                             "\"unique_id\": \"esp32_temperature\","
                             "\"device_class\": \"temperature\","
                             "\"unit_of_measurement\": \"°C\","
                             "\"state_topic\": \"homeassistant/sensor/esp32_temperature/state\","
                             "\"value_template\": \"{{ value_json.temperature }}\","
                             + deviceSettings + deviceInfo + "}";

  // Конфигурация для влажности
  String configHumTopic = "homeassistant/sensor/esp32_humidity/config";
  String configHumPayload = "{\"name\": \"Humidity\","
                            "\"unique_id\": \"esp32_humidity\","
                            "\"device_class\": \"humidity\","
                            "\"unit_of_measurement\": \"%\","
                            "\"state_topic\": \"homeassistant/sensor/esp32_humidity/state\","
                            "\"value_template\": \"{{ value_json.humidity }}\","
                            + deviceSettings + deviceInfo + "}";

  // Конфигурация для давления
  String configPresTopic = "homeassistant/sensor/esp32_pressure/config";
  String configPresPayload = "{\"name\": \"Pressure\","
                          "\"unique_id\": \"esp32_pressure\","
                          "\"device_class\": \"pressure\","
                          "\"unit_of_measurement\": \"hPa\","
                          "\"state_topic\": \"homeassistant/sensor/esp32_pressure/state\","
                          "\"value_template\": \"{{ value_json.pressure }}\","
                          + deviceSettings + deviceInfo + "}";

  // Конфигурация для качества воздуха с описанием устройства
  String configAirQTopic = "homeassistant/sensor/esp32_air_quality/config";
  String configAirQPayload = "{\"name\": \"Carbon Dioxide\","
                            "\"unique_id\": \"esp32_air_quality\","
                            "\"device_class\": \"carbon_dioxide\","
                            "\"unit_of_measurement\": \"ppm\","
                            "\"state_topic\": \"homeassistant/sensor/esp32_air_quality/state\","
                            "\"value_template\": \"{{ value_json.air_quality }}\","
                            + deviceSettings + deviceInfo + "}";


  // Публикация конфигурации сенсоров
  client.publish(configTempTopic.c_str(), configTempPayload.c_str(), true, 1);
  client.publish(configHumTopic.c_str(), configHumPayload.c_str(), true, 1);
  client.publish(configPresTopic.c_str(), configPresPayload.c_str(), true, 1);
  client.publish(configAirQTopic.c_str(), configAirQPayload.c_str(), true, 1);

}

// проверка и отправка данных по MQTT если данные изменились или прошло время interval_MQTT (в минутах)
void sendMQTTData(unsigned long timestamp) {

// отправка по изменениям датчиков 
  if (abs(currentTemperature - lastTemp) >= ThresholdTemp) {
      publishSensorData("temperature", currentTemperature, 1);
      lastTemp = currentTemperature;
  }
  if (abs(currentHumidity - lastHumidity) >= ThresholdHumidity) {
      publishSensorData("humidity", currentHumidity, 1);
      lastHumidity = currentHumidity;
  }
  if (abs(currentPressure - lastPressure) >= ThresholdPressure) {
      publishSensorData("pressure", currentPressure, 0);
      lastPressure = currentPressure;
  }
  if (abs(currentAirQuality - lastAirQuality) >= ThresholdAirQuality) {
      publishSensorData("air_quality", currentAirQuality, 0);
      lastAirQuality = currentAirQuality;
  }

  // Принудительная отправка по настроенному интервалу
  if (timestamp - lastForceSendMQTT >= interval_MQTT * 60000) {
    publishSensorData("temperature", currentTemperature, 1);
    publishSensorData("humidity", currentHumidity, 1);
    publishSensorData("pressure", currentPressure, 0);
    publishSensorData("air_quality", currentAirQuality, 0);

    lastTemp = currentTemperature;
    lastHumidity = currentHumidity;
    lastPressure = currentPressure;
    lastAirQuality = currentAirQuality;

    lastForceSendMQTT = timestamp;
  }
}

// Публикация показания датчика sensor = {"temperature", "humidity", "pressure", "air_quality"}, value - значение, numDecimals - кол-во знаков после запятой
void publishSensorData(String sensor, float value, int numDecimals) {
  String stateTopic = "homeassistant/sensor/esp32_" + sensor + "/state";
  String payload = "{\"" + sensor + "\": " + String(value, numDecimals) + "}";
  client.publish(stateTopic.c_str(), payload.c_str(), true, 1);
}

// Обработчик входящих MQTT-сообщений
void messageReceived(String &topic, String &payload) {
  Serial.println("Получено сообщение [");
  Serial.println(topic);
  Serial.println("]: ");
  Serial.println(payload);
}

// Функция подключения к MQTT
void connectMQTT() {
  client.connect("ESP32Client", mqtt_username.c_str(), mqtt_password.c_str());
  client.publish("homeassistant/sensor/esp32_sensor/availability", "online", true, 1);
}



// отладочная функция!!! По команде 1 в консоле - отключает wi-fi
void checkSerialInput() {
  if (Serial.available()) {          // Если есть ввод в консоли
    String input = Serial.readString();  // Читаем введённое значение
    input.trim();                     // Убираем пробелы и переводы строк

    if (input == "0") {                // Если введено "0"
      Serial.println("WiFi отключается...");
      WiFi.disconnect();               // Отключаем Wi-Fi
      Serial.println("WiFi отключен.");
      Serial.println("Свободно памяти: ");
      Serial.println(ESP.getFreeHeap());
      Serial.println("SDK: ");
      Serial.println(ESP.getSdkVersion());
      Serial.println("Чип: ");
      Serial.println(ESP.getChipRevision());
      Serial.println("Частота: ");
      Serial.println(ESP.getCpuFreqMHz());
      Serial.println("Мин память: ");
      Serial.println(ESP.getMinFreeHeap());

    } 
    
    else if (input == "1") {           // Если введено "1"
      Serial.println("WiFi подключается...");
      WiFi.begin(wifi_ssid, wifi_password);      // Подключаем Wi-Fi

      int attempt = 0;
      while (WiFi.status() != WL_CONNECTED && attempt < 10) { // Ждем подключения
        delay(1000);
        Serial.println(".");
        attempt++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi подключен! IP-адрес: " + WiFi.localIP().toString());
      } else {
        Serial.println("\nНе удалось подключиться к Wi-Fi.");
      }
    }
  }
}



