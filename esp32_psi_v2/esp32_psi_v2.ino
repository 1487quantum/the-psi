#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "gesture.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#define TFT_CS        5
#define TFT_RST       33 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC        27

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

String response = "";           //Server response
DynamicJsonDocument doc(3000);

int idx{0};
const char* rType[4] = {"psi_twenty_four_hourly", "pm10_twenty_four_hourly", "pm25_twenty_four_hourly", "so2_twenty_four_hourly"};
const char* rTypeName[4] = {"PSI", "PM10", "PM25", "SO2"};
const char* region[6] = {"national", "north", "south", "east", "west", "central"};
const char* regionName[6] = {"National", "North", "South", "East", "West", "Central"};

//int pReadings[4] = {0, 0, 0, 0};
int aReadings[6][4];

int idx_region{0};
int idx_type{0};
int gs_dir{ -1};

Gesture gs(GES_INT);

void dtext(const char *text, uint16_t color, int x, int y, int sz) {
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(sz);
  tft.setTextWrap(true);
  tft.print(text);
}

void getData(const char * request, String &response) {
  HTTPClient http;
  http.begin(request);                                                  // Start request
  http.GET();                                                           // HTTP GET request
  response = http.getString();                                          // Server response
  http.end();                                                           // Close connection
}

void dispDat(const char * lastUpdate) {
  tft.fillScreen(ST77XX_BLACK);
  char heading[50];
  char val[5];
  sprintf(heading, "%s:", regionName[idx_region], rType[idx_type]);
  sprintf(val, "%02d", aReadings[idx_region][idx_type]);
  dtext(heading, ST77XX_WHITE, 0, 0, 2);
  dtext(String(rTypeName[idx_type]).c_str(), ST77XX_WHITE, 0, 20, 2);
  dtext(val,  aReadings[idx_region][idx_type] < 51 ? ST77XX_GREEN : ( aReadings[idx_region][idx_type] < 201 ? ST77XX_WHITE : ST77XX_ORANGE), (tft.width() / 2) - 20, (tft.height() / 2) - 12, 4);

  //Split date
  char updDate[35];
  strncpy(updDate, lastUpdate, 35);
  char * tmp_d = strchr (updDate, 'T');  // search for T, then remove the back
  if (tmp_d) {
    *tmp_d = 0;
  }
  //Split time
  char * tmp_t = strchr (lastUpdate, 'T');  // search for T and get back
  char upFinal[20];
  sprintf(upFinal, "%s\n%s", updDate, tmp_t);
  dtext(upFinal, ST77XX_WHITE, 0, tft.height() - 18, 1);
}

void getUpdate() {
  tft.fillScreen(ST77XX_BLACK);
  dtext("Updating..", ST77XX_GREEN, 3, tft.width() / 2 - 5, 2);
  getData(api_url, response);

  //Parse JSON, read error if any
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  const char* lastUpdate = doc["items"][0]["update_timestamp"];

  for (int i{0}; i < 6; ++i)
    for (int j{0}; j < 4; ++j)
      aReadings[i][j] = doc["items"][0]["readings"][rType[j]][region[i]];

  //Print parsed value on Serial Monitor
  for (auto &i : aReadings) {
    for (auto &j : i)
      Serial.println(j);
    Serial.println();
  }
  Serial.print(lastUpdate);

  dispDat(lastUpdate);
}

void setup(void) {
  Serial.begin(9600);
  //Init TFT
  tft.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab
  tft.fillScreen(ST77XX_BLACK);
  dtext("The PSI", ST77XX_WHITE, 2, (tft.height() / 2) - 12, 3);

  //Init gesture
  if (!gs.startGesture()) {
    Serial.println("Gesture sensor not detected...");
    return;
  }

  //Initiate WiFi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  dtext("Connecting...", ST77XX_WHITE, 2, tft.height() - 12, 1);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //  Serial.print("WiFi connected, IP: ");
  //  Serial.println(WiFi.localIP());

  getUpdate();
}

void loop(void) {
  gs_dir = gs.getDir();
  if (gs_dir != -1) {
    Serial.println(idx_region);
    Serial.println(idx_type);
    switch (gs_dir) {
      case GESTURE_UP:
        Serial.println("Detected DOWN gesture");
        if (idx_region > 0 && idx_region <= 5) {
          idx_region--;
        } else if (idx_region == 0) {
          idx_region = 5;
        }
        break;

      case GESTURE_DOWN:
        Serial.println("Detected UP gesture");
        getUpdate();
        break;

      case GESTURE_LEFT:
        Serial.println("Detected RIGHT gesture");

        break;

      case GESTURE_RIGHT:
        Serial.println("Detected LEFT gesture");
        if (idx_type >= 0 && idx_type < 3) {
          idx_type++;
        } else if (idx_type == 3) {
          idx_type = 0;
        }

        break;

      default:
        // ignore
        break;
    }
    dispDat(doc["items"][0]["update_timestamp"]);
  }
  delay(100);
}
