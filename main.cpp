#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TimeLib.h>
#include <Wire.h>
#include <EEPROM.h>

RTC_DS3231 rtc;

Adafruit_SSD1306 display(128, 64, &Wire);

uint8_t mode = 0;
uint8_t setPart = 0; // 0=none, 1=HH, 2=MM

int alarmHour = 12;
int alarmMinute = 0;
int buzzerFreq = 1000; // default 1kHz

// EEPROM
int addrHour = 0;
int addrMinute = 1;
int addrFreq = 2;   // 2 byte cukup untuk int kecil

void saveSettings() {
    EEPROM.update(addrHour, alarmHour);
    EEPROM.update(addrMinute, alarmMinute);
    EEPROM.update(addrFreq, buzzerFreq / 20);
}

// icon jam
const unsigned char alarmIcon [] PROGMEM = {
  0b00111100,
  0b01000010,
  0b10100101,
  0b10000001,
  0b10100101,
  0b10011001,
  0b01000010,
  0b00111100
};

void drawAlarmIcon(int x, int y) {
  display.drawBitmap(x, y, alarmIcon, 8, 8, WHITE);
}

// button
const int buttonPin = 2;
bool lastButtonState = HIGH;
unsigned long pressTime = 0;
bool isHolding = false;
volatile bool buttonEvent = false;
volatile unsigned long pressStart = 0;

void handleButtonInterrupt() {
    static bool lastState = HIGH;
    bool state = digitalRead(buttonPin);

    if (state == LOW && lastState == HIGH) {
        // Tombol ditekan
        pressStart = millis();
    } 
    else if (state == HIGH && lastState == LOW) {
        // Tombol dilepas
        unsigned long pressDuration = millis() - pressStart;
        // tandai ada event tombol, detail ditangani di loop
        buttonEvent = true;
    }

    lastState = state;
}

// encoder (contoh sederhana)
const int pinA = 3;
const int pinB = 4;
int lastEncoded = 0;

// buzzer
const int buzzerPin = 6;  // buzzer untuk alarm
bool alarmTriggered = false;
const int buzzerFreqMin = 500;
const int buzzerFreqMax = 4000;

// blinking
bool blinkState = true;
unsigned long lastBlink = 0;

void displayScreen(const DateTime tm, int md) {
    if (md == 0) {
        display.setCursor(30, 20);

        // HH:MM size 2
        display.setTextSize(2);
        if (tm.hour() < 10) display.print("0");
        display.print(tm.hour());
        display.print(":");
        if (tm.minute() < 10) display.print("0");
        display.print(tm.minute());

        // detik kecil di bawah atau samping
        display.setCursor(90, 20);
        display.setTextSize(1);

        if (tm.second() < 10) display.print("0");
        display.print(tm.second());
        display.setCursor(0,0);
        display.setTextSize(1);
        display.print(tm.toString("DD:MMM:YYYY"));

        // Tampilkan ikon alarm jika alarm sudah di-set dan belum berbunyi
        if (!alarmTriggered) {
            drawAlarmIcon(120, 0); // pojok kanan atas
        }

        float vBat = readBatteryVoltage();
        float pct = batteryPercent(vBat);
        drawBattery(90, 55, pct); // pojok kiri atas OLED
        if (mode == 0 && !alarmTriggered) {
            drawAudioLevel(0, 56, buzzerFreq);
        }
    } else if (md == 1) {
        display.setCursor(0,0);
        display.setTextSize(4);

        char buffer[6];
        sprintf(buffer, "%02d:%02d", alarmHour, alarmMinute);

        if (setPart == 1 && !blinkState) {
            buffer[0] = ' ';
            buffer[1] = ' ';
        } else if (setPart == 2 && !blinkState) {
            buffer[3] = ' ';
            buffer[4] = ' ';
        }

        display.print(buffer);
    }

    display.display();
}

void handleButton() {
    bool reading = digitalRead(buttonPin);

    if (reading == LOW && lastButtonState == HIGH) {
        pressTime = millis();
        isHolding = true;
    }

    if (reading == LOW && isHolding) {
        if (millis() - pressTime > 2000 && mode == 0) {
            // masuk mode set alarm
            mode = 1;
            setPart = 1; // mulai dari HH
            isHolding = false;
        }
    }

    if (reading == HIGH && lastButtonState == LOW) {
        if (mode == 1) {
            // Short press di mode set alarm → pindah HH → MM → selesai
            if (setPart == 1) setPart = 2;
            else if (setPart == 2) {
                setPart = 0;
                mode = 0; // keluar mode set alarm

                saveSettings(); // 🔥 simpan ke EEPROM
            }
        } else if (mode == 0) {
            // Mode normal → tombol short press matikan alarm
            if (alarmTriggered) {
                alarmTriggered = false;
            }
        }

        isHolding = false;
    }

    lastButtonState = reading;
}

