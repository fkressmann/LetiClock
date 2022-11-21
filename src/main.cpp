#include <Arduino.h>
#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontMatrise.h>
#include <time.h>
#include <WiFiClient.h>
#include "ESP8266httpUpdate.h"
#include <DNSServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <credentials.h>

#define MY_NTP_SERVER "pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"

#define VERSION     "LetiClock-v6"
#define LED_PIN     D3 //The data pin of the arduino
#define LDR         A0
#define BUTTON_L    D1
#define BUTTON_R    D2
#define BRIGHTNESS  10 //Brightness of the LEDs
#define LED_TYPE    WS2812 //The type of the LED stripe
#define COLOR_ORDER GRB

const String prefix = MQTT_PREFIX;
const char *deviceName = DEVICE_NAME;
const char *topicMessage = "mes";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

cLEDMatrix<11, -10, HORIZONTAL_ZIGZAG_MATRIX> ledMatrix;
cLEDText ScrollingMsg;

int currentSecond, currentMinute, currentHour;
time_t now;
tm tm;

WiFiClient client;

const char *matrix =
        "ESKISTLF3NF"
        "GIZNAWZNHEZ" // reversed
        "DREIVIERTEL"
        "MJROVJCANGT" // reversed
        "HALBQZW2LFP"
        "NEBEISNIEWZ" // reversed
        "KDREIRHF3NF"
        "REIVNUENFLE" // reversed
        "WACHTZEHNRS"
        "RHUMFSHCESB"; // reversed

void update() {
    Serial.println("Checking for updates...");
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, MQTT_SERVER, 80, "/server.php", VERSION);
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(),
                          ESPhttpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
    }
}

void sendData(String subtopic, const char *data, bool retained) {
    subtopic = prefix + subtopic;
    mqttClient.publish(subtopic.c_str(), data, retained);
}

void sendData(String subtopic, String data, bool retained) {
    sendData(subtopic, data.c_str(), retained);
}

void sendData(String subtopic, String data) {
    sendData(subtopic, data, false);
}

void reconnectMqtt() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect(DEVICE_NAME, MQTT_USER, MQTT_PASSWORD, "leticlock/lwt", 0, 0, "", false)) { // ToDo: Check persistent session and mqtt re-delivery in case of committed too late
            Serial.println("connected");
            //once connected to MQTT broker, subscribe command if any
            mqttClient.subscribe((prefix + "mes").c_str());
            sendData("ip", WiFi.localIP().toString(), true);
            sendData("rssi", String(WiFi.RSSI()), true);
        } else {
            // ToDo: Make this non-blocking
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 6 seconds before retrying
            delay(6000);
        }
    }
}

void fadeAll() {
    FastLED.clear(true);
}

void showLed(int i, int r, int g, int b) {
    ledMatrix(i).r = r;
    ledMatrix(i).g = g;
    ledMatrix(i).b = b;
}

void wordES(int r, int g, int b) {
    showLed(0, r, g, b);
    showLed(1, r, g, b);
}

void wordIST(int r, int g, int b) {
    showLed(3, r, g, b);
    showLed(4, r, g, b);
    showLed(5, r, g, b);
}

void wordFUENF(int r, int g, int b) {
    showLed(7, r, g, b);
    showLed(8, r, g, b);
    showLed(9, r, g, b);
    showLed(10, r, g, b);
}

void wordZEHN(int r, int g, int b) {
    showLed(21, r, g, b);
    showLed(20, r, g, b);
    showLed(19, r, g, b);
    showLed(18, r, g, b);
}

void wordZWANZIG(int r, int g, int b) {
    showLed(17, r, g, b);
    showLed(16, r, g, b);
    showLed(15, r, g, b);
    showLed(14, r, g, b);
    showLed(13, r, g, b);
    showLed(12, r, g, b);
    showLed(11, r, g, b);
}

void wordDREI(int r, int g, int b) {
    showLed(22, r, g, b);
    showLed(23, r, g, b);
    showLed(24, r, g, b);
    showLed(25, r, g, b);
}


void wordVIERTEL(int r, int g, int b) {
    showLed(26, r, g, b);
    showLed(27, r, g, b);
    showLed(28, r, g, b);
    showLed(29, r, g, b);
    showLed(30, r, g, b);
    showLed(31, r, g, b);
    showLed(32, r, g, b);
}

void wordNACH(int r, int g, int b) {
    showLed(41, r, g, b);
    showLed(40, r, g, b);
    showLed(39, r, g, b);
    showLed(38, r, g, b);
}

void wordVOR(int r, int g, int b) {
    showLed(37, r, g, b);
    showLed(36, r, g, b);
    showLed(35, r, g, b);
}

void wordHALB(int r, int g, int b) {
    showLed(44, r, g, b);
    showLed(45, r, g, b);
    showLed(46, r, g, b);
    showLed(47, r, g, b);
}

void wordZWOELF(int r, int g, int b) {
    showLed(49, r, g, b);
    showLed(50, r, g, b);
    showLed(51, r, g, b);
    showLed(52, r, g, b);
    showLed(53, r, g, b);
}

