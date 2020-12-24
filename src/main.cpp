#include <Arduino.h>
#include <ArduinoOTA.h>

#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <MQTT.h>
#include <Button2.h>
#include <LineProtocol.h>

#include "setting.h"
#include "boot.h"
#include "alarm.h"
#include "background.h"

MQTTClient mqtt;
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

bool state_backlight = true;
bool state_alarm = false;

void drawText(char *text) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, bootlogo);
    tft.drawString(text, tft.width() / 2, tft.height() / 2, FONT_INTRO);
    delay(500);
}

struct mystat {
    float kitchen;
    float study;
    float livingroom;
    float bedroom;
    float spareroom;
};

struct mystat temperature0;
struct mystat temperature1;

void drawStatLine(char *title, float value, int line) {
    if (value == 0) {
        tft.setTextColor(TFT_DARKGREY);
    } else {
        tft.setTextColor(TFT_LIGHTGREY);
    }

    tft.setTextDatum(TL_DATUM);
    tft.drawString(title, 2, 2 + tft.fontHeight(FONT) * line + 2, FONT);
    tft.setTextDatum(TR_DATUM);

    char text[128] = {0};

    if (value == 0) {
        snprintf(text, 128, "-", value);
    } else {
        if (value < 18)
            tft.setTextColor(TFT_SKYBLUE);
        if (value > 22)
            tft.setTextColor(TFT_RED);
        snprintf(text, 128, "%.1f C", value);
    }
    tft.drawString(text, tft.width() - 2, 2 + tft.fontHeight(FONT) * line + 2, FONT);
}

void drawStat() {
    if(state_alarm) {
        Serial.println("In alarm!");
        return;
    }
    if (
            temperature0.bedroom == temperature1.bedroom &&
            temperature0.kitchen == temperature1.kitchen &&
            temperature0.livingroom == temperature1.bedroom &&
            temperature0.study == temperature1.study &&
            temperature0.spareroom == temperature1.spareroom)
    {
        return;
    }

    Serial.println("Refresh");
    Serial.print(temperature0.bedroom);
    Serial.print(" ");
    Serial.println(temperature1.bedroom);
    Serial.print(temperature0.kitchen);
    Serial.print(" ");
    Serial.println(temperature1.kitchen);
    Serial.print(temperature0.livingroom);
    Serial.print(" ");
    Serial.println(temperature1.livingroom);
    Serial.print(temperature0.study);
    Serial.print(" ");
    Serial.println(temperature1.study);
    Serial.print(temperature0.spareroom);
    Serial.print(" ");
    Serial.println(temperature1.spareroom);

    memcpy(&temperature0, &temperature1, sizeof(struct mystat));

    tft.startWrite();
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, backgroundlogo);

    drawStatLine("Bedroom", temperature1.bedroom, 0);
    drawStatLine("Study", temperature1.study, 1);
    drawStatLine("Livingroom", temperature1.livingroom, 2);
    drawStatLine("Spareroom", temperature1.spareroom, 3);
    drawStatLine("Kitchen", temperature1.kitchen, 4);

    tft.endWrite();
}

void callback(String &topic, String &payload) {
    Serial.println(topic);

    if(topic == "/test/alarm") {
        state_alarm = true;

        tft.fillScreen(TFT_RED);
        tft.setSwapBytes(true);
        tft.pushImage(0, 0, 240, 135, alarmlogo);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE);
        tft.drawString("ALARM", tft.width() / 2, tft.height() / 2, 4);

        digitalWrite(TFT_BACKLIGHT, (state_backlight = true) ? HIGH : LOW);
    }

    if(topic == "/sensor/temperature" || topic == "/esp8266/temperature") {
        struct line_protocol lp = line_protocol_parse(payload);

        if (lp.room == "kitchen") {
            temperature1.kitchen = lp.value.toFloat();
        }

        if (lp.room == "study") {
            temperature1.study = lp.value.toFloat();
        }

        if (lp.room == "livingroom") {
            temperature1.livingroom = lp.value.toFloat();
        }

        if (lp.room == "bedroom") {
            temperature1.bedroom = lp.value.toFloat();
        }

        if (lp.room == "spareroom") {
            temperature1.spareroom = lp.value.toFloat();
        }
    }
}

void tftSetup() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_LIGHTGREY);

    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, HIGH);
}

void wifiSetup() {
    drawText("Setting up WiFI.");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    drawText("Finished WiFI.");
}

void otaSetup() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();
}

void mqttSetup() {
    drawText("Setting up MQTT.");

    static WiFiClient wificlient;
    mqtt.begin("192.168.1.10", 1883, wificlient);
    mqtt.onMessage(callback);

    while(!mqtt.connect("")) delay(500);

    drawText("Finished MQTT.");

    mqtt.subscribe("/sensor/temperature");
    mqtt.subscribe("/esp8266/temperature");
    mqtt.subscribe("/test/alarm");

    drawText("Awaiting MQTT.");
}

void btnSetup() {
    btn1.setPressedHandler([](Button2 &b) {
            digitalWrite(TFT_BACKLIGHT, (state_backlight = !state_backlight) ? HIGH : LOW);
            });

    btn2.setPressedHandler([](Button2 &b) {
            state_alarm = false;
            digitalWrite(TFT_BACKLIGHT, (state_backlight = false) ? HIGH : LOW);
            });
}

void screenSetup() {
    temperature0.kitchen = 0;
    temperature0.study = 0;
    temperature0.livingroom = 0;
    temperature0.spareroom = 0;

    temperature1.kitchen = 0;
    temperature1.study = 0;
    temperature1.livingroom = 0;
    temperature1.spareroom = 0;
}

void setup() {
    Serial.begin(115200);

    screenSetup();
    tftSetup();
    wifiSetup();
    mqttSetup();
    btnSetup();

    digitalWrite(TFT_BACKLIGHT, state_backlight ? HIGH : LOW);
}

void screenboot() {
    tft.pushImage(240, 135, 240, 135, bootlogo);
}

void drawScreen() {
    drawStat();
}

void loop() {
    unsigned long start = millis();

    if (!mqtt.connected()) mqtt.connect("");

    mqtt.loop();
    btn1.loop();
    btn2.loop();

    ArduinoOTA.handle();

    drawScreen();

    while(millis() < start + 500) {
        btn1.loop();
        btn2.loop();
    }
}
