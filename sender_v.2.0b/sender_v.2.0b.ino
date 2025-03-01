// Передатчик (Sender) - Версия v2.0b
// Устройство для отправки состояния одного контакта через LoRa с режимом сна

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCK     5
#define MISO    19
#define MOSI    27
#define SS      18
#define RST     14
#define DIO0    26
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define CONTACT_PIN 0
#define LED_PIN 2

#define BAND 868E6
#define SEND_INTERVAL 10    // Проверка контакта каждые 10 мс
#define DISPLAY_INTERVAL 20 // Обновление дисплея каждые 20 мс
#define SLEEP_TIMEOUT 1800000
#define SLEEP_WARNING 10000
#define PONG_TIMEOUT 10000
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define STR_SENDER "Sender"
#define STR_CLOSED "CLOSED"
#define STR_OPEN "OPEN"
#define STR_OK "OK"
#define STR_UPTIME "Uptime: "
#define STR_CONTACT_CLOSED "CLOSED"
#define STR_CONTACT_OPEN "OPEN"
#define STR_VERSION "v2.0b"
#define STR_ERROR "ERROR"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

String lastState = "";
unsigned long lastPingTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastStateChangeTime = 0;
unsigned long lastPongTime = 0;

void displayUptime(unsigned long millis);
void showStartupScreen();
void initLoRa();

void setup() {
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
  delay(100); // Дополнительная задержка для стабилизации дисплея
  display.clearDisplay();
  display.display();

  showStartupScreen();

  SPI.begin(SCK, MISO, MOSI, SS);
  initLoRa();

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(STR_SENDER);
  display.display();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
  lastStateChangeTime = millis();
  lastPongTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastPingTime >= SEND_INTERVAL) {
    lastPingTime = currentMillis;

    bool contactClosed = !digitalRead(CONTACT_PIN);
    String message = contactClosed ? STR_CONTACT_CLOSED : STR_CONTACT_OPEN;

    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);

    if (message != lastState || (currentMillis - lastDisplayTime >= DISPLAY_INTERVAL)) {
      updateDisplay(contactClosed);
      if (message != lastState) {
        lastState = message;
        lastStateChangeTime = currentMillis;
      }
      lastDisplayTime = currentMillis;
    }
  }

  checkResponse();

  if (currentMillis - lastPongTime >= PONG_TIMEOUT) {
    initLoRa();
    lastPongTime = currentMillis;
  }

  if (currentMillis - lastStateChangeTime >= SLEEP_TIMEOUT) {
    display.clearDisplay();
    display.display();
    digitalWrite(LED_PIN, LOW);
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
      String pongPacket = "PONG " + String(rssi);
      LoRa.beginPacket();
      LoRa.print(pongPacket);
      LoRa.endPacket();
      lastPongTime = millis();
    }
  }
}

void updateDisplay(bool contactClosed) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(STR_SENDER);
  display.setCursor(90, 0); // Версия на (90, 0)
  display.println(STR_VERSION);

  display.drawLine(0, 8, SCREEN_WIDTH - 1, 8, WHITE);

  display.setCursor(0, 20);
  display.print("Contact: ");
  if (contactClosed) {
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

  unsigned long timeToSleep = SLEEP_TIMEOUT - (millis() - lastStateChangeTime);
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
  display.setCursor(90, 40);
  display.println(STR_VERSION);
  display.display();
  delay(3000); // Удерживаем заставку 3 секунды
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
