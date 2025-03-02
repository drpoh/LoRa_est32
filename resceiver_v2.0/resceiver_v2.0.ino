// Приёмник (Receiver) - Версия v2.0
// Устройство для приёма состояния контактов через LoRa с режимом сна
// Изменение #1: 2025-03-06 10:00 - STATE_PIN 32 для состояния, CONTACT_PIN 0 для сна, инвертирована логика OPEN/CLOSE
// Изменение #2: 2025-03-06 16:00 - Добавлен трёхтональный сигнал зуммера при состоянии CLOSED
// Изменение #3: 2025-03-06 18:00 - Исправлена опечатка STR_CONTACTS_OPEN на STR_CONTACT_OPEN
// Изменение #4: 2025-03-06 20:00 - Зуммер пищит непрерывно при CLOSED с более быстрым и громким сигналом
// Изменение #5: 2025-03-06 22:00 - Частоты зуммера изменены на 1400 Гц, 1700 Гц, 2000 Гц
// Изменение #6: 2025-03-07 12:00 - Трёхтональные сигналы включения/выключения, обработка SHUTDOWN от Sender
// Изменение #7: 2025-03-07 20:00 - Звуки вкл/выкл обновлены (100 мс)
// Изменение #8: 2025-03-08 10:00 - Добавлено "Sender off" под "NO SIGNAL" при получении SHUTDOWN
// Изменение #9: 2025-03-08 14:00 - Добавлены: время последнего сигнала, RSSI в процентах, анимация активности, разные тона
// Изменение #10: 2025-03-09 12:00 - Убраны events и всё связанное

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
#define CONTACT_PIN 0   // Для глубокого сна
#define BUZZER_PIN 13   // Пин для зуммера
#define LED_PIN 2       // LED для индикации активности

// Константы конфигурации
#define BAND 868E6          // Частота LoRa (868 МГц)
#define PING_INTERVAL 2000  // Интервал отправки PING (мс)
#define SIGNAL_TIMEOUT 5000 // Таймаут сигнала (мс)
#define DISPLAY_INTERVAL 100 // Интервал обновления дисплея (мс)
#define SLEEP_TIMEOUT 3600000 // Время до перехода в сон (мс, 60 минут)
#define SLEEP_WARNING 10000   // Время до сна для предупреждения (мс, 10 с)
#define LONG_PRESS_TIME 1000  // 1 секунда для выключения
#define SCREEN_WIDTH 128      // Ширина дисплея
#define SCREEN_HEIGHT 64      // Высота дисплея

#define STR_RECEIVER "Receiver"
#define STR_CLOSED "CLOSED"
#define STR_OPEN "OPEN"
#define STR_OK "OK"
#define STR_UPTIME "Uptime: "
#define STR_NO_SIGNAL "NO SIGNAL"
#define STR_SENDER_OFF "Sender off"
#define STR_CONTACT_CLOSED "Contacts CLOSED"
#define STR_CONTACT_OPEN "Contacts OPEN"
#define STR_TX "TX: "
#define STR_RX "RX: "
#define STR_LAST_SIGNAL "Last signal: "
#define STR_VERSION "v2.0"
#define STR_ERROR "ERROR"
#define STR_SHUTDOWN "SHUTDOWN"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

String receivedMessage = "";
String lastReceivedState = "";
int senderRssi = -1;
int receiverRssi = -1;
unsigned long lastPingTime = 0;
unsigned long lastSignalTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastButtonPress = 0;
bool buzzerActive = false;
unsigned long lastBuzzerTime = 0;
int buzzerToneIndex = 0;
bool senderOff = false;

