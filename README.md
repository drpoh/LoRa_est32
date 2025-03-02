# Sender / Receiver User Manual

## Инструкция на русском языке / Ohjeet suomeksi / Instructions in English

---

### Цель проекта / Projektin tavoite / Project Goal

#### Русский
Целью проекта является создание надёжной системы беспроводного мониторинга состояния электрических контактов (например, выключателей, реле или датчиков) с использованием технологии LoRa. Sender (передатчик) фиксирует состояние контакта и передаёт его на Receiver (приёмник), который отображает статус и оповещает звуком при замыкании контакта. Это позволяет электрикам удалённо контролировать цепи, снижая необходимость постоянного физического присутствия у оборудования, и упрощает диагностику в сложных или распределённых системах.

#### Suomi
Projektin tavoitteena on luoda luotettava langaton järjestelmä sähköisten kontaktien tilan (esim. kytkimet, releet tai anturit) seurantaan LoRa-teknologian avulla. Sender (lähetin) havaitsee kontaktin tilan ja lähettää sen Receiverille (vastaanotin), joka näyttää tilan ja ilmoittaa äänimerkillä kontaktin sulkeutuessa. Tämä mahdollistaa sähköasentajille piirien etäseurannan, vähentäen tarvetta jatkuvalle fyysiselle läsnäololle laitteiden luona ja helpottaen vianmääritystä monimutkaisissa tai hajautetuissa järjestelmissä.

#### English
The goal of the project is to create a reliable wireless system for monitoring the state of electrical contacts (e.g., switches, relays, or sensors) using LoRa technology. The Sender (transmitter) detects the contact state and transmits it to the Receiver, which displays the status and alerts with a sound when the contact is closed. This allows electricians to remotely monitor circuits, reducing the need for constant physical presence at the equipment and simplifying diagnostics in complex or distributed systems.

---

### Логика использования приборов электриком / Sähköasentajan laitteiden käyttölogiikka / Logic of Using the Devices by an Electrician

#### Русский
##### Применение в работе электрика
Электрик может использовать Sender и Receiver для следующих задач:  
- **Мониторинг состояния оборудования:**  
  - Подключение Sender к контактам выключателя, реле или датчика (например, концевого выключателя на воротах или кнопки аварийного останова).  
  - Receiver размещается в удобном месте (например, в диспетчерской или у электрика в руках) для наблюдения за состоянием.  
  - Пример: Контроль включения/выключения насоса в системе водоснабжения без необходимости подходить к насосу.  
- **Диагностика цепей:**  
  - Sender помогает определить, замкнута ли цепь (например, для проверки исправности проводки или реле).  
  - Receiver сигнализирует звуком при замыкании, что упрощает поиск неисправностей в цепи.  
- **Удалённый контроль:**  
  - Использование в больших зданиях или на объектах с распределённым оборудованием (например, склады, фермы), где проверка каждого контакта вручную занимает много времени.  
  - Дальность до 1-2 км позволяет контролировать состояние без прокладки длинных кабелей.  

##### Логика работы электрика с приборами
1. **Подготовка Sender:**  
   - Подключите контакт (например, выключатель) к GPIO 32:  
     - Замкнутый контакт (подключён к 3.3V) = "CLOSED".  
     - Разомкнутый контакт (подключён к GND) = "OPEN".  
   - Подключите батарею 3.7V (например, 750 мАч Li-Ion) к Sender.  
   - Включите устройство: на экране появится заставка "RS-Expert" и "v2.0" на 1 секунду.  
2. **Подготовка Receiver:**  
   - Подключите батарею 3.7V к Receiver.  
   - Включите устройство: появится заставка "RS-Expert" и "v2.0" на 1 секунду, затем трёхтональный звук (800 Hz, 1000 Hz, 1200 Hz).  
   - Разместите Receiver в зоне видимости (до 1-2 км от Sender в условиях прямой видимости).  
