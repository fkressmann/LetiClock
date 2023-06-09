#include <Arduino.h>
#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontMatrise.h>
#include <time.h>
#include "ESP8266httpUpdate.h"
#include <DNSServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <credentials.h>

#define MY_NTP_SERVER "pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03" // Settings for middle european time

#define VERSION     "LetiClock-v19"
#define LED_PIN     D3
#define LDR         A0
#define BUTTON_L    D1
#define BUTTON_R    D2
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

const String prefix = MQTT_PREFIX;
const char *topicMessage = "message";
const char *topicLog = "log";

void reconnectMqtt(boolean log);

void doBusiness();

struct {
    bool available = false;
    int length;
    char buffer[256];
} mqttMessage;

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

cLEDMatrix<11, -10, HORIZONTAL_ZIGZAG_MATRIX> ledMatrix;
cLEDText ScrollingMsg;

int currentMinute, currentHour;
int previousMinute = 100; // Init with some insane value to trigger update on boot
const double E = 2.71828;
time_t now;
tm tm;
bool buttonL, buttonR = false;
bool outputEnabled = true;

struct WORD {
    int start;
    int length;
};

WORD W_ES = {0, 2};
WORD W_IST = {3, 3};
WORD W_M_FUENF = {7, 4};
WORD W_M_ZEHN = {18, 4};
WORD W_M_ZWANZIG = {11, 7};
WORD W_DREIVIERTEL = {22, 11};
WORD W_VIERTEL = {26, 7};
WORD W_NACH = {38, 4};
WORD W_VOR = {35, 3};
WORD W_HALB = {44, 4};
WORD W_S_ZWOELF = {49, 5};
WORD W_S_ZWEI = {62, 4};
WORD W_S_EIN = {61, 3};
WORD W_S_EINS = {60, 4};
WORD W_S_SIEBEN = {55, 6};
WORD W_S_DREI = {67, 4};
WORD W_S_FUENF = {73, 4};
WORD W_S_ELF = {85, 3};
WORD W_S_NEUN = {81, 4};
WORD W_S_VIER = {77, 4};
WORD W_S_ACHT = {89, 4};
WORD W_S_ZEHN = {93, 4};
WORD W_S_SECHS = {104, 5};
WORD W_UHR = {99, 3};

// first byte encodes array length, rest the LEDs positions
const int info_WIFI[] = {4, 16, 25, 53, 59};
const int info_MQTT[] = {4, 43, 33, 48, 92};
const int info_MQT[] = {3, 43, 33,
                        48}; // Used to display 'MQT' while displaying the time as this does not use any letters that time would use
const int info_UPDATE[] = {6, 8, 54, 67, 89, 92, 94};
const int info_DONE[] = {4, 22, 36, 61, 69};
const int info_ERROR[] = {5, 0, 23, 29, 36, 68};
const int info_CONNECT[] = {7, 39, 36, 55, 61, 69, 90, 92};
const int info_TIME[] = {4, 5, 12, 33, 56};

char serialBuffer[4] = {};

bool wasButtonPressed() {
    return buttonL || buttonR;
}

void sendData(String subtopic, const char *data, bool retained) {
    subtopic = prefix + subtopic;
    Serial.printf("Sending %s via MQTT to topic %s\n", data, subtopic.c_str());
    mqttClient.publish(subtopic.c_str(), data, retained);
}

void sendData(String subtopic, String data, bool retained) {
    sendData(subtopic, (String(asctime(&tm)) + data).c_str(), retained);
}

void adjustBrightness() {
    int value = analogRead(LDR);
    // Exponential scaling of 10bit analog input to 8bit LED brightness
    int brightness = max(2, (int) (pow(E, 0.0072195 * value) * 0.149831));
    Serial.printf("Brightness analog / digital: %d / %d \n", value, brightness);
    FastLED.setBrightness(brightness);
    FastLED.show();
}

uint8_t randomColor() {
    return random(0, 256);
}

void setLed(uint8_t i, uint8_t h, uint8_t s, uint8_t v) {
    Serial.printf("Setting LED %d (H%d S%d V%d)\n", i, h, s, v);
    ledMatrix(i).setHSV(h, s, v);
}

void setLed(uint8_t i, uint8_t color) {
    setLed(i, color, 255, 255);
}

void showInfo(const int *info) {
    FastLED.clear(true);
    int length = info[0];
    int color = randomColor();
    for (int i = 1; i <= length; i++) {
        setLed(info[i], color);
    }
    FastLED.show();
    FastLED.show();
}

