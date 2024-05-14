#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include "time.h"

// WiFi connection
const char ssid[] = "xxxxxx";               // your wifi SSID
const char pass[] = "xxxxxx";              //  Your WiFi password
const char* NTP_POOL1 = "0.id.pool.ntp.org";  //-->> NTP Server 1
const char* NTP_POOL2 = "1.id.pool.ntp.org";  //-->> NTP Server 1
const long utcOffsetInSeconds = 25200;        //-->> Example : Indonesia GMT is +7.00  So 7*3600 = 25200


float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;
float sdeg = 0, mdeg = 0, hdeg = 0;
uint16_t osx = 120, osy = 120, omx = 120, omy = 120, ohx = 120, ohy = 120;
uint16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0;
uint8_t hh, mm, ss;
boolean initial = 1;


#define TFT_GREY 0x5AEB
TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(100); }

  Serial.println("Initialized TFT ...");
  tft.init();
  // tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  Serial.println("Initialized AP ...");

  tft.setCursor(0, 4, 4);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Initialised AP", tft.width() / 2, 100);

  ConnectToAP();
  delayMicroseconds(500);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("NTP Synchronizing", tft.width() / 2, 100);

  configTime(utcOffsetInSeconds, 0, NTP_POOL1, NTP_POOL2);
  Serial.print("Waiting for NTP time sync: ");
  printLocalTime();
  delayMicroseconds(500);

  
  tft.fillScreen(TFT_GREY);
  tft.setTextColor(TFT_WHITE, TFT_GREY);  // Adding a background colour erases previous text automatically

  // Draw clock face
  tft.fillCircle(120, 120, 118, TFT_GREEN);
  tft.fillCircle(120, 120, 110, TFT_BLACK);

  // Draw 12 lines
  for (int i = 0; i < 360; i += 30) {
    sx = cos((i - 90) * 0.0174532925);
    sy = sin((i - 90) * 0.0174532925);
    x0 = sx * 114 + 120;
    yy0 = sy * 114 + 120;
    x1 = sx * 100 + 120;
    yy1 = sy * 100 + 120;

    tft.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
  }

  // Draw 60 dots
  for (int i = 0; i < 360; i += 6) {
    sx = cos((i - 90) * 0.0174532925);
    sy = sin((i - 90) * 0.0174532925);
    x0 = sx * 102 + 120;
    yy0 = sy * 102 + 120;
    // Draw minute markers
    tft.drawPixel(x0, yy0, TFT_WHITE);

    // Draw main quadrant dots
    if (i == 0 || i == 180) tft.fillCircle(x0, yy0, 2, TFT_WHITE);
    if (i == 90 || i == 270) tft.fillCircle(x0, yy0, 2, TFT_WHITE);
  }

  tft.fillCircle(120, 121, 3, TFT_WHITE);
}
void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time, Waiting for NTP time sync: ");
    configTime(utcOffsetInSeconds, 0, NTP_POOL1, NTP_POOL2);
    printLocalTime();
    delay(1000);
  } else {
    hh = (timeinfo.tm_hour % 12 == 0) ? 12 : timeinfo.tm_hour % 12;  // convert 24h to 12h
    mm = timeinfo.tm_min;
    ss = timeinfo.tm_sec;

    sdeg = ss * 6;
    mdeg = mm * 6 + sdeg * 0.01666667;  // 0-59 -> 0-360 - includes seconds
    hdeg = hh * 30 + mdeg * 0.0833333;  // 0-11 -> 0-360 - includes minutes and seconds

    hx = cos((hdeg - 90) * 0.0174532925);
    hy = sin((hdeg - 90) * 0.0174532925);
    mx = cos((mdeg - 90) * 0.0174532925);
    my = sin((mdeg - 90) * 0.0174532925);
    sx = cos((sdeg - 90) * 0.0174532925);
    sy = sin((sdeg - 90) * 0.0174532925);

    if (ss == 0 || initial) {
      initial = 0;
      // Erase hour and minute hand positions every minute
      tft.drawLine(ohx, ohy, 120, 121, TFT_BLACK);
      ohx = hx * 62 + 121;
      ohy = hy * 62 + 121;
      tft.drawLine(omx, omy, 120, 121, TFT_BLACK);
      omx = mx * 84 + 120;
      omy = my * 84 + 121;
    }


    // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
    tft.drawLine(osx, osy, 120, 121, TFT_BLACK);
    osx = sx * 90 + 121;
    osy = sy * 90 + 121;
    tft.drawLine(osx, osy, 120, 121, TFT_RED);
    tft.drawLine(ohx, ohy, 120, 121, TFT_WHITE);
    tft.drawLine(omx, omy, 120, 121, TFT_WHITE);
    tft.drawLine(osx, osy, 120, 121, TFT_RED);
    tft.fillCircle(120, 121, 5, TFT_RED);
    delay(1000);
  }
}


void ConnectToAP() {
  Serial.println("Attempting to Connect");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }
}
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay, 10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
}