3. **Проверка работы:**  
   - Замкните контакт на Sender (например, включите выключатель):  
     - Sender отобразит "Sender", "CLOSED" и "TX: X%".  
     - Receiver покажет "Receiver", "CLOSED", "RX: X%" и "TX: X%", и начнёт издавать непрерывный трёхтональный звук (1400 Hz, 1700 Hz, 2000 Hz).  
   - Разомкните контакт:  
     - Sender отобразит "OPEN".  
     - Receiver покажет "OPEN", звук выключится.  
   - Если Sender выключен или вне зоны действия (через 5 секунд):  
     - Receiver покажет "NO SIGNAL" и "Last signal: Xs ago".  
     - При получении "SHUTDOWN" добавится "Sender off".  
4. **Отключение:**  
   - Для выключения Sender или Receiver нажмите и держите кнопку на GPIO 0 (1 секунда):  
     - Sender: "Shutting down..." и глубокий сон.  
     - Receiver: "Shutting down...", трёхтональный звук (1200 Hz, 1000 Hz, 800 Hz) и глубокий сон.  
   - Или устройства автоматически выключатся через 60 минут без активности.  

##### Пример сценария
Электрик устанавливает Sender на выключатель освещения в подвале здания, а Receiver берёт с собой в офис на первом этаже. Когда кто-то включает свет (замыкает контакт), Receiver показывает "CLOSED" и издаёт звук, уведомляя электрика об активности без необходимости спускаться в подвал.

#### Suomi
##### Sähköasentajan käyttökohteet
Sähköasentaja voi käyttää Sender- ja Receiver-laitteita seuraaviin tehtäviin:  
- **Laitteiden tilan seuranta:**  
  - Sender liitetään kytkimen, releen tai anturin (esim. portin rajakytkin tai hätäpysäytyspainike) kontakteihin.  
  - Receiver sijoitetaan kätevään paikkaan (esim. valvomo tai sähköasentajan käsiin) tilan tarkkailua varten.  
  - Esimerkki: Vesihuoltojärjestelmän pumpun päälle/pois-tilan valvonta ilman tarvetta mennä pumpun luo.  
- **Piirien diagnosointi:**  
  - Sender auttaa selvittämään, onko piiri suljettu (esim. johtojen tai releiden toimintakunnon tarkistus).  
  - Receiver antaa äänimerkin sulkeutuessa, mikä helpottaa vikojen paikantamista piirissä.  
- **Etävalvonta:**  
  - Käyttö suurissa rakennuksissa tai hajautetuissa laitteistoissa (esim. varastot, maatilat), joissa jokaisen kontaktin manuaalinen tarkistaminen vie aikaa.  
  - Jopa 1-2 km:n kantama mahdollistaa tilan valvonnan ilman pitkiä kaapeleita.  

##### Sähköasentajan työskentelylogiikka laitteiden kanssa
1. **Senderin valmistelu:**  
   - Liitä kontakti (esim. kytkin) GPIO 32:een:  
     - Suljettu kontakti (liitetty 3.3V:hen) = "CLOSED".  
     - Avoin kontakti (liitetty GND:hen) = "OPEN".  
   - Kytke 3.7V akku (esim. 750 mAh Li-Ion) Senderiin.  
   - Käynnistä laite: näytöllä näkyy "RS-Expert" ja "v2.0" 1 sekunnin ajan.  
2. **Receiverin valmistelu:**  
   - Kytke 3.7V akku Receiveriin.  
   - Käynnistä laite: näytöllä näkyy "RS-Expert" ja "v2.0" 1 sekunnin ajan, sitten kolmisävelinen ääni (800 Hz, 1000 Hz, 1200 Hz).  
   - Sijoita Receiver näkyvyysalueelle (1-2 km Senderistä suorassa näköyhteydessä).  
3. **Toiminnan tarkistus:**  
   - Sulje kontakti Senderissä (esim. kytke kytkin päälle):  
     - Sender näyttää "Sender", "CLOSED" ja "TX: X%".  
     - Receiver näyttää "Receiver", "CLOSED", "RX: X%" ja "TX: X%", ja alkaa tuottaa jatkuvaa kolmisävelistä ääntä (1400 Hz, 1700 Hz, 2000 Hz).  
   - Avaa kontakti:  
     - Sender näyttää "OPEN".  
     - Receiver näyttää "OPEN", ääni sammuu.  
   - Jos Sender sammutetaan tai on kantaman ulkopuolella (5 sekunnin kuluttua):  
     - Receiver näyttää "NO SIGNAL" ja "Last signal: Xs ago".  
     - "SHUTDOWN"-viestin saapuessa lisätään "Sender off".  
