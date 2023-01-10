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
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"

#define VERSION     "LetiClock-v15"
#define LED_PIN     D3 //The data pin of the arduino
#define LDR         A0
#define BUTTON_L    D1
#define BUTTON_R    D2
#define LED_TYPE    WS2812 //The type of the LED stripe
#define COLOR_ORDER GRB

#define DEBUG

const String prefix = MQTT_PREFIX;
const char *deviceName = DEVICE_NAME;
const char *topicMessage = "message";
const char *topicLog = "log";

void reconnectMqtt();
void reconnectMqtt(boolean log);

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
bool d1Triggered, d2Triggered = false;

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


bool wasButtonPressed() {
    return d1Triggered || d2Triggered;
}

void sendData(String subtopic, const char *data, bool retained) {
    subtopic = prefix + subtopic;
    mqttClient.publish(subtopic.c_str(), data, retained);
}

void sendData(String subtopic, String data, bool retained) {
    sendData(subtopic, (String(asctime(&tm)) + data).c_str(), retained);
}

void adjustBrightness() {
    int value = analogRead(A0);
    // Exponential scaling of 10bit analog input to 8bit LED brightness
    int brightness = max(2, (int) (pow(E, 0.0072195 * value) * 0.149831));
#ifdef DEBUG
        Serial.printf("Brightness analog / digital: %d / %d \n", value, brightness);
#endif
    FastLED.setBrightness(brightness);
}

uint8 randomColor() {
    return random(0, 256);
}

void setLed(int i, int color) {
    ledMatrix(i).setHSV(color, 255, 255);
}

void setWord(WORD word) {
    int color = randomColor();
    for (int i = word.start; i < word.start + word.length; i++) {
        setLed(i, color);
    }
}

int findCharPosition(char character, int start) {
    for (int i = start; i < 110; i++) {
        if (character == matrix[i]) return i;
    }
    return -1;
}

void showWord(char const *word, int wordLength) {
    int pos = 0;
    int pixel[wordLength];
    bool searchEverywhere = false;

    for (int i = 0; i < wordLength; i++) {
        if (searchEverywhere) pos = 0;
        pos = findCharPosition(word[i], pos);
        if (pos == -1) {
            searchEverywhere = true;
            i--;
            continue;
        }
        pixel[i] = pos;
        pos--;
    }
    FastLED.clear(true);
    int color = randomColor();
    for (int i = 0; i < wordLength; i++) {
        setLed(pixel[i], color);
    }
    FastLED.show();
}

void showText(bool infinite) {
    sendData("ack", ": mes-rec", false);
    // Disconnect MQTT to
    // 1. not time-out cause message can run for long time
    // 2. not receive other messages while this is displayed, Broker will cache QOS1 for us
    bool isMqttConnected = mqttClient.connected();
    if (isMqttConnected) mqttClient.disconnect();
    FastLED.clearData();
    while (!d1Triggered && !d2Triggered) {
        ScrollingMsg.SetText((unsigned char *) mqttMessage.buffer, mqttMessage.length);
        while (!wasButtonPressed() && ScrollingMsg.UpdateText() != -1) {
            adjustBrightness();
            FastLED.delay(100);
        }
        if (!infinite) break;
    }
    if (isMqttConnected) reconnectMqtt(false);
    mqttMessage.available = false;
    previousMinute = 100; // Fill with crap data to trigger clock rendering after message finished
}

