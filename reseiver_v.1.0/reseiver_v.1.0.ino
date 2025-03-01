// Приёмник (Receiver) - Версия v1.0
// Устройство для приёма состояния контактов через LoRa с режимом сна и звуковой индикацией

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
#define PRG_PIN 0   // Кнопка PRG для отображения лога и пробуждения
#define BUZZER_PIN 13 // Buzzer для звуковой индикации

#define BAND 868E6
#define PING_INTERVAL 2000  // Интервал отправки PING (мс)
#define SIGNAL_TIMEOUT 5000 // Тайм-аут сигнала (мс)
#define DISPLAY_INTERVAL 50 // Обновление дисплея (мс)
#define LOG_SIZE 6  // Размер лога (записей)
#define SLEEP_TIMEOUT 1800000 // Время до сна (мс, 30 минут)
#define SLEEP_WARNING 10000   // Предупреждение до сна (мс, 10 с)
#define LONG_PRESS_TIME 1000  // Время долгого нажатия PRG для сброса лога (мс)
#define EEPROM_SIZE (LOG_SIZE * (sizeof(unsigned long) + 8)) // Размер EEPROM для лога

// Константы строк
#define STR_RECEIVER "Receiver"
#define STR_CLOSED "CLOSED"
#define STR_OPEN "OPEN"
#define STR_OK "OK"
#define STR_UPTIME "Uptime: "
#define STR_NO_SIGNAL "NO SIGNAL"
#define STR_LOG "Log (last 6):"
#define STR_CONTACTS_CLOSED "Contacts CLOSED"
#define STR_CONTACTS_OPEN "Contacts OPEN"
#define STR_TX "TX: "
#define STR_RX "RX: "
#define STR_VERSION "v1.0"
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

void displayUptime(unsigned long millis);
void showStartupScreen();
void checkHardware();
void saveLogToEEPROM();
void loadLogFromEEPROM();
void resetLog();

void setup() {
  Serial.begin(115200);
  delay(100);

  // Настройка пинов
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
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }

  showStartupScreen();

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(BAND)) {
    Serial.println(F("LoRa init failed! Retrying..."));
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

  EEPROM.begin(EEPROM_SIZE); // Инициализация EEPROM
  loadLogFromEEPROM();       // Загрузка лога из EEPROM

  checkHardware(); // Самодиагностика

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(STR_RECEIVER);
  display.display();
  Serial.println("Receiver setup completed");

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
  lastSignalTime = millis();
}

// Основной цикл
void loop() {
  unsigned long currentMillis = millis();

  // Отправка PING каждые 2 секунды
  if (currentMillis - lastPingTime >= PING_INTERVAL) {
    lastPingTime = currentMillis;
    LoRa.beginPacket();
    LoRa.print("PING");
    LoRa.endPacket();
    Serial.println("[" + String(currentMillis) + "] Sent PING from receiver");
  }

  // Обработка кнопки PRG
  if (!digitalRead(PRG_PIN)) {
    unsigned long pressStart = millis();
    while (!digitalRead(PRG_PIN) && (millis() - pressStart < LONG_PRESS_TIME));
    if (millis() - pressStart >= LONG_PRESS_TIME && (currentMillis - lastButtonPress > 200)) {
      resetLog(); // Сброс лога при долгом нажатии
      Serial.println("[" + String(currentMillis) + "] Log reset");
    } else if (currentMillis - lastButtonPress > 200) {
      showLog = !showLog; // Переключение лога при коротком нажатии
      Serial.println("[" + String(currentMillis) + "] PRG button pressed, showLog: " + String(showLog));
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
    delay(10);
    digitalWrite(LED_PIN, LOW);

    Serial.println("[" + String(currentMillis) + "] Raw received: " + receivedMessage);

    if (receivedMessage.startsWith("PONG")) {
      receiverRssi = receivedMessage.substring(5).toInt();
      Serial.println("[" + String(currentMillis) + "] Received PONG with RSSI: " + String(receiverRssi));
    } else if (receivedMessage.startsWith("Contacts")) {
      senderRssi = LoRa.packetRssi();
      Serial.println("[" + String(currentMillis) + "] Received: " + receivedMessage + " | Sender RSSI: " + String(senderRssi));

      String newState = (receivedMessage == STR_CONTACTS_CLOSED) ? STR_CLOSED : STR_OPEN;
      if (newState != lastReceivedState && receivedMessage.startsWith("Contacts")) {
        stateLog[logIndex].state = newState;
        stateLog[logIndex].timestamp = currentMillis;
        logIndex = (logIndex + 1) % LOG_SIZE;
        if (logCount < LOG_SIZE) logCount++;
        saveLogToEEPROM(); // Сохранение лога в EEPROM
        lastReceivedState = newState;
        Serial.println("[" + String(currentMillis) + "] StateLog updated: " + newState);
        if (newState == STR_CLOSED) {
          digitalWrite(BUZZER_PIN, HIGH);
          delay(100);
          digitalWrite(BUZZER_PIN, LOW);
        }
      }
    } else {
      Serial.println("[" + String(currentMillis) + "] Unknown message: " + receivedMessage);
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
      updateDisplay(receivedMessage);
    }
    lastDisplayTime = currentMillis;
  }

  // Переход в глубокий сон
  if (currentMillis - lastSignalTime >= SLEEP_TIMEOUT) {
    Serial.println("[" + String(currentMillis) + "] Entering deep sleep...");
    display.clearDisplay();
    display.display();
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    esp_deep_sleep_start();
  }
}

// Обновление основного экрана
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

  if (message == STR_CONTACTS_CLOSED) {
    display.setCursor(40, 20);
    display.println("---------");
    display.setCursor(56, 30);
    display.println(STR_OK);
    display.setCursor(0, 20);
    display.println(STR_CLOSED);
  } else if (message == STR_CONTACTS_OPEN) {
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

// Отображение лога
void displayLog(unsigned long currentMillis) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(STR_LOG);

  int startIndex = (logCount < LOG_SIZE) ? 0 : logIndex;
  for (int i = 0; i < min(logCount, LOG_SIZE); i++) {
    int idx = (startIndex + i) % LOG_SIZE;
    unsigned long timeElapsed = (currentMillis >= stateLog[idx].timestamp) ? (currentMillis - stateLog[idx].timestamp) / 1000 : 0;
    display.setCursor(0, 10 + i * 8);
    display.print(stateLog[idx].state);
    display.print(" ");
    display.print(timeElapsed);
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
  Serial.println("- OLED: OK");
  Serial.print("- LoRa: ");
  if (LoRa.begin(BAND)) {
    Serial.println("OK");
  } else {
    Serial.println("FAIL");
  }
  Serial.print("- Buzzer: ");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("OK");
}

// Сохранение лога в EEPROM
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

// Загрузка лога из EEPROM
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

// Сброс лога
void resetLog() {
  for (int i = 0; i < LOG_SIZE; i++) {
    stateLog[i].state = "N/A";
    stateLog[i].timestamp = 0;
  }
  logIndex = 0;
  logCount = 0;
  saveLogToEEPROM();
}