4. **Sammutus:**  
   - Sammuta Sender tai Receiver pitämällä GPIO 0 -painiketta painettuna (1 sekunti):  
     - Sender: "Shutting down..." ja syvä uni.  
     - Receiver: "Shutting down...", kolmisävelinen ääni (1200 Hz, 1000 Hz, 800 Hz) ja syvä uni.  
   - Tai laitteet sammuvat automaattisesti 60 minuutin toimimattomuuden jälkeen.  

##### Esimerkkiskenaario
Sähköasentaja asentaa Senderin kellarin valokytkimeen ja pitää Receiverin mukanaan toimistossa ensimmäisessä kerroksessa. Kun joku kytkee valon päälle (sulkee kontaktin), Receiver näyttää "CLOSED" ja antaa äänimerkin, ilmoittaen sähköasentajalle toiminnasta ilman tarvetta mennä kellariin.

#### English
##### Applications in Electrician Work
An electrician can use Sender and Receiver for the following tasks:  
- **Equipment State Monitoring:**  
  - Connect Sender to the contacts of a switch, relay, or sensor (e.g., a limit switch on gates or an emergency stop button).  
  - Place Receiver in a convenient location (e.g., control room or in the electrician’s hand) to monitor the state.  
  - Example: Monitoring the on/off state of a pump in a water supply system without approaching the pump.  
- **Circuit Diagnostics:**  
  - Sender helps determine if a circuit is closed (e.g., checking wiring or relay functionality).  
  - Receiver alerts with a sound when closed, simplifying fault detection in the circuit.  
- **Remote Control:**  
  - Use in large buildings or sites with distributed equipment (e.g., warehouses, farms) where manually checking each contact is time-consuming.  
  - A range of up to 1-2 km enables state monitoring without long cable runs.  

##### Electrician’s Workflow with the Devices
1. **Preparing Sender:**  
   - Connect a contact (e.g., switch) to GPIO 32:  
     - Closed contact (connected to 3.3V) = "CLOSED".  
     - Open contact (connected to GND) = "OPEN".  
   - Attach a 3.7V battery (e.g., 750 mAh Li-Ion) to Sender.  
   - Power on: The screen displays "RS-Expert" and "v2.0" for 1 second.  
2. **Preparing Receiver:**  
   - Attach a 3.7V battery to Receiver.  
   - Power on: The screen displays "RS-Expert" and "v2.0" for 1 second, followed by a three-tone sound (800 Hz, 1000 Hz, 1200 Hz).  
   - Place Receiver within line of sight (up to 1-2 km from Sender).  
3. **Testing Operation:**  
   - Close the contact on Sender (e.g., turn on a switch):  
     - Sender displays "Sender", "CLOSED", and "TX: X%".  
     - Receiver shows "Receiver", "CLOSED", "RX: X%", and "TX: X%", and begins a continuous three-tone sound (1400 Hz, 1700 Hz, 2000 Hz).  
   - Open the contact:  
     - Sender displays "OPEN".  
     - Receiver shows "OPEN", sound stops.  
   - If Sender is off or out of range (after 5 seconds):  
     - Receiver shows "NO SIGNAL" and "Last signal: Xs ago".  
     - Upon receiving "SHUTDOWN", adds "Sender off".  
4. **Shutdown:**  
   - To shut down Sender or Receiver, press and hold the GPIO 0 button (1 second):  
     - Sender: "Shutting down..." and deep sleep.  
     - Receiver: "Shutting down...", three-tone sound (1200 Hz, 1000 Hz, 800 Hz), and deep sleep.  
   - Or devices automatically shut down after 60 minutes of inactivity.  

