// Приёмник (Receiver) - Версия v2.0b
// Устройство для приёма состояния одного контакта через LoRa с режимом сна и звуковой индикацией

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCK     5
#define MISO    19
#define MOSI    27
#define SS      18
#define RST     14
#define DIO0    26

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define LED_PIN 2
#define PRG_PIN 0
#define BUZZER_PIN 13

#define BAND 868E6
#define PING_INTERVAL 2000  // Возвращаем 2000 мс для снижения нагрузки
#define SIGNAL_TIMEOUT 5000
#define DISPLAY_INTERVAL 10 // Ускоряем до 10 мс для мгновенной реакции
#define LOG_SIZE 6
#define SLEEP_TIMEOUT 1800000
#define SLEEP_WARNING 10000
#define LONG_PRESS_TIME 5000
#define LOSS_TIMEOUT 10000
#define EEPROM_SIZE (LOG_SIZE * (sizeof(unsigned long) + 8))

#define STR_RECEIVER "Receiver"
#define STR_CLOSED "CLOSED"
#define STR_OPEN "OPEN"
#define STR_OK "OK"
#define STR_UPTIME "Uptime: "
#define STR_NO_SIGNAL "NO SIGNAL"
#define STR_LOG "Log (last 6):"
#define STR_CONTACT_CLOSED "CLOSED"
#define STR_CONTACT_OPEN "OPEN"
#define STR_TX "TX: "
#define STR_RX "RX: "
#define STR_VERSION "v2.0b"
#define STR_ERROR "ERROR"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

struct LogEntry {
  String state;
  unsigned long timestamp;
};

LogEntry stateLog[LOG_SIZE];
int logIndex = 0;
int logCount = 0;

String receivedMessage = "";
String lastReceivedState = "";
int senderRssi = -1;
int receiverRssi = -1;
unsigned long lastPingTime = 0;
unsigned long lastSignalTime = 0;
unsigned long lastDisplayTime = 0;
bool showLog = false;
unsigned long lastButtonPress = 0;
bool contactState = false;

void displayUptime(unsigned long millis);
void showStartupScreen();
void saveLogToEEPROM();
void loadLogFromEEPROM();
void resetLog();
void displayTime(unsigned long millis);
void initLoRa();

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(PRG_PIN, INPUT_PULLUP);
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
  delay(100); // Дополнительная задержка для стабилизации дисплея
  display.clearDisplay();
  display.display();

  showStartupScreen();

  SPI.begin(SCK, MISO, MOSI, SS);
  initLoRa();

  EEPROM.begin(EEPROM_SIZE);
  loadLogFromEEPROM();

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

  // Отправка PING каждые 2 секунды
  if (currentMillis - lastPingTime >= PING_INTERVAL) {
    lastPingTime = currentMillis;
    LoRa.beginPacket();
    LoRa.print("PING");
    LoRa.endPacket();
  }

  // Обработка кнопки PRG
  if (!digitalRead(PRG_PIN)) {
    unsigned long pressStart = millis();
    while (!digitalRead(PRG_PIN) && (millis() - pressStart < LONG_PRESS_TIME));
    if (millis() - pressStart >= LONG_PRESS_TIME && (currentMillis - lastButtonPress > 200)) {
      resetLog();
    } else if (currentMillis - lastButtonPress > 200) {
      showLog = !showLog;
    }
    lastButtonPress = currentMillis;
  }

  // Обработка входящих пакетов
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);

    if (receivedMessage == "PONG") {
      receiverRssi = LoRa.packetRssi();
    } else {
      senderRssi = LoRa.packetRssi();
      contactState = (receivedMessage == STR_CONTACT_CLOSED);

      if (receivedMessage != lastReceivedState) {
        stateLog[logIndex].state = receivedMessage;
        stateLog[logIndex].timestamp = currentMillis;
        logIndex = (logIndex + 1) % LOG_SIZE;
        if (logCount < LOG_SIZE) logCount++;
        saveLogToEEPROM();
        lastReceivedState = receivedMessage;
        if (contactState) {
          digitalWrite(BUZZER_PIN, HIGH);
          delay(10); // Уменьшаем задержку до 10 мс
          digitalWrite(BUZZER_PIN, LOW);
        }
      }
    }

    lastSignalTime = currentMillis;
  }

  // Обновление дисплея
  if (currentMillis - lastDisplayTime >= DISPLAY_INTERVAL) {
    if (currentMillis - lastSignalTime > SIGNAL_TIMEOUT && !showLog) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(STR_RECEIVER);
      display.setCursor(0, 20);
      display.println(STR_NO_SIGNAL);
      displayUptime(currentMillis);
      display.display();
    } else if (showLog) {
      displayLog(currentMillis);
    } else {
      updateDisplay();
    }
    lastDisplayTime = currentMillis;
  }

  // Переход в глубокий сон
  if (currentMillis - lastSignalTime >= SLEEP_TIMEOUT) {
    display.clearDisplay();
    display.display();
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    esp_deep_sleep_start();
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(STR_RECEIVER);
  display.setCursor(90, 0); // Версия на (90, 0)
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

  display.setCursor(0, 20);
  display.print("Contact: ");
  if (contactState) {
    display.println(STR_CLOSED);
    display.setCursor(40, 30);
    display.println("---------");
    display.setCursor(56, 40);
    display.println(STR_OK);
  } else {
    display.println(STR_OPEN);
    display.setCursor(40, 30);
    display.println("--- X ---");
  }

  displayUptime(millis());

  unsigned long timeToSleep = SLEEP_TIMEOUT - (millis() - lastSignalTime);
  if (timeToSleep <= SLEEP_WARNING) {
    display.setCursor(0, 50);
    display.print("Sleeping in ");
    display.print(timeToSleep / 1000);
    display.println("s");
  }

  display.display();
}

