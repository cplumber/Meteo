# ESP32 Project

## Description

This project is designed to work with the **ESP32** and uses the built-in touchscreen display **ILI9341**. The project includes various features such as Wi-Fi connectivity, MQTT data transmission, and displaying information on the screen.

**Used hardware:**
- **Microcontroller board:** ESP32-2432S028 aka Cheap Yellow Display (ESP-WROOM-32 with integrated ILI9341 touchscreen display)
- **Sensors:**
    * [BME280](https://www.bosch-sensortec.com/products/environmental-sensors/humidity-sensors-bme280/)
    * [SCD40](https://sensirion.com/products/catalog/SCD40)

  The sensors are connected via the I2C interface.

## Features
- Reads data from BME280 and SCD40 sensors.
- Stores sensor data for a configurable period (default 48 hours).
- Sends data to MQTT broker at configurable intervals (including forced messages).
- Displays real-time readings and historical graphs on an ILI9241 screen.
- When Wi-Fi connection fails, the device switches to Access Point mode.
- After connecting to the Access Point, all settings can be configured through a web interface.
- After configuration (including Wi-Fi settings), the device will reboot and connect to the configured Wi-Fi network.
- Device IP address is shown on the screen for easy access to the web interface.

## Required Libraries

To use this project, you need to install the following libraries. Some are included by default in the Arduino IDE, some are installed with the ESP32 board package, and others need to be installed manually.

### **Included in Arduino IDE**  
These libraries are built into the Arduino IDE and do not require additional installation:  
- **Arduino.h** – Core Arduino functions.  
- **Wire.h** – I2C communication.  

### **Installed with ESP32 Board Package**  
These libraries are included when you install the ESP32 board support in the Arduino IDE:  
- **WiFi.h** – Wi-Fi communication.  
- **WiFiUdp.h** – UDP communication for NTP.  
- **WebServer.h** – Lightweight web server for ESP32.  
- **Preferences.h** – EEPROM-like storage for ESP32.  

#### **How to Install ESP32 Board Support in Arduino IDE**  
1. Open **File** → **Preferences**  
2. In the **Additional Board Manager URLs** field, add:  
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Open **Tools** → **Board** → **Boards Manager**  
4. Search for **ESP32** and install the package by **Espressif Systems**.  
5. Select the appropriate ESP32 board from the **Tools → Board** menu.  

### **Manually Installed Libraries**  
These libraries need to be installed separately via the Arduino Library Manager or from GitHub.  

#### **Installation via Arduino Library Manager**  
1. Open **Arduino IDE**.  
2. Go to **Sketch** → **Include Library** → **Manage Libraries**.  
3. Search for the library name and click **Install**.  

#### **Required Libraries:**  
- **[NTPClient](https://github.com/arduino-libraries/NTPClient)** – NTP time synchronization.  
- **[SensirionI2cScd4x](https://github.com/Sensirion/arduino-i2c-scd4x)** – SCD40 CO₂ sensor driver.  
- **[Adafruit BME280](https://github.com/adafruit/Adafruit_BME280_Library)** – BME280 temperature, humidity, and pressure sensor driver.  
- **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)** – High-speed TFT display library.  
  - **Configuration:** The library requires manual configuration in `User_Setup.h`.  
  - A preconfigured **User_Setup.h** file is available in this repository. You need to replace the original file located in the **TFT_eSPI** library folder.  
  - The file **Free_Fonts.h** is part of the **TFT_eSPI** examples and does not require separate installation.  
- **[MQTT](https://github.com/256dpi/arduino-mqtt)** – Lightweight MQTT client for IoT communication.  


## Installation

### 1. Clone the Repository  
Clone this repository or download it as a ZIP archive and extract it.  

### 2. Open the Project in Arduino IDE  
Launch Arduino IDE and open the main sketch file.  

### 3. Install Required Libraries  
Ensure all required libraries listed in the [Required Libraries](#required-libraries) section are installed.  

### 4. Configure the TFT_eSPI Library  
Before uploading the code, replace the default **User_Setup.h** file in the **TFT_eSPI** library with the preconfigured file from this repository.  

- Locate the installed **TFT_eSPI** library folder.  
- Copy **User_Setup.h** from this repository into that folder, replacing the existing file.  

### 5. Connect the Hardware  
Refer to the [Hardware Setup](#hardware-setup) section for details on wiring the sensors.  

### 6. Upload the Code  
Connect the ESP32 board to your computer and upload the code.  


## Hardware Setup  

### Microcontroller Board  
- **ESP32-2432S028** (ESP-WROOM-32 with integrated touchscreen display ILI9341)  

### I2C Connection  
The ESP32 module has dedicated I2C lines routed to **connector CN1**, which provides power and I2C signals:  

| Pin | Function | ESP32 GPIO |
|------|-----------|------------|
| 1    | +3.3V    | -          |
| 2    | SDA      | GPIO27     |
| 3    | SCL      | GPIO22     |
| 4    | GND      | -          |

By convention:  
- **SCL (Clock) → GPIO22**  
- **SDA (Data) → GPIO27**  

### Sensor Wiring  
Connect the sensors as follows:  

| Sensor  | VCC  | GND  | SCL (GPIO22) | SDA (GPIO27) |
|---------|------|------|-------------|-------------|
| BME280  | 3.3V | GND  | GPIO22       | GPIO27      |
| SCD40   | 3.3V | GND  | GPIO22       | GPIO27      |

### I2C Pull-Up Resistors  
I2C lines require pull-up resistors to operate correctly.  

- The **GPIO27 (SDA) line** already has a built-in **10kΩ pull-up resistor** on the ESP32 module.  
- The **GPIO22 (SCL) line** requires an **external 10kΩ pull-up resistor** to +3.3V.  

> **Important:** When connecting I2C sensors, ensure an additional **10kΩ pull-up resistor** is installed between **GPIO22 (SCL) and +3.3V**.  


## Usage
1. Once the code is uploaded, the device will try to connect to a pre-configured Wi-Fi network.
2. If the Wi-Fi connection fails, the device will switch to Access Point mode.
3. The device's IP address will be displayed on the screen.
4. Connect to the Access Point and open the web interface by entering the displayed IP address in a web browser.
5. Configure the Wi-Fi settings, MQTT parameters, data storage duration, and MQTT message sending intervals through the web interface.
6. After the configuration is complete, the device will reboot and connect to the configured Wi-Fi network.
7. The device's IP address will continue to be shown on the screen and can be used for further access to the web interface.

## License
This project is licensed under the MIT License – see the [LICENSE.md](LICENSE.md) file for details.

## Donations

If you find this project useful and would like to support its development, you can donate via any of the following methods:

- **PayPal**: [gia@gia.org.ua] [Donate via PayPal](https://www.paypal.me)  
- **Ko-fi**: [Donate on Ko-fi](https://ko-fi.com/igorgimelfarb)  

**Your support is greatly appreciated!**


---

# Проект на ESP32

## Описание

Этот проект предназначен для работы с **ESP32** и использует встроенный тач-скрин дисплей **ILI9341**. Проект включает в себя различные функции, такие как подключение к Wi-Fi, отправка данных через MQTT и отображение информации на экране.

**Используемое оборудование:**
- **Плата микроконтроллера:** ESP32-2432S028 также известная как Cheap Yellow Display (ESP-WROOM-32 с интегрированным тач-скрин дисплеем ILI9341)
- **Датчики:**
    * [BME280](https://www.bosch-sensortec.com/products/environmental-sensors/humidity-sensors-bme280/)
    * [SCD40](https://sensirion.com/products/catalog/SCD40)

  Датчики подключены через I2C интерфейс.


## Возможности
- Чтение данных с датчиков BME280 и SCD40.
- Хранение данных за настраиваемый период (по умолчанию 48 часов).
- Отправка данных на MQTT-сервер с настраиваемым интервалом (включая принудительную отправку сообщений).
- Отображение текущих показателей и исторических графиков на экране ILI9241.
- При невозможности подключения к Wi-Fi устройство переключается в режим точки доступа.
- После подключения к точке доступа можно настроить все параметры через веб-интерфейс.
- После настройки (включая параметры подключения к Wi-Fi) устройство перезагружается и подключается к сети.
- IP-адрес устройства отображается на экране для удобства доступа к веб-интерфейсу.

## Необходимые библиотеки

Для работы этого проекта требуются следующие библиотеки. Некоторые из них включены в стандартный набор Arduino IDE, другие устанавливаются вместе с пакетом плат ESP32, а некоторые нужно устанавливать вручную.

### **Включены в Arduino IDE**  
Эти библиотеки уже есть в стандартной установке Arduino IDE и не требуют дополнительной установки:  
- **Arduino.h** – основные функции Arduino.  
- **Wire.h** – работа с интерфейсом I2C.  

### **Устанавливаются с пакетом плат ESP32**  
Эти библиотеки устанавливаются автоматически при добавлении поддержки плат ESP32 в Arduino IDE:  
- **WiFi.h** – работа с Wi-Fi.  
- **WiFiUdp.h** – работа с UDP (используется для NTP).  
- **WebServer.h** – легковесный веб-сервер для ESP32.  
- **Preferences.h** – хранение данных, аналог EEPROM.  

#### **Как установить поддержку плат ESP32 в Arduino IDE**  
1. Откройте **Файл** → **Настройки**  
2. В поле **Дополнительные URL-адреса для менеджера плат** добавьте:  
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Откройте **Инструменты** → **Плата** → **Менеджер плат**  
4. Введите **ESP32** в поиске и установите пакет от **Espressif Systems**.  
5. Выберите нужную плату ESP32 в меню **Инструменты → Плата**.  

### **Дополнительно устанавливаемые библиотеки**  
Эти библиотеки необходимо установить отдельно через **Arduino Library Manager** или вручную с GitHub.  

#### **Установка через Arduino Library Manager**  
1. Откройте **Arduino IDE**.  
2. Перейдите в **Скетч** → **Подключить библиотеку** → **Управлять библиотеками**.  
3. Найдите нужную библиотеку и нажмите **Установить**.  

#### **Необходимые библиотеки:**  
- **[NTPClient](https://github.com/arduino-libraries/NTPClient)** – клиент NTP для синхронизации времени.  
- **[SensirionI2cScd4x](https://github.com/Sensirion/arduino-i2c-scd4x)** – драйвер для датчика CO₂ SCD40.  
- **[Adafruit BME280](https://github.com/adafruit/Adafruit_BME280_Library)** – драйвер для датчика температуры, влажности и давления BME280.  
- **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)** – высокоскоростная библиотека для TFT-дисплеев.  
- **Настройка:** библиотека требует ручной конфигурации в файле `User_Setup.h`.  
- В репозитории уже есть преднастроенный **User_Setup.h**, его нужно заменить в папке библиотеки **TFT_eSPI**.  
- Файл **Free_Fonts.h** является частью примеров **TFT_eSPI** и не требует отдельной установки.  
- **[MQTT](https://github.com/256dpi/arduino-mqtt)** – легковесный MQTT-клиент для IoT-устройств.  

## Установка

### 1. Клонировать репозиторий  
Клонируйте этот репозиторий или скачайте его как ZIP-архив и извлеките.

### 2. Открыть проект в Arduino IDE  
Запустите Arduino IDE и откройте основной файл скетча.

### 3. Установить необходимые библиотеки  
Убедитесь, что все необходимые библиотеки, указанные в разделе [Необходимые библиотеки](#required-libraries), установлены.

### 4. Настроить библиотеку TFT_eSPI  
Перед загрузкой кода замените файл **User_Setup.h** в библиотеке **TFT_eSPI** на предварительно настроенный файл из этого репозитория.

- Найдите папку установленной библиотеки **TFT_eSPI**.  
- Скопируйте **User_Setup.h** из этого репозитория в эту папку, заменив существующий файл.

### 5. Подключить оборудование  
Обратитесь к разделу [Настройка оборудования](#hardware-setup) для получения подробностей по подключению датчиков.

### 6. Загрузить код  
Подключите плату ESP32 к компьютеру и загрузите код.

## Настройка оборудования

### Плата микроконтроллера  
- **ESP32-2432S028** (ESP-WROOM-32 с интегрированным экраном ILI9341)

### Подключение I2C  
Модуль ESP32 имеет выделенные линии I2C, которые подключены к **разъему CN1**, предоставляющему питание и сигналы I2C:

| Пин  | Функция | GPIO ESP32 |
|------|---------|------------|
| 1    | +3.3V   | -          |
| 2    | SDA     | GPIO27     |
| 3    | SCL     | GPIO22     |
| 4    | GND     | -          |

По умолчанию:  
- **SCL (Clock) → GPIO22**  
- **SDA (Data) → GPIO27**

### Подключение датчиков  
Подключите датчики следующим образом:

| Датчик  | VCC  | GND  | SCL (GPIO22) | SDA (GPIO27) |
|---------|------|------|--------------|--------------|
| BME280  | 3.3V | GND  | GPIO22       | GPIO27       |
| SCD40   | 3.3V | GND  | GPIO22       | GPIO27       |

### Резисторы подтяжки для I2C  
Линии I2C требуют использования резисторов подтяжки для корректной работы.

- Линия **GPIO27 (SDA)** уже имеет встроенный **резистор подтяжки 10kΩ** на модуле ESP32.  
- Линия **GPIO22 (SCL)** требует **внешнего резистора подтяжки 10kΩ** к +3.3V.

> **Важно:** При подключении датчиков I2C убедитесь, что дополнительный **резистор подтяжки 10kΩ** установлен между **GPIO22 (SCL) и +3.3V**.


## Лицензия
Этот проект лицензирован по лицензии MIT — см. файл [LICENSE.md](LICENSE.md) для подробностей.

##  Поддержка
Если вы находите этот проект полезным и хотите поддержать его разработку, вы можете сделать пожертвование любым из следующих способов:

- **PayPal**: [gia@gia.org.ua] [Donate via PayPal](https://www.paypal.me)  
- **Ko-fi**: [Donate on Ko-fi](https://ko-fi.com/igorgimelfarb)  


**Ваша поддержка будет очень ценна!**
