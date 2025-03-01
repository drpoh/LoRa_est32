// Передатчик (Sender) - Версия v1.0
// Устройство для отправки состояния контактов через LoRa с режимом сна

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Пины для LoRa и OLED
#define SCK     5    // SPI SCK
#define MISO    19   // SPI MISO
#define MOSI    27   // SPI MOSI
#define SS      18   // LoRa CS
#define RST     14   // LoRa Reset
#define DIO0    26   // LoRa DIO0
#define OLED_SDA 4   // I2C SDA
#define OLED_SCL 15  // I2C SCL
#define OLED_RST 16  // OLED Reset
#define CONTACT_PIN 0 // Вход для состояния контактов (PRG кнопка) и пробуждения
#define LED_PIN 2     // LED для индикации активности

// Константы конфигурации
#define BAND 868E6          // Частота LoRa (868 МГц)
#define SEND_INTERVAL 50    // Интервал проверки и отправки данных (мс)
#define DISPLAY_INTERVAL 50 // Интервал обновления дисплея (мс)
#define SLEEP_TIMEOUT 1800000 // Время до перехода в сон (мс, 30 минут)
#define SLEEP_WARNING 10000   // Время до сна для предупреждения (мс, 10 с)
#define SCREEN_WIDTH 128      // Ширина дисплея
#define SCREEN_HEIGHT 64      // Высота дисплея

// Константы строк
#define STR_SENDER "Sender"
#define STR_CLOSED "CLOSED"
#define STR_OPEN "OPEN"
#define STR_OK "OK"
#define STR_UPTIME "Uptime: "
#define STR_CONTACTS_CLOSED "Contacts CLOSED"
#define STR_CONTACTS_OPEN "Contacts OPEN"
#define STR_VERSION "v1.0"
#define STR_ERROR "ERROR"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

String lastState = "";         // Последнее отправленное состояние
unsigned long lastPingTime = 0; // Время последней отправки
unsigned long lastDisplayTime = 0; // Время последнего обновления дисплея
unsigned long lastStateChangeTime = 0; // Время последнего изменения состояния

// Прототипы функций
void displayUptime(unsigned long millis);
void showStartupScreen();
void checkHardware();

void setup() {
  Serial.begin(115200); // Инициализация Serial для отладки
  delay(100); // Небольшая задержка для стабилизации

  // Настройка пинов
  pinMode(CONTACT_PIN, INPUT_PULLUP); // Вход для контактов с подтяжкой
  pinMode(LED_PIN, OUTPUT);           // Выход для LED
  digitalWrite(LED_PIN, LOW);         // Выключаем LED

  pinMode(OLED_RST, OUTPUT); // Настройка пина сброса OLED
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL); // Инициализация I2C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Инициализация дисплея
    Serial.println(F("SSD1306 allocation failed"));
    while (1); // Остановка при сбое
  }

  showStartupScreen(); // Показ стартового экрана

  SPI.begin(SCK, MISO, MOSI, SS); // Инициализация SPI
  LoRa.setPins(SS, RST, DIO0);    // Настройка пинов LoRa
  if (!LoRa.begin(BAND)) {        // Инициализация LoRa
    Serial.println(F("LoRa init failed! Retrying..."));
    delay(1000);
    if (!LoRa.begin(BAND)) { // Повторная попытка
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println(STR_ERROR);
      display.display();
      while (1); // Остановка при сбое
    }
  }

  LoRa.setTxPower(20); // Установка мощности передачи
  LoRa.setSpreadingFactor(12); // Установка Spreading Factor

  checkHardware(); // Самодиагностика оборудования

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(STR_SENDER);
  display.display();
  Serial.println("Sender setup completed");

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Настройка пробуждения по PRG
  lastStateChangeTime = millis(); // Инициализация времени последнего изменения
}