void wordZWEI(int r, int g, int b) {
    showLed(65, r, g, b);
    showLed(64, r, g, b);
    showLed(63, r, g, b);
    showLed(62, r, g, b);
}

void wordEIN(int r, int g, int b) {
    showLed(63, r, g, b);
    showLed(62, r, g, b);
    showLed(61, r, g, b);
}

void wordEINS(int r, int g, int b) {
    showLed(63, r, g, b);
    showLed(62, r, g, b);
    showLed(61, r, g, b);
    showLed(60, r, g, b);
}

void wordSIEBEN(int r, int g, int b) {
    showLed(60, r, g, b);
    showLed(59, r, g, b);
    showLed(58, r, g, b);
    showLed(57, r, g, b);
    showLed(56, r, g, b);
    showLed(55, r, g, b);
}

void wordSTUNDEDREI(int r, int g, int b) {
    showLed(67, r, g, b);
    showLed(68, r, g, b);
    showLed(69, r, g, b);
    showLed(70, r, g, b);

}

void wordSTUNDEFUENF(int r, int g, int b) {
    showLed(73, r, g, b);
    showLed(74, r, g, b);
    showLed(75, r, g, b);
    showLed(76, r, g, b);
}

void wordVIER(int r, int g, int b) {
    showLed(80, r, g, b);
    showLed(79, r, g, b);
    showLed(78, r, g, b);
    showLed(77, r, g, b);
}

void wordNEUN(int r, int g, int b) {
    showLed(84, r, g, b);
    showLed(83, r, g, b);
    showLed(82, r, g, b);
    showLed(81, r, g, b);
}

void wordELF(int r, int g, int b) {
    showLed(87, r, g, b);
    showLed(86, r, g, b);
    showLed(85, r, g, b);
}

void wordACHT(int r, int g, int b) {
    showLed(89, r, g, b);
    showLed(90, r, g, b);
    showLed(91, r, g, b);
    showLed(92, r, g, b);
}

void wordSTUNDEZEHN(int r, int g, int b) {
    showLed(93, r, g, b);
    showLed(94, r, g, b);
    showLed(95, r, g, b);
    showLed(96, r, g, b);
}

void wordSECHS(int r, int g, int b) {
    showLed(108, r, g, b);
    showLed(107, r, g, b);
    showLed(106, r, g, b);
    showLed(105, r, g, b);
    showLed(104, r, g, b);
}

void wordUHR(int r, int g, int b) {
    showLed(101, r, g, b);
    showLed(100, r, g, b);
    showLed(99, r, g, b);
}

int color() {
    return random(0, 255);
}

int findCharPosition(char character, int start) {
    Serial.print("Trying to find char: ");
    Serial.println(character);
    for (int i = start; i < 110; i++) {
        if (character == matrix[i]) {
            Serial.print("Found at position: ");
            Serial.println(i);
            return i;
        }
    }
    Serial.println("Didn't find it");
    return -1;
}

void showWord(char const *word, int wordLength) {
    // ToDo: Update this to show chars at random places cause forming a word in order is unlikely ayways
    // int wordLength = (sizeof(word) / sizeof(char)) - 1;
    int pos = 0;
    int pixel[wordLength];
    bool searchEverywhere = false;
    Serial.print("Trying to find word: ");
    Serial.println(word);

    for (int i = 0; i < wordLength; i++) {
        if (searchEverywhere) pos = 0;
        pos = findCharPosition(word[i], pos);
        if (pos == -1) {
            searchEverywhere = true;
            continue;
        }
        pixel[i] = pos;
    }
    fadeAll();
    for (int i = 0; i < wordLength; i++) {
        showLed(pixel[i], 100, 100, 100);
        Serial.print("Displaying char ");
        Serial.print(word[i]);
        Serial.print(" at position ");
        Serial.println(pixel[i]);
    }
    FastLED.show();
    Serial.println("Done.");
}

void showText(const String message, int times) {
    // ToDo: Make this non-blocking for main loop | or maybe not cause we might need to block the MQTT callback

    // ToDo: Work just with char* s
    fadeAll();
    String concat = String(EFFECT_SCROLL_LEFT) + "   " + message + "   ";
    for (int i = 0; i < times; i++) {
        ScrollingMsg.SetText((unsigned char *) concat.c_str(), concat.length());
        while (ScrollingMsg.UpdateText() != -1) {
            FastLED.delay(100);
            yield();
        }
    }
    fadeAll();
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.println("Received MQTT message: ");
    Serial.print(topic);
    Serial.print((char *) payload);
    Serial.println();
    char *slashPointer = strrchr(topic, '/');

    if (strcmp(slashPointer + 1, topicMessage) == 0) {
        char payloadCstr[length + 1];
        strncpy(payloadCstr, (char *) payload, length);
        payloadCstr[length] = '\0';
        String message = String(payloadCstr);
        showText(message, 1);
    }

}