##### Example Scenario
An electrician installs Sender on a basement light switch and keeps Receiver in an upstairs office. When someone turns on the light (closes the contact), Receiver displays "CLOSED" and buzzes, notifying the electrician of activity without requiring a trip to the basement.

---

### Specifications / Tekniset tiedot / Спецификации
- **Frequency / Taajuus / Частота:** 868 MHz (LoRa)  
- **Power Supply / Virtalähde / Источник питания:** 3.7V Li-Ion battery (e.g., 750 mAh) / 3,7 V Li-Ion-akku (esim. 750 mAh) / Батарея Li-Ion 3.7V (например, 750 мАч)  
- **Range / Kantama / Дальность:** Up to 1-2 km (line of sight, depending on environment) / Jopa 1-2 km (näköyhteys, ympäristöstä riippuen) / До 1-2 км (прямая видимость, зависит от условий)  
- **Indicators / Ilmaisimet / Индикаторы:** OLED display, LED, buzzer (Receiver only) / OLED-näyttö, LED, summeri (vain Receiver) / OLED-дисплей, светодиод, зуммер (только Receiver)  

---

### Components / Komponentit / Компоненты
- **Sender / Lähetin / Передатчик:**  
  - GPIO 32: Contact input (HIGH = closed, LOW = open) / Kontaktitulo (HIGH = suljettu, LOW = auki) / Вход контакта (HIGH = замкнут, LOW = разомкнут)  
  - GPIO 0: Shutdown button (press for 1 second) / Sammutuspainike (paina 1 sekunti) / Кнопка выключения (нажать на 1 секунду)  
  - LED: Indicates transmission activity / Ilmaisee lähetystoiminnan / Указывает активность передачи  
- **Receiver / Vastaanotin / Приёмник:**  
  - GPIO 0: Shutdown button (press for 1 second) / Sammutuspainike (paina 1 sekunti) / Кнопка выключения (нажать на 1 секунду)  
  - LED: Indicates packet reception / Ilmaisee paketin vastaanoton / Указывает приём пакета  
  - Buzzer: Alerts for "CLOSED" state / Hälyttää "CLOSED"-tilassa / Оповещает о состоянии "CLOSED"  

---

### Operation / Käyttö / Работа
#### English
1. **Powering On:**  
   - Connect a 3.7V battery to each device.  
   - On startup, both devices display "RS-Expert" and "v2.0" for 1 second, then enter operational mode.  
   - Receiver emits a three-tone startup sound (800 Hz, 1000 Hz, 1200 Hz).  
2. **Sender:**  
   - Monitors GPIO 32 every 300 ms: "CLOSED" (HIGH) or "OPEN" (LOW).  
   - Displays: "Sender", "v2.0", "TX: X%", status, blinking dot, uptime.  
   - Shuts down after 60 minutes of inactivity or GPIO 0 press (1 second).  
3. **Receiver:**  
   - Displays: "Receiver", "v2.0", "TX: X%", "RX: X%", status, uptime.  
   - Alerts with a three-tone sound when "CLOSED".  
   - Shows "NO SIGNAL" if no signal for 5 seconds, "Sender off" on "SHUTDOWN".  
   - Shuts down after 60 minutes of no signal or GPIO 0 press (1 second).

#### Suomi
1. **Käynnistys:**  
   - Kytke 3,7 V akku kumpaankin laitteeseen.  
   - Käynnistyessä molemmat laitteet näyttävät "RS-Expert" ja "v2.0" 1 sekunnin ajan, sitten siirtyvät toimintatilaan.  
   - Receiver antaa kolmisävelisen käynnistysäänen (800 Hz, 1000 Hz, 1200 Hz).  
2. **Sender:**  
   - Tarkkailee GPIO 32:ta 300 ms välein: "CLOSED" (HIGH) tai "OPEN" (LOW).  
   - Näyttää: "Sender", "v2.0", "TX: X%", tilan, vilkkuvan pisteen, käyttöajan.  
   - Sammuu 60 minuutin toimimattomuuden jälkeen tai GPIO 0:n painalluksella (1 sekunti).  