void displayLog(unsigned long currentMillis) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(STR_LOG);

  int startIndex = (logCount < LOG_SIZE) ? 0 : logIndex;
  for (int i = 0; i < min(logCount, LOG_SIZE); i++) {
    int idx = (startIndex + i) % LOG_SIZE;
    display.setCursor(0, 10 + i * 8);
    display.print(i + 1); // Номер записи
    display.print(": ");
    display.print(stateLog[idx].state); // Без "Contact"
    display.print(" ");
    displayTime(stateLog[idx].timestamp);
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
  display.setCursor(90, 40);
  display.println(STR_VERSION);
  display.display();
  delay(3000); // Удерживаем заставку 3 секунды
}

void saveLogToEEPROM() {
  int address = 0;
  for (int i = 0; i < LOG_SIZE; i++) {
    EEPROM.put(address, stateLog[i].timestamp);
    address += sizeof(unsigned long);
    String state = stateLog[i].state;
    for (int j = 0; j < 8; j++) {
      char c = (j < state.length()) ? state[j] : '\0';
      EEPROM.write(address + j, c);
    }
    address += 8;
  }
  EEPROM.commit();
}

void loadLogFromEEPROM() {
  int address = 0;
  for (int i = 0; i < LOG_SIZE; i++) {
    EEPROM.get(address, stateLog[i].timestamp);
    address += sizeof(unsigned long);
    char state[9];
    for (int j = 0; j < 8; j++) {
      state[j] = EEPROM.read(address + j);
    }
    state[8] = '\0';
    stateLog[i].state = String(state);
    address += 8;
    if (stateLog[i].timestamp > 0 && stateLog[i].state != "N/A") {
      logCount++;
      logIndex = (logIndex + 1) % LOG_SIZE;
    }
  }
  if (logCount > 0) {
    logIndex = logCount % LOG_SIZE;
    lastReceivedState = stateLog[(logIndex - 1 + LOG_SIZE) % LOG_SIZE].state;
  }
}

void resetLog() {
  for (int i = 0; i < LOG_SIZE; i++) {
    stateLog[i].state = "N/A";
    stateLog[i].timestamp = 0;
  }
  logIndex = 0;
  logCount = 0;
  saveLogToEEPROM();
}

void displayTime(unsigned long millis) {
  unsigned long seconds = millis / 1000;
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;
  if (hours < 10) display.print("0");
  display.print(hours);
  display.print(":");
  if (minutes < 10) display.print("0");
  display.print(minutes);
  display.print(":");
  if (secs < 10) display.print("0");
  display.print(secs);
}

void initLoRa() {
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
}