void prepareMessage(char *payload, unsigned int length) {
    mqttMessage.buffer[0] = EFFECT_SCROLL_LEFT[0];
    mqttMessage.buffer[1] = ' ';
    strncpy(mqttMessage.buffer + 2, payload, length);
    mqttMessage.buffer[length + 2] = ' ';
    mqttMessage.buffer[length + 3] = '\0';
    mqttMessage.available = true;
    mqttMessage.length = length + 3;
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    char *slashPointer = strrchr(topic, '/');
    if (strcmp(slashPointer + 1, topicMessage) == 0) {
        prepareMessage((char *) payload, length);
    } else {
        sendData("ack", ": pong", false);
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

void handleClock() {
    currentHour = tm.tm_hour;
    currentMinute = tm.tm_min;
    if (currentMinute != previousMinute) {
        FastLED.clear();
        previousMinute = currentMinute;
        handleMinutes();
        if (currentMinute >= 25) currentHour++;
        if (currentHour > 12) currentHour = currentHour - 12;
        handleHours();
    }
}

void reconnectMqtt(bool log) {
    if (!mqttClient.connected()) {
#ifdef DEBUG
        Serial.println("Connecting to MQTT");
#endif
        if (log) showWord("MQTT", 4);
        if (mqttClient.connect(DEVICE_NAME, MQTT_USER, MQTT_PASSWORD, "leticlock/lwt", 0, 0, "offline",
                               false)) {
            mqttClient.subscribe((prefix + topicMessage).c_str(), 1);
            if (log) sendData(topicLog, ": " + String(VERSION) + " connected", true);
#ifdef DEBUG
            Serial.println("MQTT connected");
#endif
        } else {
#ifdef DEBUG
            Serial.println("MQTT connection failed");
#endif
            FastLED.delay(1000);
        }
    }
}

void reconnectMqtt() {
    reconnectMqtt(true);
}

void callbackUpdateStarted() {
    showWord("UPDATE", 6);
}

// Somehow doesn't work, needs to be inspected
void callbackUpdateProgress(int current, int total) {
   int progress = ((float) current / (float) total) * 110;
#ifdef DEBUG
    Serial.printf("Update current/total:progress %d / %d : %d\n", current, total, progress);
#endif
   setLed(progress, randomColor());
   FastLED.show();
   FastLED.show(); // Need this twice to be shown, no idea why
}

void callbackUpdateEnd() {
    showWord("DONE", 4);
#ifdef DEBUG
    Serial.println("Update done.");
#endif
}

void callbackUpdateError(int error) {
    showWord("ERROR", 5);
#ifdef DEBUG
    Serial.print("OTA error: ");
    Serial.println(error);
#endif
}

void update() {
#ifdef DEBUG
    Serial.println("Asking for update...");
#endif
    bool mqttIsConnected = mqttClient.connected();
    if (mqttIsConnected) mqttClient.disconnect();
    t_httpUpdate_return ret = ESPhttpUpdate.update(espClient, MQTT_SERVER, 443, "/server.php", VERSION);
    if (mqttIsConnected) reconnectMqtt(false);
    switch (ret) {
        case HTTP_UPDATE_FAILED:
#ifdef DEBUG
            Serial.println(ESPhttpUpdate.getLastErrorString());
#endif
            sendData(topicLog, ESPhttpUpdate.getLastErrorString(), true);
            break;
#ifdef DEBUG
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
#endif
    }
}

void IRAM_ATTR isrD1() {
    d1Triggered = true;
}

void IRAM_ATTR isrD2() {
    d2Triggered = true;
}

void updateTime() {
    time(&now);                       // read the current time
    localtime_r(&now, &tm);           // update the structure tm with the current time
}

void handleButtons() {
    // Start configportal only if both buttons pressed
    if (d1Triggered && d2Triggered) {
#ifdef DEBUG
        Serial.println("Start config portal");
#endif
        showWord("CONNECT", 7);
        WiFiManager wifiManager;
        wifiManager.startConfigPortal("LetiClock", "ThisIsChildish:D");
#ifdef DEBUG
        Serial.printf("Config finished: %s: %s \n", WiFi.SSID().c_str(), WiFi.psk().c_str());
#endif
    }
    d1Triggered = false;
    d2Triggered = false;
}

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
#endif
    // Important to attach interrupts before going into WiFi.isConnected() loop!
    pinMode(D1, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(D1), isrD1, FALLING);
    pinMode(D2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(D2), isrD2, FALLING);

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(ledMatrix[0], ledMatrix.Size() + 4).setCorrection(TypicalLEDStrip);
    showWord("WIFI", 4);

#ifdef DEBUG
    Serial.print("Firmware version: ");
    Serial.println(VERSION);
    Serial.printf("Connecting to WiFi: %s: %s \n", WiFi.SSID().c_str(), WiFi.psk().c_str());
#endif

    WiFi.persistent(true);
    WiFi.begin();
    while (!WiFi.isConnected()) {
#ifdef DEBUG
        Serial.println("WiFi not yet connected...");
#endif
    }
    
    espClient.setFingerprint(SSL_FINGERPRINT);

    configTime(MY_TZ, MY_NTP_SERVER);
    updateTime();
    delay(100);

    ESPhttpUpdate.onStart(callbackUpdateStarted);
    ESPhttpUpdate.onProgress(callbackUpdateProgress);
    ESPhttpUpdate.onEnd(callbackUpdateEnd);
    ESPhttpUpdate.onError(callbackUpdateError);
    update();

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    ScrollingMsg.SetFont(MatriseFontData);
    ScrollingMsg.Init(&ledMatrix, ledMatrix.Width(), ScrollingMsg.FontHeight() + 1, 0, 0);
    ScrollingMsg.SetTextColrOptions(COLR_GRAD_AH, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00);

    prepareMessage(VERSION, sizeof(VERSION));
    showText(false);

    reconnectMqtt();
}

void loop() {
#ifdef DEBUG
    Serial.printf("WiFi status: %d \n", WiFi.status());
#endif
    updateTime();
    handleClock();
    adjustBrightness();
    reconnectMqtt();
    mqttClient.loop();
    if (millis() % 1000 < 2) update();
    handleButtons();
    FastLED.delay(1000);
    if (mqttMessage.available) showText(true);
}