// Основной цикл
void loop() {
  unsigned long currentMillis = millis();

  // Проверка и отправка состояния контактов
  if (currentMillis - lastPingTime >= SEND_INTERVAL) {
    lastPingTime = currentMillis;

    bool contactsClosed = !digitalRead(CONTACT_PIN); // Чтение состояния контактов
    String message = contactsClosed ? STR_CONTACTS_CLOSED : STR_CONTACTS_OPEN;
    Serial.println("[" + String(currentMillis) + "] Sending: " + message);

    LoRa.beginPacket(); // Начало передачи пакета
    LoRa.print(message); // Отправка сообщения
    LoRa.endPacket();   // Завершение передачи
    delay(10);          // Задержка для стабильности LoRa

    digitalWrite(LED_PIN, HIGH); // Включение LED
    delay(10);                   // Короткий импульс
    digitalWrite(LED_PIN, LOW);  // Выключение LED

    // Обновление дисплея при изменении состояния или по интервалу
    if (message != lastState || (currentMillis - lastDisplayTime >= DISPLAY_INTERVAL)) {
      updateDisplay(contactsClosed);
      if (message != lastState) {
        lastState = message;
        lastStateChangeTime = currentMillis;
        Serial.println("[" + String(currentMillis) + "] State changed");
      }
      lastDisplayTime = currentMillis;
    }
  }

  checkResponse(); // Обработка входящих PING-сообщений

  // Переход в глубокий сон при отсутствии активности
  if (currentMillis - lastStateChangeTime >= SLEEP_TIMEOUT) {
    Serial.println("[" + String(currentMillis) + "] Entering deep sleep...");
    display.clearDisplay();
    display.display();
    digitalWrite(LED_PIN, LOW);
    esp_deep_sleep_start();
  }
}

// Обработка входящих PING-сообщений от приёмника
void checkResponse() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String response = "";
    while (LoRa.available()) {
      response += (char)LoRa.read();
    }
    if (response.startsWith("PING")) {
      int rssi = LoRa.packetRssi();
      LoRa.beginPacket();
      LoRa.print("PONG " + String(rssi));
      LoRa.endPacket();
      Serial.println("[" + String(millis()) + "] Received PING, Sent PONG with RSSI: " + String(rssi));
    }
  }
}

// Обновление дисплея с текущим состоянием
void updateDisplay(bool contactsClosed) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  // Заголовок и версия
  display.setCursor(0, 0);
  display.println(STR_SENDER);
  display.setCursor(100, 0);
  display.println(STR_VERSION);

  // Разделительная линия
  display.drawLine(0, 8, SCREEN_WIDTH - 1, 8, WHITE);

  // Состояние контактов
  display.setCursor(120, 0);
  display.println((millis() % 1000 < 500) ? "*" : " ");
  if (contactsClosed) {
    display.setCursor(40, 20);
    display.println("---------");
    display.setCursor(56, 30);
    display.println(STR_OK);
    display.setCursor(0, 20);
    display.println(STR_CLOSED);
  } else {
    display.setCursor(40, 20);
    display.println("--- X ---");
    display.setCursor(0, 20);
    display.println(STR_OPEN);
  }

  displayUptime(millis());

  // Предупреждение о сне
  unsigned long timeToSleep = SLEEP_TIMEOUT - (millis() - lastStateChangeTime);
  if (timeToSleep <= SLEEP_WARNING) {
    display.setCursor(0, 40);
    display.print("Sleeping in ");
    display.print(timeToSleep / 1000);
    display.println("s");
  }

  display.display();
}

// Отображение времени работы
void displayUptime(unsigned long millis) {
  unsigned long seconds = millis / 1000;
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;
  display.setCursor(0, 50);
  display.print(STR_UPTIME);
  if (hours < 10) display.print("0");
  display.print(hours);
  display.print(":");
  if (minutes < 10) display.print("0");
  display.print(minutes);
  display.print(":");
  if (secs < 10) display.print("0");
  display.print(secs);
}

// Показ стартового экрана
void showStartupScreen() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println("RS-Expert Oy");
  display.setTextSize(1);
  display.setCursor(100, 40);
  display.println(STR_VERSION);
  display.display();
  delay(2000);
}

// Самодиагностика оборудования
void checkHardware() {
  Serial.println("Hardware check:");
  Serial.println("- OLED: OK"); // Дисплей уже проверен
  Serial.print("- LoRa: ");
  if (LoRa.begin(BAND)) {
    Serial.println("OK");
  } else {
    Serial.println("FAIL");
  }
}
