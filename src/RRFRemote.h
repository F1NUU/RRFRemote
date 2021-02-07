#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <Preferences.h>
#include "font.h"

// Version

#define VERSION "0.2.2"

// Debug

#define DEBUG false

// Color (https://htmlcolorcodes.com/fr/)

typedef struct __attribute__((__packed__)) {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_t ;

const color_t TFT_BACK       = { 48,  48,  48};

/*
// Orange
const color_t TFT_FRONT      = {245, 176,  65};
const color_t TFT_HEADER     = {186,  74,   0};
*/

/*
// Vert
const color_t TFT_FRONT      = { 52, 174,  96};
const color_t TFT_HEADER     = { 20,  90,  50};
*/

// Bleu
const color_t TFT_FRONT      = { 52, 152, 219};
const color_t TFT_HEADER     = { 27,  79, 114};

/*
// Rouge
const color_t TFT_FRONT      = {231,  76,  60};
const color_t TFT_HEADER     = {120,  40,  31};
*/

// Icon

#define ICON_FONT &icon_works_webfont14pt7b
#define ICON_STAT 40
#define ICON_CLOCK 105
#define ICON_CALL 77
#define ICON_TOT 105
#define ICON_SETTING 106
#define ICON_BAT100 118
#define ICON_BAT075 117
#define ICON_BAT050 116
#define ICON_BAT025 115
#define ICON_BAT000 34
#define ICON_CHARGING 37

/*
#defile ICON_FONT       &PeaxDrawnIcons20pt7b
#define ICON_STAT       86
#define ICON_CLOCK      51
#define ICON_CALL       104
#define ICON_TOT        50
#define ICON_SETTING    35
#define ICON_BAT100     118
#define ICON_BAT075     117
#define ICON_BAT050     116 
#define ICON_BAT025     115
#define ICON_BAT000     34
#define ICON_CHARGING   37
*/

// Wifi

const char *ssid = "\\_(o)_/";
const char *password = "petitchaton";
WiFiClient client_rrftracker, client_hamqsl;

// Preferences

Preferences preferences;

// HTTP endpoint

String spotnik = "http://192.168.1.99:3000/";

String endpoint[] = {
  "http://rrf.f5nlg.ovh:8080/RRFTracker/RRF-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/TECHNIQUE-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/BAVARDAGE-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/LOCAL-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/INTERNATIONAL-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/FON-today/rrf_tiny.json"
};

// Scroll

TFT_eSprite img = TFT_eSprite(&M5.Lcd); // Create Sprite object "img" with pointer to "tft" object
String msg = "";
int pos;

// Misceleanous

const char *room[] = {"RRF", "TECHNIQUE", "BAVARDAGE", "LOCAL", "INTERNATIONAL", "FON"};
const char *dtmf[] = {"96", "98", "100", "101", "99", "97"};

String tmp_str;
String json_data = "", xml_data = "";
String json_data_new = "";

String date_str, date_str_old;
String emission_str, emission_str_old;
String link_total_str, link_total_str_old;
String link_actif_str, link_actif_str_old;
String tx_total_str, tx_total_str_old;

int room_current = 0;
int brightness = 32;
int transmit_on = 0, transmit_off = 0;
int reset = 0;
int alternance = 0;
int refresh = 0;
int type = 0;
int battery_current = -1;
int qsy = 0;

// Parse data
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// Compute interpolation
int interpolation(int value, int in_min, int in_max, int out_min, int out_max)
{
  if ((in_max - in_min) != 0)
  {
    return int((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
  }
  else
  {
    return 0;
  }
}

// Clear screen
void clear()
{
  msg = "";
  M5.lcd.clear();

  // Grey zone
  M5.Lcd.drawLine(0, 100, 320, 100, TFT_WHITE);
  M5.Lcd.fillRect(0, 101, 320, 239, M5.Lcd.color565(TFT_BACK.r, TFT_BACK.g, TFT_BACK.b));

  // Elsewhere
  M5.Lcd.fillRoundRect(0, 168, 150, 71, 4, TFT_WHITE);
  M5.Lcd.fillRoundRect(1, 169, 24, 69, 4, M5.Lcd.color565(TFT_FRONT.r, TFT_FRONT.g, TFT_FRONT.b));

  M5.Lcd.drawFastVLine(22, 169, 69, M5.Lcd.color565(TFT_FRONT.r, TFT_FRONT.g, TFT_FRONT.b));
  M5.Lcd.drawFastVLine(23, 169, 69, M5.Lcd.color565(TFT_FRONT.r, TFT_FRONT.g, TFT_FRONT.b));
  M5.Lcd.drawFastVLine(24, 169, 69, M5.Lcd.color565(TFT_FRONT.r, TFT_FRONT.g, TFT_FRONT.b));

  M5.Lcd.drawFastVLine(25, 169, 69, M5.Lcd.color565(TFT_BACK.r, TFT_BACK.g, TFT_BACK.b));

  M5.Lcd.drawFastVLine(96, 169, 69, M5.Lcd.color565(TFT_BACK.r, TFT_BACK.g, TFT_BACK.b));

  M5.Lcd.drawFastHLine(2, 182, 147, M5.Lcd.color565(TFT_BACK.r, TFT_BACK.g, TFT_BACK.b));
  M5.Lcd.drawFastHLine(2, 196, 147, M5.Lcd.color565(TFT_BACK.r, TFT_BACK.g, TFT_BACK.b));
  M5.Lcd.drawFastHLine(2, 210, 147, M5.Lcd.color565(TFT_BACK.r, TFT_BACK.g, TFT_BACK.b));
  M5.Lcd.drawFastHLine(2, 224, 147, M5.Lcd.color565(TFT_BACK.r, TFT_BACK.g, TFT_BACK.b));

  // Log
  M5.Lcd.fillRoundRect(160, 117, 160, 122, 4, TFT_WHITE);
}

// Manage buttons
void button() {
  M5.update();  // Read the state of button

  if(M5.BtnA.isPressed() && M5.BtnC.isPressed()) {
    M5.Power.deepSleep();
  } else if(M5.BtnB.isPressed() && M5.BtnA.isPressed()) {
    brightness--;
    if (brightness <= 5) {
      brightness = 5;
    }
  } else if(M5.BtnB.isPressed() && M5.BtnC.isPressed()) {
    brightness++;
    if (brightness > 250) {
      brightness = 250;
    }
  } else {
    if (M5.BtnA.wasPressed()) {
      room_current -= 1;
      refresh = 0;
      reset = 0;
    } else if (M5.BtnC.wasPressed()) {
      room_current += 1;
      refresh = 0;
      reset = 0;
    } else if (M5.BtnB.wasPressed()) {
      qsy = 1;
    }
  }

  preferences.putUInt("brightness", brightness);
}

// Build scroll
void build_scroll()
{
  int h = 20;
  int w = M5.Lcd.width();

  // We could just use fillSprite(color) but lets be a bit more creative...

  while (h--)
    img.drawFastHLine(0, h, w, TFT_BLACK);

  // Now print text on top of the graphics
  img.setTextSize(1);          // Font size scaling is x1
  img.setTextFont(2);          // Font 4 selected
  img.setTextColor(TFT_WHITE); // Black text, no background colour
  img.setTextWrap(false);      // Turn of wrap so we can print past end of sprite

  // Need to print twice so text appears to wrap around at left and right edges
  img.setCursor(pos, 2); // Print text at xpos
  img.print(msg);

  img.setCursor(pos - w, 2); // Print text at xpos - sprite width
  img.print(msg);
}

// Scroll
void scroll(int pause)
{
  // Sprite for scroll

  build_scroll();
  img.pushSprite(0, 78);

  pos -= 1;
  if (pos == 0)
  {
    pos = M5.Lcd.width();
  }

  delay(pause);
}

// Get data from RRFTracker and manage QSY
void rrftracker(void *pvParameters)
{
  HTTPClient http;
  unsigned long timer = 0, wait = 0, limit = 750;

  for (;;)
  {
    timer = millis();

    if (room_current < 0)
    {
      room_current = 5;
    }
    else if (room_current > 5)
    {
      room_current = 0;
    }

    preferences.putUInt("room_current", room_current);

    if ((WiFi.status() == WL_CONNECTED)) // Check the current connection status
    {                                             

      if (qsy == 1) {
        http.begin(client_rrftracker, spotnik + String("?dtmf=") + String(dtmf[room_current]));  // Specify the URL
        http.addHeader("Content-Type", "text/plain"); // Specify content-type header
        http.setTimeout(500);                         // Set timeout
        int httpCode = http.GET();                    // Make the request
        if (httpCode < 0)                             // Check for the returning code
        {                                 
          qsy = 0;                                                          
        }
        http.end(); // Free the resources
      }

      http.begin(client_rrftracker, endpoint[room_current]);  // Specify the URL
      http.addHeader("Content-Type", "text/plain");   // Specify content-type header
      http.setTimeout(500);                           // Set Time Out
      int httpCode = http.GET();                      // Make the request
      if (httpCode > 0)                               // Check for the returning code
      {                                                                               
        json_data_new = http.getString(); // Get data
      }
      http.end(); // Free the resources
    }

    wait = millis() - timer;
    if (wait < limit)
    {
      vTaskDelay(pdMS_TO_TICKS(limit - wait));
    }
  }
}

// Get data from HamQSL
void hamqsl(void * pvParameters) {
  HTTPClient http;
  unsigned long timer = 0, wait = 0, limit = 60*60*1000;    // Retreive all hours

  for(;;) {
    timer = millis();

    if ((WiFi.status() == WL_CONNECTED)) // Check the current connection status
    {                                             
      http.begin(client_hamqsl, "http://www.hamqsl.com/solarxml.php");  // Specify the URL
      http.addHeader("Content-Type", "text/plain"); // Specify content-type header
      http.setTimeout(2000);                        // Set Time Out
      int httpCode = http.GET();                    // Make the request
      if (httpCode > 0)                             // Check for the returning code
      {                                                                               
        xml_data = http.getString();  // Get data
      }
      http.end(); // Free the resources
    }

    wait = millis() - timer;
    if (wait < limit)
    {
      vTaskDelay(pdMS_TO_TICKS(limit - wait));
    }
  }
}