void setTranslucentMqt(uint8_t brightness) {
    Serial.println("Setting translucent MQT");
    int length = info_MQT[0];
    int color = randomColor();
    for (int i = 1; i <= length; i++) {
        setLed(info_MQT[i], color, 255, brightness);
    }
    FastLED.show();
    FastLED.show();
}

void setTranslucentMqt() {
    setTranslucentMqt(255);
    // ToDo: Change this to a lower brightness as soon as using a Async MQTT lib
    // at the moment, mqttClint.connect() is blocking for up to 5 sec which prevents FastLED from dithering
    // and thus MQT might not be visible during connection when master brightness is low
}

void clearTranslucentMqt() {
    setTranslucentMqt(0);
}

void setWord(WORD word) {
    int color = randomColor();
    for (int i = word.start; i < word.start + word.length; i++) {
        setLed(i, color);
    }
}

void prepareMessage(char *payload, unsigned int length) {
    mqttMessage.buffer[0] = EFFECT_SCROLL_LEFT[0];
    mqttMessage.buffer[1] = ' '; // Two spaces in the beginning to slide message into canvas
    mqttMessage.buffer[2] = ' ';
    strncpy(mqttMessage.buffer + 3, payload, length);
    mqttMessage.buffer[length + 3] = ' '; // And one to slide it out
    mqttMessage.buffer[length + 4] = '\0'; //End string here cause it's a C string
    mqttMessage.available = true;
    mqttMessage.length = length + 4;
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    char *slashPointer = strrchr(topic, '/');
    if (strcmp(slashPointer + 1, topicMessage) == 0) {
        prepareMessage((char *) payload, length);
    }
}

void handleMinutes() {
    setWord(W_ES);
    setWord(W_IST);
    int minutes = currentMinute % 5;
    int color = randomColor();
    switch (minutes) {
        case 4:
            setLed(111, color);
        case 3:
            setLed(110, color);
        case 2:
            setLed(112, color);
        case 1:
            setLed(113, color);
    }

    if (currentMinute >= 0 && currentMinute < 5) {
        setWord(W_UHR);
        return;
    }
    if (currentMinute >= 5 && currentMinute < 10) {
        setWord(W_M_FUENF);
        setWord(W_NACH);
        return;
    }
    if (currentMinute >= 10 && currentMinute < 15) {
        setWord(W_M_ZEHN);
        setWord(W_NACH);
        return;
    }
    if (currentMinute >= 15 && currentMinute < 20) {
        if (random(0, 2) == 0) {
            setWord(W_VIERTEL);
            setWord(W_NACH);
        } else {
            setWord(W_VIERTEL);
            currentHour++;
        }
        return;
    }
    if (currentMinute >= 20 && currentMinute < 25) {
        if (random(0, 2) == 0) {
            setWord(W_M_ZWANZIG);
            setWord(W_NACH);
        } else {
            setWord(W_M_ZEHN);
            setWord(W_VOR);
            setWord(W_HALB);
            currentHour++;
        }
        return;
    }
    if (currentMinute >= 25 && currentMinute < 30) {
        setWord(W_M_FUENF);
        setWord(W_VOR);
        setWord(W_HALB);
        return;
    }
    if (currentMinute >= 30 && currentMinute < 35) {
        setWord(W_HALB);
        return;
    }
    if (currentMinute >= 35 && currentMinute < 40) {
        setWord(W_M_FUENF);
        setWord(W_NACH);
        setWord(W_HALB);
        return;
    }
    if (currentMinute >= 40 && currentMinute < 45) {
        if (random(0, 2) == 0) {
            setWord(W_M_ZWANZIG);
            setWord(W_VOR);
        } else {
            setWord(W_M_ZEHN);
            setWord(W_NACH);
            setWord(W_HALB);
        }
        return;
    }
    if (currentMinute >= 45 && currentMinute < 50) {
        if (random(0, 2) == 0) {
            setWord(W_VIERTEL);
            setWord(W_VOR);
        } else {
            setWord(W_DREIVIERTEL);
        }
        return;
    }
    if (currentMinute >= 50 && currentMinute < 55) {
        setWord(W_M_ZEHN);
        setWord(W_VOR);
        return;
    }
    if (currentMinute >= 55) {
        setWord(W_M_FUENF);
        setWord(W_VOR);
        return;
    }
}