void handleClock() {
    currentHour = tm.tm_hour;
    currentMinute = tm.tm_min;
    currentSecond = tm.tm_sec;

    // ToDo: Change LEDs only when time changes, keep coloring of a word identical when not changed

    // ToDo: Define a fixed color set

    wordES(color(), color(), color());
    wordIST(color(), color(), color());

    if (currentMinute >= 0 && currentMinute < 5) {
        wordUHR(color(), color(), color());
    }
    if (currentMinute >= 5 && currentMinute < 10) {
        wordFUENF(color(), color(), color());
        wordNACH(color(), color(), color());
    }
    if (currentMinute >= 10 && currentMinute < 15) {
        wordZEHN(color(), color(), color());
        wordNACH(color(), color(), color());
    }
    if (currentMinute >= 15 && currentMinute < 20) {
        wordVIERTEL(color(), color(), color());
        wordNACH(color(), color(), color());
    }
    if (currentMinute >= 20 && currentMinute < 25) {
        wordZWANZIG(color(), color(), color());
        wordNACH(color(), color(), color());
    }
    if (currentMinute >= 25 && currentMinute < 30) {
        wordFUENF(color(), color(), color());
        wordVOR(color(), color(), color());
        wordHALB(color(), color(), color());
        currentHour += 1;
    }
    if (currentMinute >= 30 && currentMinute < 35) {
        wordHALB(color(), color(), color());
        currentHour += 1;
    }
    if (currentMinute >= 35 && currentMinute < 40) {
        wordFUENF(color(), color(), color());
        wordNACH(color(), color(), color());
        wordHALB(color(), color(), color());
        currentHour += 1;
    }
    if (currentMinute >= 40 && currentMinute < 45) {
        wordZWANZIG(color(), color(), color());
        wordVOR(color(), color(), color());
        currentHour += 1;
    }
    if (currentMinute >= 45 && currentMinute < 50) {
        // ToDo: Add dreiviertel Schreibweise
        wordVIERTEL(color(), color(), color());
        wordVOR(color(), color(), color());
        currentHour++;
    }
    if (currentMinute >= 50 && currentMinute < 55) {
        wordZEHN(color(), color(), color());
        wordVOR(color(), color(), color());
        currentHour += 1;
    }
    if (currentMinute >= 55) {
        wordFUENF(color(), color(), color());
        wordVOR(color(), color(), color());
        currentHour += 1;
    }

    if (currentHour > 12) currentHour = currentHour - 12;

    switch (currentHour) {
        case 1:
            if (currentMinute >= 0 && currentMinute < 5) {
                wordEIN(color(), color(), color());
            } else {
                wordEINS(color(), color(), color());
            }
            break;
        case 2:
            wordZWEI(color(), color(), color());
            break;
        case 3:
            wordSTUNDEDREI(color(), color(), color());
            break;
        case 4:
            wordVIER(color(), color(), color());
            break;
        case 5:
            wordSTUNDEFUENF(color(), color(), color());
            break;
        case 6:
            wordSECHS(color(), color(), color());
            break;
        case 7:
            wordSIEBEN(color(), color(), color());
            break;
        case 8:
            wordACHT(color(), color(), color());
            break;
        case 9:
            wordNEUN(color(), color(), color());
            break;
        case 10:
            wordSTUNDEZEHN(color(), color(), color());
            break;
        case 11:
            wordELF(color(), color(), color());
            break;
        case 12:
        case 0:
            wordZWOELF(color(), color(), color());
            break;
    }

    int minutes = currentMinute % 5;
    int r = color();
    int g = color();
    int b = color();
    switch (minutes) {
        case 4:
            showLed(111, r, g, b);
        case 3:
            showLed(110, r, g, b);
        case 2:
            showLed(112, r, g, b);
        case 1:
            showLed(113, r, g, b);
    }
}

void setup() {
    Serial.begin(115200);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(ledMatrix[0], ledMatrix.Size() + 4).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);

    Serial.print("Firmware version: ");
    Serial.println(VERSION);
    Serial.println("Connecting to WiFi");

    showWord("WIFI", 4);
    WiFiManager wifiManager;
    wifiManager.autoConnect("LetiClock", "ThisIsChildish:D");
    Serial.println("");
    Serial.println("WiFi connected");

    // ToDo: Execute this on regular basis
    showWord("UPDATE", 6);
    update();

    showWord("MQTT", 4);
    mqttClient.setServer(MQTT_SERVER, 1883);
    mqttClient.setCallback(mqttCallback);
    reconnectMqtt();

    configTime(MY_TZ, MY_NTP_SERVER);

    ScrollingMsg.SetFont(MatriseFontData);
    ScrollingMsg.Init(&ledMatrix, ledMatrix.Width(), ScrollingMsg.FontHeight() + 1, 0, 0);
    ScrollingMsg.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0xff, 0x00, 0x00);

}

void loop() {
    reconnectMqtt();
    mqttClient.loop();
    time(&now);                       // read the current time
    localtime_r(&now, &tm);           // update the structure tm with the current time
    handleClock();

    FastLED.show();
    delay(1000);
    fadeAll();
}