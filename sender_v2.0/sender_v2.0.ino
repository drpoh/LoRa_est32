// Передатчик (Sender) - Версия v2.0 (обновлённая)
// Устройство для отправки состояния контактов через LoRa с режимом сна
// Изменение #1: 2025-03-05 23:00 - Инвертирована логика OPEN/CLOSE, добавлен глубокий сон через GPIO 0 (60 минут или 3 секунды нажатия)
// Изменение #2: 2025-03-06 10:00 - STATE_PIN 32 для состояния, CONTACT_PIN 0 для сна
// Изменение #3: 2025-03-06 12:00 - Убрана отладка Serial, добавлено пробуждение от GPIO 32 (EXT1) и GPIO 0 (EXT0)
// Изменение #4: 2025-03-06 14:00 - LONG_PRESS_TIME изменён на 1000 мс
// Изменение #5: 2025-03-06 22:00 - Убрана мигающая звёздочка в updateDisplay
// Изменение #6: 2025-03-07 12:00 - Добавлено сообщение SHUTDOWN перед глубоким сном
// Изменение #7: 2025-03-08 18:00 - SEND_INTERVAL и DISPLAY_INTERVAL изменены на 300 мс
// Изменение #8: 2025-03-08 22:00 - Добавлены TX и мигающая точка

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
#define STATE_PIN 32    // Проверка состояния контакта
#define CONTACT_PIN 0   // Для глубокого сна
#define LED_PIN 2       // LED для индикации активности

// Константы конфигурации
#define BAND 868E6          // Частота LoRa (868 МГц)
#define SEND_INTERVAL 300   // Интервал проверки и отправки данных (мс)
#define DISPLAY_INTERVAL 300 // Интервал обновления дисплея (мс)
#define SLEEP_TIMEOUT 3600000 // Время до перехода в сон (мс, 60 минут)
#define SLEEP_WARNING 10000   // Время до сна для предупреждения (мс, 10 с)
#define LONG_PRESS_TIME 1000  // 1 секунда для выключения
#define SCREEN_WIDTH 128      // Ширина дисплея
#define SCREEN_HEIGHT 64      // Высота дисплея

#define STR_SENDER "Sender"
#define STR_CLOSED "CLOSED"
#define STR_OPEN "OPEN"
#define STR_OK "OK"
#define STR_UPTIME "Uptime: "
#define STR_CONTACT_CLOSED "Contacts CLOSED"
#define STR_CONTACT_OPEN "Contacts OPEN"
#define STR_TX "TX: "         // Изменение #8
#define STR_VERSION "v2.0"
#define STR_ERROR "ERROR"
#define STR_SHUTDOWN "SHUTDOWN"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

String lastState = "";
unsigned long lastPingTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastStateChangeTime = 0;
int senderRssi = -1; // Изменение #8: RSSI передатчика

void displayUptime(unsigned long millis);
void showStartupScreen();
void checkHardware();

void setup() {
  pinMode(STATE_PIN, INPUT_PULLUP);
  pinMode(CONTACT_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  showStartupScreen();

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(BAND)) {
    delay(1000);
    if (!LoRa.begin(BAND)) {
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println(STR_ERROR);
      display.display();
      while (1);
    }
  }

  LoRa.setTxPower(20);
  LoRa.setSpreadingFactor(12);

  checkHardware();

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(STR_SENDER);
  display.display();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
  esp_sleep_enable_ext1_wakeup((1ULL << STATE_PIN), ESP_EXT1_WAKEUP_ANY_HIGH);
  lastStateChangeTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  if (!digitalRead(CONTACT_PIN)) {
    unsigned long pressStart = millis();
    while (!digitalRead(CONTACT_PIN) && (millis() - pressStart < LONG_PRESS_TIME));
    if (millis() - pressStart >= LONG_PRESS_TIME) {
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println("Shutting down...");
      display.display();
      delay(1000);
      digitalWrite(LED_PIN, LOW);
      LoRa.beginPacket();
      LoRa.print(STR_SHUTDOWN);
      LoRa.endPacket();
      delay(10);
      esp_deep_sleep_start();
    }
  }

  if (currentMillis - lastPingTime >= SEND_INTERVAL) {
    lastPingTime = currentMillis;

    bool contactsClosed = digitalRead(STATE_PIN);
    String message = contactsClosed ? STR_CONTACT_CLOSED : STR_CONTACT_OPEN;

    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
    delay(5);

    digitalWrite(LED_PIN, HIGH);
    delay(5);
    digitalWrite(LED_PIN, LOW);

    if (message != lastState || (currentMillis - lastDisplayTime >= DISPLAY_INTERVAL)) {
      updateDisplay(contactsClosed);
      if (message != lastState) {
        lastState = message;
        lastStateChangeTime = currentMillis;
      }
      lastDisplayTime = currentMillis;
    }
  }

  checkResponse();

  if (currentMillis - lastStateChangeTime >= SLEEP_TIMEOUT) {
    display.clearDisplay();
    display.display();
    digitalWrite(LED_PIN, LOW);
    LoRa.beginPacket();
    LoRa.print(STR_SHUTDOWN);
    LoRa.endPacket();
    delay(10);
    esp_deep_sleep_start();
  }
}

void checkResponse() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String response = "";
    while (LoRa.available()) {
      response += (char)LoRa.read();
    }
    if (response.startsWith("PING")) {
      int rssi = LoRa.packetRssi();
      senderRssi = rssi; // Изменение #8: RSSI передатчика
      LoRa.beginPacket();
      LoRa.print("PONG " + String(rssi));
      LoRa.endPacket();
    }
  }
}

void updateDisplay(bool contactsClosed) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(STR_SENDER);
  display.setCursor(100, 0);
  display.println(STR_VERSION);

  display.drawLine(0, 8, SCREEN_WIDTH - 1, 8, WHITE);

  int senderPercent = (senderRssi == -1) ? -1 : constrain(((senderRssi + 120) * 100) / 90, 0, 100); // Изменение #8
  display.setCursor(0, 10);
  display.print(STR_TX);
  if (senderPercent == -1) display.println("N/A");
  else display.println(String(senderPercent) + "%");

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

  unsigned long timeToSleep = SLEEP_TIMEOUT - (millis() - lastStateChangeTime);
  if (timeToSleep <= SLEEP_WARNING) {
    display.setCursor(0, 40);
    display.print("Sleeping in ");
    display.print(timeToSleep / 1000);
    display.println("s");
  }

  display.setCursor(120, 10); // Изменение #8: Мигающая точка
  display.println((millis() % 1000 < 500) ? "." : " ");
  display.display();
}

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

void checkHardware() {
  if (!LoRa.begin(BAND)) {
    delay(1000);
    if (!LoRa.begin(BAND)) {
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println(STR_ERROR);
      display.display();
      while (1);
    }
  }
}