void handleHours() {
    switch (currentHour) {
        case 1:
            if (currentMinute >= 0 && currentMinute < 5) {
                setWord(W_S_EIN);
            } else {
                setWord(W_S_EINS);
            }
            break;
        case 2:
            setWord(W_S_ZWEI);
            break;
        case 3:
            setWord(W_S_DREI);
            break;
        case 4:
            setWord(W_S_VIER);
            break;
        case 5:
            setWord(W_S_FUENF);
            break;
        case 6:
            setWord(W_S_SECHS);
            break;
        case 7:
            setWord(W_S_SIEBEN);
            break;
        case 8:
            setWord(W_S_ACHT);
            break;
        case 9:
            setWord(W_S_NEUN);
            break;
        case 10:
            setWord(W_S_ZEHN);
            break;
        case 11:
            setWord(W_S_ELF);
            break;
        case 12:
        case 0:
            setWord(W_S_ZWOELF);
            break;
    }
}

void renderClock() {
    currentHour = tm.tm_hour;
    currentMinute = tm.tm_min;
    FastLED.clear();
    handleMinutes();
    if (currentMinute >= 25) currentHour++;
    if (currentHour > 12) currentHour = currentHour - 12;
    handleHours();
    if (!mqttClient.connected()) setTranslucentMqt(); // Show MQT when connection is not established
    FastLED.show();
    FastLED.show();
}

void handleClock() {
    currentMinute = tm.tm_min;
    if (currentMinute != previousMinute) {
        renderClock();
        previousMinute = currentMinute;
    }
}

void showText(bool infinite) {
    sendData("ack", ": mes-rec", false);
    // Disconnect MQTT to
    // 1. not time-out cause message can run for long time
    // 2. not receive other messages while this is displayed, Broker will cache QOS1 for us
    bool wasMqttConnected = mqttClient.connected();
    if (wasMqttConnected) mqttClient.disconnect();
    FastLED.clearData();
    while (!buttonL && !buttonR) {
        ScrollingMsg.SetText((unsigned char *) mqttMessage.buffer, mqttMessage.length);
        while (!wasButtonPressed() && ScrollingMsg.UpdateText() != -1) {
            adjustBrightness();
            FastLED.delay(100);
        }
        if (!infinite) break;
    }
    buttonL = false; // Clear button press to not trigger unwanted actions in next handleButtons() run
    buttonR = false;
    if (wasMqttConnected) reconnectMqtt(false);
    mqttMessage.available = false;
    outputEnabled = true;
    if (infinite) renderClock(); // Directly trigger rendering of clock after running infinite message
}

void reconnectMqtt(bool log) {
    if (!mqttClient.connected()) {
        setTranslucentMqt();
        FastLED.show();
        Serial.println("Connecting to MQTT");
        if (log) showInfo(info_MQTT);
        if (mqttClient.connect(
                DEVICE_NAME,
                MQTT_USER,
                MQTT_PASSWORD,
                (prefix + "lwt").c_str(),
                0,
                true,
                "offline",
                false)
                ) {
            mqttClient.subscribe((prefix + topicMessage).c_str(), 1);
            if (log)
                sendData("lwt",
                         String(VERSION) + " connected",
                         true);
            clearTranslucentMqt();
            Serial.println("MQTT connected");
        } else {
            Serial.println("MQTT connection failed");
            FastLED.delay(1000);
        }
    }
}

void callbackUpdateStarted() {
    showInfo(info_UPDATE);
}

void callbackUpdateProgress(int current, int total) {
    int progress = ((float) current / (float) total) * 110;
    Serial.printf("Update current/total:progress %d / %d : %d\n", current, total, progress);
    setLed(progress, randomColor());
    FastLED.show();
    FastLED.show(); // Need this twice to be shown, no idea why
}

void callbackUpdateEnd() {
    showInfo(info_DONE);
    Serial.printf("Update done\n");
}

void callbackUpdateError(int error) {
    showInfo(info_ERROR);
    Serial.printf("OTA error: %d\n", error);
}

void update() {
    Serial.printf("Asking for update...\n");
    bool wasMqttConnected = mqttClient.connected();
    if (wasMqttConnected) mqttClient.disconnect();
    t_httpUpdate_return ret = ESPhttpUpdate.update(espClient, MQTT_SERVER, 443, "/server.php", VERSION);
    if (wasMqttConnected) reconnectMqtt(false);
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("%s\n", ESPhttpUpdate.getLastErrorString().c_str());
            sendData(topicLog, ESPhttpUpdate.getLastErrorString(), true);
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.printf("HTTP_UPDATE_NO_UPDATES\n");
            break;

        case HTTP_UPDATE_OK:
            Serial.printf("HTTP_UPDATE_OK\n");
            break;
    }
}