void displayUptime(unsigned long millis);
void showStartupScreen();
void checkHardware();
void playContinuousThreeToneBuzzer();
void playStartupSound();
void playClosedSound();
void playShutdownSound();

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(CONTACT_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  showStartupScreen();
  playStartupSound();

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
  display.println(STR_RECEIVER);
  display.display();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
  lastSignalTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastPingTime >= PING_INTERVAL) {
    lastPingTime = currentMillis;
    LoRa.beginPacket();
    LoRa.print("PING");
    LoRa.endPacket();
  }

  if (!digitalRead(CONTACT_PIN)) {
    unsigned long pressStart = millis();
    while (!digitalRead(CONTACT_PIN) && (millis() - pressStart < LONG_PRESS_TIME));
    if (millis() - pressStart >= LONG_PRESS_TIME) {
      digitalWrite(BUZZER_PIN, LOW);
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println("Shutting down...");
      display.display();
      playShutdownSound();
      delay(1000);
      digitalWrite(LED_PIN, LOW);
      esp_deep_sleep_start();
    }
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }

    digitalWrite(LED_PIN, HIGH);
    delay(5);
    digitalWrite(LED_PIN, LOW);

    receiverRssi = LoRa.packetRssi();

    if (receivedMessage.startsWith("PONG")) {
      senderRssi = receivedMessage.substring(5).toInt();
    } else if (receivedMessage.startsWith("Contacts")) {
      String newState = (receivedMessage == STR_CONTACT_CLOSED) ? STR_CLOSED : STR_OPEN;
      if (newState != lastReceivedState && receivedMessage.startsWith("Contacts")) {
        lastReceivedState = newState;

        if (newState == STR_CLOSED) {
          buzzerActive = true;
          playClosedSound();
          senderOff = false;
        } else if (newState == STR_OPEN) {
          digitalWrite(BUZZER_PIN, LOW);
          buzzerActive = false;
          buzzerToneIndex = 0;
          senderOff = false;
        }
      }
    } else if (receivedMessage == STR_SHUTDOWN) {
      digitalWrite(BUZZER_PIN, LOW);
      senderOff = true;
      lastSignalTime = currentMillis;
    }

    lastSignalTime = currentMillis;
  }

  if (buzzerActive) {
    playContinuousThreeToneBuzzer();
  }

  if (currentMillis - lastDisplayTime >= DISPLAY_INTERVAL) {
    if (currentMillis - lastSignalTime > SIGNAL_TIMEOUT) {
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(STR_RECEIVER);
      display.setCursor(0, 10);
      display.println(STR_NO_SIGNAL);
      if (senderOff) {
        display.setCursor(0, 20);
        display.println(STR_SENDER_OFF);
      }
      display.setCursor(0, 30);
      display.print(STR_LAST_SIGNAL);
      unsigned long timeElapsed = (currentMillis - lastSignalTime) / 1000;
      display.print(timeElapsed);
      display.println("s ago");
      display.setCursor(120, 0);
      display.println((currentMillis % 1000 < 500) ? "." : " ");
      displayUptime(currentMillis);
      display.display();
    } else {
      updateDisplay(receivedMessage);
    }
    lastDisplayTime = currentMillis;
  }

  if (currentMillis - lastSignalTime >= SLEEP_TIMEOUT) {
    digitalWrite(BUZZER_PIN, LOW);
    display.clearDisplay();
    display.display();
    playShutdownSound();
    digitalWrite(LED_PIN, LOW);
    esp_deep_sleep_start();
  }
}

void updateDisplay(String message) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(STR_RECEIVER);
  display.setCursor(100, 0);
  display.println(STR_VERSION);

  display.drawLine(0, 8, SCREEN_WIDTH - 1, 8, WHITE);

  int senderPercent = (senderRssi == -1) ? -1 : constrain(((senderRssi + 120) * 100) / 90, 0, 100);
  int receiverPercent = (receiverRssi == -1) ? -1 : constrain(((receiverRssi + 120) * 100) / 90, 0, 100);

  display.setCursor(0, 10);
  display.print(STR_TX);
  if (senderPercent == -1) display.println("N/A");
  else display.println(String(senderPercent) + "%");

  display.setCursor(70, 10);
  display.print(STR_RX);
  if (receiverPercent == -1) display.println("N/A");
  else display.println(String(receiverPercent) + "%");

  if (message == STR_CONTACT_CLOSED) {
    display.setCursor(40, 20);
    display.println("---------");
    display.setCursor(56, 30);
    display.println(STR_OK);
    display.setCursor(0, 20);
    display.println(STR_CLOSED);
  } else if (message == STR_CONTACT_OPEN) {
    display.setCursor(40, 20);
    display.println("--- X ---");
    display.setCursor(0, 20);
    display.println(STR_OPEN);
  }
  displayUptime(millis());

  unsigned long timeToSleep = SLEEP_TIMEOUT - (millis() - lastSignalTime);
  if (timeToSleep <= SLEEP_WARNING) {
    display.setCursor(0, 40);
    display.print("Sleeping in ");
    display.print(timeToSleep / 1000);
    display.println("s");
  }

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

void playContinuousThreeToneBuzzer() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastBuzzerTime >= 80) {
    noTone(BUZZER_PIN);
    switch (buzzerToneIndex) {
      case 0:
        tone(BUZZER_PIN, 1400, 80);
        break;
      case 1:
        tone(BUZZER_PIN, 1700, 80);
        break;
      case 2:
        tone(BUZZER_PIN, 2000, 80);
        break;
    }
    lastBuzzerTime = currentMillis;
    buzzerToneIndex = (buzzerToneIndex + 1) % 3;
  }
}

void playStartupSound() {
  tone(BUZZER_PIN, 800, 100);
  delay(100);
  tone(BUZZER_PIN, 1000, 100);
  delay(100);
  tone(BUZZER_PIN, 1200, 100);
  delay(100);
}

void playClosedSound() {
  tone(BUZZER_PIN, 1500, 100);
  delay(100);
}

void playShutdownSound() {
  tone(BUZZER_PIN, 1200, 100);
  delay(100);
  tone(BUZZER_PIN, 1000, 100);
  delay(100);
  tone(BUZZER_PIN, 800, 100);
  delay(100);
}
