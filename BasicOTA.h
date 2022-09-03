#pragma once

#include <ArduinoOTA.h>

// https://github.com/esp8266/Arduino/blob/master/libraries/ArduinoOTA/examples/BasicOTA/BasicOTA.ino
void OTAsetup(const char* hostname) {
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = F("sketch");
    } else { // U_FS
      type = F("filesystem");
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    messageSerial.println(String(F("Start updating ")) + type);
  });
  ArduinoOTA.onEnd([]() {
    messageSerial.println(F("\nEnd"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    messageSerial.printf_P(PSTR("Progress: %u%%\r"), (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    messageSerial.printf_P(PSTR("Error[%u]: "), error);
    if (error == OTA_AUTH_ERROR) {
      messageSerial.println(F("Auth Failed"));
    } else if (error == OTA_BEGIN_ERROR) {
      messageSerial.println(F("Begin Failed"));
    } else if (error == OTA_CONNECT_ERROR) {
      messageSerial.println(F("Connect Failed"));
    } else if (error == OTA_RECEIVE_ERROR) {
      messageSerial.println(F("Receive Failed"));
    } else if (error == OTA_END_ERROR) {
      messageSerial.println(F("End Failed"));
    }
  });
  ArduinoOTA.begin();
}
