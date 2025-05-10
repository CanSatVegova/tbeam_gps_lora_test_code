#include <Arduino.h>
#include "LoRaBoards.h"
#include <TinyGPS++.h>
#include <SPI.h>
#include <LittleFS.h>

TinyGPSPlus gps;

long long x = 0;

double lnga = 0;
double lata = 0;
int sc = 0;


double lastSavedLat = 0.0;
double lastSavedLng = 0.0;

void setup()
{
    setupBoards();

    Serial.begin(9600);
    SerialGPS.begin(9600);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(15, INPUT_PULLUP);

    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed! Formatting...");
        if (LittleFS.format()) {
            Serial.println("LittleFS formatted successfully.");
            if (!LittleFS.begin()) {
                Serial.println("Failed to mount LittleFS after formatting.");
                return;
            }
        } else {
            Serial.println("LittleFS formatting failed!");
            return;
        }
    } else {
        Serial.println("LittleFS mounted.");
    }

    delay(1500);
    
    u8g2->clearDisplay();
    u8g2->sendBuffer();

    if (!LittleFS.exists("/gps_log.txt")) {
        File f = LittleFS.open("/gps_log.txt", FILE_WRITE);
        if (f) f.close();
    }
}

void saveCoordsToFile(double lat, double lng) {
    double roundedLat = round(lat * 10000.0) / 10000.0;
    double roundedLng = round(lng * 10000.0) / 10000.0;

    if (roundedLat == lastSavedLat && roundedLng == lastSavedLng) {
        return;
    }

    File file = LittleFS.open("/gps_log.txt", FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    if ((lat > 0.0 && lng > 0.0)) {
        if (gps.time.isValid()) {
            file.printf("%.4f,%.4f,%02d:%02d:%02d\n", 
                        lat, lng, 
                        gps.time.hour(), gps.time.minute(), gps.time.second());
        } else {
            file.printf("%.4f,%.4f,Invalid Time\n", lat, lng);
        }
        file.close();

        lastSavedLat = roundedLat;
        lastSavedLng = roundedLng;
    }
}

void displayInfo()
{
    double avgLat = (sc > 0) ? (lata / sc) : 0.0;
    double avgLng = (sc > 0) ? (lnga / sc) : 0.0;

    char satStr[30];
    sprintf(satStr, "Sats: %d", gps.satellites.value());
    char latStr[30];
    sprintf(latStr, "Lat: %.4f", avgLat);
    char lngStr[30];
    sprintf(lngStr, "Lng: %.4f", avgLng);

    u8g2->clearDisplay();
    u8g2->drawStr(0, 15, satStr);
    u8g2->drawStr(0, 30, lngStr);
    u8g2->drawStr(0, 45, latStr);
    u8g2->sendBuffer();

    saveCoordsToFile(avgLat, avgLng);

    lata = 0;
    lnga = 0;
    sc = 0;
}

void readGPSLog() {
    File file = LittleFS.open("/gps_log.txt", "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.println("---- GPS Logs ----");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

void loop()
{
    if (digitalRead(15) == LOW) {
        if (LittleFS.exists("/gps_log.txt")) {
            LittleFS.remove("/gps_log.txt");
            Serial.println("gps_log.txt deleted.");
        } else {
            Serial.println("gps_log.txt does not exist.");
        }
        delay(1000);
    }

    if (digitalRead(BUTTON_PIN) == LOW) {
        readGPSLog();
        delay(100);
    }

    while (SerialGPS.available() > 0) {
        char c = SerialGPS.read();
        //? Serial.print(c);

        if (gps.encode(c)) {
            if (x > 200000) {
                displayInfo();
                x = 0;
            }
            else if (!(x % 1000)) {
                if (gps.location.isValid()) {
                    lata += gps.location.lat();
                    lnga += gps.location.lng();
                    sc++;
                }
            }
        }
    }

    x++;

    if (millis() > 15000 && gps.charsProcessed() < 10) {
        Serial.println(F("No GPS detected: check wiring."));
        delay(15000);
    }
}
