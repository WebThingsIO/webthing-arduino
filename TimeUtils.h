// If we don't have any WiFi-compatible board
#if !defined(ESP32) && !defined(ESP8266) && !defined(SEEED_WIO_TERMINAL)

// TODO: Actually add an ethernet NTP client
String getTimeStampString() { dest = "1980-01-01T00:00:00+00:00"; }

// If we're good to use WiFi
#else
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and no
// offset
NTPClient timeClient(ntpUDP);

void beginTimeClient() {
  Serial.println("Starting NTP client...");
  timeClient.begin();
  timeClient.update();
}

String getTimeStampString() {
  timeClient.update();

  time_t rawtime = timeClient.getEpochTime();
  struct tm *ti;
  ti = localtime(&rawtime);

  uint16_t year = ti->tm_year + 1900;
  String yearStr = String(year);

  uint8_t month = ti->tm_mon + 1;
  String monthStr = month < 10 ? "0" + String(month) : String(month);

  uint8_t day = ti->tm_mday;
  String dayStr = day < 10 ? "0" + String(day) : String(day);

  uint8_t hours = ti->tm_hour;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  uint8_t minutes = ti->tm_min;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  uint8_t seconds = ti->tm_sec;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return yearStr + "-" + monthStr + "-" + dayStr + "T" + hoursStr + ":" +
         minuteStr + ":" + secondStr + "Z";
}

#endif