void IRAM_ATTR isrButtonL() {
    buttonL = true;
}

void IRAM_ATTR isrButtonR() {
    buttonR = true;
}

void updateTime() {
    time(&now);                       // read the current time
    localtime_r(&now, &tm);           // update the structure tm with the current time
}

void handleButtons() {
    // Start config portal only if both buttons pressed
    if (buttonL && buttonR) {
        Serial.printf("Start config portal\n");
        showInfo(info_CONNECT);
        WiFiManager wifiManager;
        wifiManager.startConfigPortal("LetiClock", "ThisIsChildish:D");
        Serial.printf("Config finished: %s: %s \n", WiFi.SSID().c_str(), WiFi.psk().c_str());
    } else if (buttonL) {
        if (outputEnabled) {
            // Disable output
            Serial.println("Disabling output");
            FastLED.clear(true);
            outputEnabled = false;
        } else {
            // Enable output
            Serial.println("Enabling output");
            outputEnabled = true;
            renderClock(); // Directly trigger rendering of clock
        }
    }
    buttonL = false;
    buttonR = false;
}

void handleSerialInput() {
    if (Serial.available()) {
        int count = Serial.readBytesUntil('\n', serialBuffer, 4);
        Serial.printf("Received serial: %s \n", serialBuffer);
        if (count > 1) {
            int ledToLightUp = atoi(serialBuffer);
            setLed(ledToLightUp, randomColor());
            Serial.printf("Lighting up LED %d \n", ledToLightUp);
        } else if (count == 1) {
            FastLED.clear(true);
            Serial.println("Clearing LED data");
        }
    }
}

// To be called in all kinds of loop actions without MQTT
void doBusiness() {
    if (outputEnabled) adjustBrightness();
    FastLED.show();
    handleButtons();
    handleSerialInput();
    FastLED.show();
    yield();
}

void setup() {
    Serial.begin(115200);
    // Important to attach interrupts before going into WiFi.isConnected() loop!
    pinMode(BUTTON_L, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_L), isrButtonL, FALLING);
    pinMode(BUTTON_R, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_R), isrButtonR, FALLING);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(ledMatrix[0], ledMatrix.Size() + 4).setCorrection(TypicalLEDStrip);
    adjustBrightness();

    Serial.printf("Firmware version: %s\n", VERSION);

    showInfo(info_WIFI);
    Serial.printf("Connecting to WiFi: %s: %s \n", WiFi.SSID().c_str(), WiFi.psk().c_str());
    WiFi.persistent(true);
    WiFi.begin();
    while (!WiFi.isConnected()) {
        delay(500);
        doBusiness();
        Serial.print(".");
    }
    Serial.println();

    espClient.setFingerprint(SSL_FINGERPRINT);

    ESPhttpUpdate.onStart(callbackUpdateStarted);
    ESPhttpUpdate.onProgress(callbackUpdateProgress);
    ESPhttpUpdate.onEnd(callbackUpdateEnd);
    ESPhttpUpdate.onError(callbackUpdateError);
    update();

    showInfo(info_TIME);
    configTime(MY_TZ, MY_NTP_SERVER);
    // Wait for valid time sync (not year 1970 anymore)
    Serial.println("Waiting for valid time sync...");
    do {
        doBusiness();
        delay(100); // Allow some time for getting a sync
        updateTime();
        Serial.printf("Year is: %d\n", tm.tm_year + 1900);
    } while (tm.tm_year < 100); // Years before 2000 -> indicates an issue
    Serial.printf("Got valid sync: %s\n", asctime(&tm));

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    ScrollingMsg.SetFont(MatriseFontData);
    ScrollingMsg.Init(&ledMatrix, ledMatrix.Width(), ScrollingMsg.FontHeight() + 1, 0, 0);
    ScrollingMsg.SetTextColrOptions(COLR_GRAD_AH, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff);

    prepareMessage(VERSION, sizeof(VERSION));
    showText(false);

    reconnectMqtt(true);
}

void loop() {
    FastLED.show();
    reconnectMqtt(false);
    FastLED.show();
    updateTime();
    FastLED.show();
    doBusiness();
    FastLED.show();
    if (outputEnabled) handleClock();
    FastLED.show();
    mqttClient.loop();
    if (millis() % 1000 < 2) update();
    FastLED.delay(1000);
    if (mqttMessage.available) showText(true);
}