3. **Receiver:**  
   - Näyttää: "Receiver", "v2.0", "TX: X%", "RX: X%", tilan, käyttöajan.  
   - Hälyttää kolmisävelisellä äänellä "CLOSED"-tilassa.  
   - Näyttää "NO SIGNAL" 5 sekunnin signaalittomuuden jälkeen, "Sender off" "SHUTDOWN"-viestin saapuessa.  
   - Sammuu 60 minuutin signaalittomuuden jälkeen tai GPIO 0:n painalluksella (1 sekunti).

#### Русский
1. **Включение:**  
   - Подключите батарею 3.7V к каждому устройству.  
   - При запуске оба устройства показывают "RS-Expert" и "v2.0" в течение 1 секунды, затем переходят в рабочий режим.  
   - Receiver издаёт трёхтональный звук (800 Гц, 1000 Гц, 1200 Гц).  
2. **Sender:**  
   - Проверяет GPIO 32 каждые 300 мс: "CLOSED" (HIGH) или "OPEN" (LOW).  
   - Отображает: "Sender", "v2.0", "TX: X%", состояние, мигающую точку, время работы.  
   - Выключается через 60 минут без активности или при нажатии GPIO 0 (1 секунда).  
3. **Receiver:**  
   - Отображает: "Receiver", "v2.0", "TX: X%", "RX: X%", состояние, время работы.  
   - Издаёт трёхтональный звук при "CLOSED".  
   - Показывает "NO SIGNAL" через 5 секунд без сигнала, "Sender off" при "SHUTDOWN".  
   - Выключается через 60 минут без сигнала или при нажатии GPIO 0 (1 секунда).

---

### Troubleshooting / Vianmääritys / Устранение неисправностей
- **No Display / Ei näyttöä / Нет изображения:** Check battery connection and charge / Tarkista akun liitäntä ja lataus / Проверьте подключение и заряд батареи.  
- **No Signal / Ei signaalia / Нет сигнала:** Ensure devices are within range and antennas are attached / Varmista, että laitteet ovat kantaman sisällä ja antennit kiinni / Убедитесь, что устройства в пределах досягаемости и антенны подключены.  
- **No Sound / Ei ääntä / Нет звука:** Verify buzzer connection on Receiver / Tarkista summerin liitäntä Receiverissä / Проверьте подключение зуммера на Receiver.  
- **Incorrect State / Väärä tila / Неправильное состояние:** Check GPIO 32 wiring on Sender (HIGH = 3.3V, LOW = GND) / Tarkista Senderin GPIO 32 -johdotus (HIGH = 3,3V, LOW = GND) / Проверьте подключение GPIO 32 на Sender (HIGH = 3.3V, LOW = GND).  

---

### Firmware Updates / Laiteohjelmistopäivitykset / Обновления прошивки
Visit `https://github.com/drpoh/LoRa_est32` for the latest firmware, source code, and support.  
Käy osoitteessa `https://github.com/drpoh/LoRa_est32` uusimpien laiteohjelmistojen, lähdekoodin ja tuen saamiseksi.  
Посетите `https://github.com/drpoh/LoRa_est32` для получения последних версий прошивки, исходного кода и поддержки.

---

### Safety Notes / Turvallisuusohjeet / Примечания по безопасности
- Use only 3.7V Li-Ion batteries with appropriate protection / Käytä vain 3,7 V Li-Ion-akkuja, joissa on asianmukainen suojaus / Используйте только батареи Li-Ion 3.7V с защитой.  
- Avoid exposing devices to water or extreme temperatures / Vältä laitteiden altistamista vedelle tai äärimmäisille lämpötiloille / Избегайте воздействия воды или экстремальных температур на устройства.  
- Disconnect the battery when not in use for extended periods / Irrota akku, kun laitetta ei käytetä pitkään aikaan / Отключайте батарею при длительном неиспользовании.

---

### Contact / Yhteydenotto / Контакты
For support, contact RS-Expert via the GitHub repository.  
Tukea varten ota yhteyttä RS-Expertiin GitHub-repositorion kautta.  
Для поддержки обратитесь к RS-Expert через репозиторий на GitHub.
