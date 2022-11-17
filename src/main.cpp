#include <Arduino.h>
#include <FastLED.h>
#include "ESP8266WiFi.h"
#include <time.h>
#include <credentials.h>



#define LED_PIN     2 //The data pin of the arduino
#define NUM_LEDS    114 //Numbers of LED
#define BRIGHTNESS  20 //Brightness of the LEDs
#define LED_TYPE    WS2812 //The type of the LED stripe
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// WiFi parameters
const char* ssid = WIFI_SSID;
const char* password = WIFI_PSK;

String hours, minutes, seconds;
int currentSecond, currentMinute, currentHour;

#define MY_NTP_SERVER "pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"  
time_t now;
tm tm;  

const char *matrix = 
"ESKISTLF3NF"
"ZEHNZWANZIG"
"DREIVIERTEL"
"TGNACJVORJM"
"HALBQZW2LFP"
"ZWEINSIEBEN"
"KDREIRHF3NF"
"ELFNEUNVIER"
"WACHTZEHNRS"
"BSECHSFMUHR";

void fadeAll() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].r = 0;
    leds[i].g = 0;
    leds[i].b = 0;

  }
  FastLED.show();
}

void showLed(int i, int r, int g, int b) {
  leds[i].r = r;
  leds[i].g = g;
  leds[i].b = b;
}

void wordES( int r, int g, int b) {
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
  showLed(56, r, g, b);
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

void showWord(char *word, int wordLength) {
  // int wordLength = (sizeof(word) / sizeof(char)) - 1;
  int lastPos = -1;
  int startIndex = 0;
  int pixel[wordLength];
  Serial.print("Trying to find word: ");
  Serial.println(word);

  for (int i = 0; i < wordLength; i++) {
    int pos = findCharPosition(word[i], startIndex);
    if (pos == -1) {
      return;
    }
    if (lastPos + 1 != pos && lastPos != -1) {
      startIndex = pos - i;
      i = -1;
      lastPos = -1;
      Serial.print("Offsetting position of first char '");
      Serial.print(word[0]);
      Serial.print("' to ");
      Serial.println(startIndex);
    } else {
      lastPos = pos;
      startIndex = pos + 1;
      pixel[i] = pos;
    }
  }
  for (int i = 0; i < wordLength; i++) {
      showLed(pixel[i], 100, 100, 100);
      Serial.print("Displaying char ");
      Serial.print(word[i]);
      Serial.print(" at position ");
      Serial.println(pixel[i]);
  }

  Serial.println("Done.");
}

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

  configTime(MY_TZ, MY_NTP_SERVER);

  fadeAll();

  // showWord("ES", 2);
  // showWord("IST", 3);
  // showWord("F3NF", 4);
  // showWord("ZWANZIG", 7);
  // showWord("SECHS", 5);
  // showWord("UHR", 3);
}

void loop() {
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  Serial.print("\thour:");
  Serial.print(tm.tm_hour);         // hours since midnight  0-23
  Serial.print("\tmin:");
  Serial.print(tm.tm_min);          // minutes after the hour  0-59
  Serial.print("\tsec:");
  Serial.print(tm.tm_sec);          // seconds after the minute  0-61*
  Serial.println();

  currentHour = tm.tm_hour;
  if (currentHour > 12) currentHour = currentHour - 12;
  currentMinute = tm.tm_min;
  currentSecond = tm.tm_sec;


  wordES(color(), color(), color());
  wordIST(color(), color(), color());
  wordUHR(color(), color(), color());

  if (currentMinute >= 2 && currentMinute < 7) {
    wordFUENF(color(), color(), color());
    wordNACH(color(), color(), color());
  }
  if (currentMinute >= 7 && currentMinute < 12) {
    wordZEHN(color(), color(), color());
    wordNACH(color(), color(), color());
  }
  if (currentMinute >= 12 && currentMinute < 17) {
    wordVIERTEL(color(), color(), color());
    wordNACH(color(), color(), color());
  }
  if (currentMinute >= 17 && currentMinute < 22) {
    wordZWANZIG(color(), color(), color());
    wordNACH(color(), color(), color());
  }
  if (currentMinute >= 22 && currentMinute < 27) {
    wordFUENF(color(), color(), color());
    wordVOR(color(), color(), color());
    wordHALB(color(), color(), color());
    currentHour += 1;
  }
  if (currentMinute >= 27 && currentMinute < 32) {
    wordHALB(color(), color(), color());
    currentHour += 1;
  }
  if (currentMinute >= 32 && currentMinute < 37) {
    wordFUENF(color(), color(), color());
    wordNACH(color(), color(), color());
    wordHALB(color(), color(), color());
    currentHour += 1;
  }
  if (currentMinute >= 37 && currentMinute < 42) {
    wordZWANZIG(color(), color(), color());
    wordVOR(color(), color(), color());
    currentHour += 1;
  }
  if (currentMinute >= 42 && currentMinute < 47) {
    wordDREI(color(), color(), color());
    wordVIERTEL(color(), color(), color());
    wordNACH(color(), color(), color());
  }
  if (currentMinute >= 47 && currentMinute < 52) {
    wordZEHN(color(), color(), color());
    wordVOR(color(), color(), color());
    currentHour += 1;
  }
  if (currentMinute >= 52 && currentMinute < 57) {
    wordZEHN(color(), color(), color());
    wordVOR(color(), color(), color());
    currentHour += 1;
  }
  if (currentMinute >= 57 && currentMinute <= 59 ) {
    currentHour += 1;
  }

  switch (currentHour) {
    case 1:
      wordEINS(color(), color(), color());
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
      wordZWOELF(color(), color(), color());
      break;
    case 0:
      wordZWOELF(color(), color(), color());
      break;
  }

  FastLED.show();

  delay(1000);
}