void handleEncoder() {
    int MSB = digitalRead(pinA);
    int LSB = digitalRead(pinB);

    int encoded = (MSB << 1) | LSB;
    int sum  = (lastEncoded << 2) | encoded;

    if (mode == 0) {
        if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
            buzzerFreq += 100;
        }

        if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
            buzzerFreq -= 100;
        }

        buzzerFreq = constrain(buzzerFreq, 200, 4000);
    }
    // ===== HANYA MODE SET ALARM =====
    if (mode == 1) {
        if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
            if (setPart == 1) alarmHour = (alarmHour + 1) % 24;
            else if (setPart == 2) alarmMinute = (alarmMinute + 1) % 60;
        }

        if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
            if (setPart == 1) alarmHour = (alarmHour - 1 + 24) % 24;
            else if (setPart == 2) alarmMinute = (alarmMinute - 1 + 60) % 60;
        }
    }

    // ===== MODE NORMAL: TIDAK ADA KONTROL APA PUN =====
    // encoder tidak mempengaruhi buzzer atau variabel lain

    lastEncoded = encoded;
}

float readBatteryVoltage() {
    int raw = analogRead(A0); // pin untuk baterai
    float voltage = raw / 1023.0 * 5.0 * ((15.0 + 10.0)/10.0); // tegangan divider
    return voltage;
}

float batteryPercent(float voltage) {
    float minV = 9.0;
    float maxV = 12.6;
    float pct = (voltage - minV) / (maxV - minV) * 100.0;
    if (pct > 100) pct = 100;
    if (pct < 0) pct = 0;
    return pct;
}

void drawBattery(int x, int y, float percent) {
    display.drawRect(x, y, 12, 6, WHITE); // outline baterai
    display.drawRect(x+12, y+1, 2, 4, WHITE); // kepala baterai
    int w = (int)(percent / 100.0 * 10); // isi baterai 10px max
    display.fillRect(x+1, y+1, w, 4, WHITE);
    
    display.setCursor(x+16, y-1);
    display.setTextSize(1);
    display.print((int)percent);
    display.print("%");
}

void drawAudioLevel(int x, int y, int freq) {
    // batas frekuensi
    const int minF = 200;
    const int maxF = 4000;

    // hitung persen dari frekuensi
    int percent = map(freq, minF, maxF, 0, 100);

    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;
    
    // ===== teks % =====
    display.setTextSize(1);
    display.setCursor(x, y);
    display.print(percent);
    display.print("%");
}

void setup() {
    Serial.begin(9600);

    pinMode(buzzerPin, OUTPUT); // set buzzer sebagai output
    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonInterrupt, CHANGE);

    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }

    if (rtc.lostPower()) {
        Serial.println("RTC lost power! Setting the time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    alarmHour = EEPROM.read(addrHour);
    alarmMinute = EEPROM.read(addrMinute);

    // EEPROM int perlu 2 byte
    buzzerFreq = EEPROM.read(addrFreq) * 20;

    // safety fallback kalau EEPROM kosong
    if (alarmHour > 23) alarmHour = 0;
    if (alarmMinute > 59) alarmMinute = 0;
    if (buzzerFreq < 200 || buzzerFreq > 4000) buzzerFreq = 1000;

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.dim(true);
    display.setTextColor(WHITE);
    display.clearDisplay();
}

void loop() {
    static unsigned long lastInterruptTime = 0;
    static bool alreadyTriggered = false;
    unsigned long interruptTime = millis();

    DateTime now = rtc.now();

    handleEncoder();

    // Handle button event
    if(interruptTime - lastInterruptTime > 50) { 
        if (buttonEvent) {
            buttonEvent = false; // reset flag
            unsigned long pressDuration = millis() - pressStart;

            if (pressDuration >= 2000 && mode == 0 && !alarmTriggered) {
                // long press → masuk mode set alarm
                mode = 1;
                setPart = 1;
            } 
            else if (pressDuration < 2000) {
                if (mode == 1) {
                    // short press → pindah HH → MM → selesai
                    if (setPart == 1) setPart = 2;
                    else if (setPart == 2) {
                        setPart = 0;
                        mode = 0;
                    }
                } else if (mode == 0) {
                    // short press di mode normal → matikan alarm
                    if (alarmTriggered) alarmTriggered = false;
                }
            }
            lastInterruptTime = interruptTime;
        }
    }

    // Cek alarm
    if (mode == 0 && !alreadyTriggered) {
        if (now.hour() == alarmHour && now.minute() == alarmMinute) {
            alarmTriggered = true;
            alreadyTriggered = true;
        }
    }

    if (now.minute() != alarmMinute) {
        alreadyTriggered = false;
    }

    display.clearDisplay();

    if (mode == 0) {
        displayScreen(now, 0);
    } else if (mode == 1) {
        displayScreen(now, 1);
    }

    if (millis() - lastBlink > 500) {
        blinkState = !blinkState;
        lastBlink = millis();
    }

    // jalankan buzzer jika alarm aktif
    if (alarmTriggered) {
    tone(buzzerPin, buzzerFreq); // freq tetap fixed / dari logika lain
    } else {
        noTone(buzzerPin);
    }
}