// Передатчик (Sender) - v3.0
// Устройство для отправки состояния контактов через LoRa с режимом сна
// Изменение #1: 2025-03-05 23:00 - Инвертирована логика OPEN/CLOSE, добавлен глубокий сон через GPIO 0 (60 минут или 3 секунды нажатия)
// Изменение #2: 2025-03-06 10:00 - STATE_PIN 32 для состояния, CONTACT_PIN 0 для сна
// Изменение #3: 2025-03-06 12:00 - Убрана отладка Serial, добавлено пробуждение от GPIO 32 (EXT1) и GPIO 0 (EXT0)
// Изменение #4: 2025-03-06 14:00 - LONG_PRESS_TIME изменён на 1000 мс
// Изменение #5: 2025-03-06 22:00 - Убрана мигающая звёздочка в updateDisplay
// Изменение #6: 2025-03-07 12:00 - Добавлено сообщение SHUTDOWN перед глубоким сном
// Изменение #7: 2025-03-08 18:00 - SEND_INTERVAL и DISPLAY_INTERVAL изменены на 300 мс
// Изменение #8: 2025-03-09 20:00 - Заставка "RS-Expert" вместо "RS-Expert Oy", длительность 1 с
// Изменение #9: 2025-03-11 14:00 - Версия обновлена до v3.0

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
#define STR_UPTIME "Uptime: "
#define STR_CONTACT_CLOSED "Contacts CLOSED"
#define STR_CONTACT_OPEN "Contacts OPEN"
#define STR_TX "TX: "
#define STR_LAST_CHANGE "Change: "
#define STR_NEVER "Never"
#define STR_VERSION "v3.0" // Обновлено до v3.0
#define STR_ERROR "ERROR"
#define STR_SHUTDOWN "SHUTDOWN"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

String lastState = "";
unsigned long lastPingTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastStateChangeTime = 0;
int senderRssi = -1;
bool stateChanged = false;

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
        stateChanged = true;
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
      senderRssi = rssi;
      LoRa.beginPacket();
      LoRa.print("PONG " + String(rssi));
      LoRa.endPacket();
    }
  }
}

void updateDisplay(bool contactsClosed) {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Мигающая точка (активность, на 0,0)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print((millis() % 1000 < 500) ? "." : " ");

  // Заголовок "Sender" (сдвинут правее из-за точки)
  display.setCursor(6, 0);
  display.println(STR_SENDER);

  // TX (в верхней строке справа)
  int senderPercent = (senderRssi == -1) ? -1 : constrain(((senderRssi + 120) * 100) / 90, 0, 100);
  display.setCursor(80, 0);
  display.print(STR_TX);
  if (senderPercent == -1) display.println("N/A");
  else display.println(String(senderPercent) + "%");

  // Линия-разделитель
  display.drawLine(0, 8, SCREEN_WIDTH - 1, 8, WHITE);

  // Состояние контакта (размер 1)
  display.setTextSize(1);
  display.setCursor(0, 20);
  if (contactsClosed) {
    display.println(STR_CLOSED);
    display.setCursor(40, 20);
    display.println("---------");
  } else {
    display.println(STR_OPEN);
    display.setCursor(40, 20);
    display.println("--- X ---");
  }

  // Время последнего изменения (сокращено до "Change: MM:SS")
  display.setCursor(0, 40);
  display.print(STR_LAST_CHANGE);
  if (!stateChanged) {
    display.println(STR_NEVER);
  } else {
    unsigned long timeElapsed = (millis() - lastStateChangeTime) / 1000;
    int minutes = timeElapsed / 60;
    int seconds = timeElapsed % 60;
    if (minutes < 10) display.print("0");
    display.print(minutes);
    display.print(":");
    if (seconds < 10) display.print("0");
    display.print(seconds);
  }

  // Uptime (размер 1, внизу)
  displayUptime(millis);

  display.display();
}

void displayUptime(unsigned long millis) {
  unsigned long seconds = millis / 1000;
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;
  display.setTextSize(1);
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
  display.println("RS-Expert");
  display.setTextSize(1);
  display.setCursor(100, 40);
  display.println(STR_VERSION);
  display.display();
  delay(1000);
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
