#pragma once

#include <ESP8266WiFi.h>
#define private public
#include <PubSubClient.h> // see https://github.com/knolleary/pubsubclient
#undef private
class PubSubClient_P : public PubSubClient {
public:
  PubSubClient_P(Client& client) : PubSubClient(client) {}
  boolean publish(const char* topic, const char* payload, boolean retained = false) {
    if (connected()) {
      // Leave room in the buffer for header and variable length field
      uint16_t length = MQTT_MAX_HEADER_SIZE;
      length = writeString(topic,this->buffer,length);

      // Add payload
      uint16_t plength = strnlen_P(payload,this->bufferSize);
      memcpy_P(this->buffer+length,payload,plength);
      length += plength;

      // Write the header
      uint8_t header = MQTTPUBLISH;
      if (retained) {
        header |= 1;
      }
      if (write(header,this->buffer,length-MQTT_MAX_HEADER_SIZE)) {
        ((WiFiClient*)_client)->flush(5000);
        return true;
      }
    }
    return false;
  }
};

WiFiClient espClient;
PubSubClient_P MQTTclient(espClient);

const char* MQTTprefix(const __FlashStringHelper* prefix, ...) {
  static char path[128];
  char* pointer = path;
  pointer += sprintf_P(pointer, PSTR("%s/%s"), WiFi.hostname().c_str(), prefix);

  va_list args;
  va_start(args, prefix);
  while (const char* name = va_arg(args, char*)) {
    pointer += sprintf_P(pointer, PSTR("/%s"), name);
  }
  va_end(args);

  return path;
}

void MQTTinformation() {
  MQTTclient.publish(MQTTprefix(F("ESP"), F("Vcc"), 0), itoa(ESP.getVcc(), number, 10));
  MQTTclient.publish(MQTTprefix(F("ESP"), F("ChipId"), 0), itoa(ESP.getChipId(), number, 16));

  MQTTclient.publish(MQTTprefix(F("ESP"), F("SdkVersion"), 0), ESP.getSdkVersion());
  MQTTclient.publish(MQTTprefix(F("ESP"), F("CoreVersion"), 0), ESP.getCoreVersion().c_str());
  MQTTclient.publish(MQTTprefix(F("ESP"), F("FullVersion"), 0), ESP.getFullVersion().c_str());

  MQTTclient.publish(MQTTprefix(F("ESP"), F("BootVersion"), 0), itoa(ESP.getBootVersion(), number, 10));
  MQTTclient.publish(MQTTprefix(F("ESP"), F("BootMode"), 0), itoa(ESP.getBootMode(), number, 10));

  MQTTclient.publish(MQTTprefix(F("ESP"), F("CpuFreq"), 0), itoa(ESP.getCpuFreqMHz(), number, 10));

  MQTTclient.publish(MQTTprefix(F("ESP"), F("FlashChipId"), 0), itoa(ESP.getFlashChipId(), number, 16));
  MQTTclient.publish(MQTTprefix(F("ESP"), F("FlashChipVendorId"), 0), itoa(ESP.getFlashChipVendorId(), number, 16));

  MQTTclient.publish(MQTTprefix(F("ESP"), F("FlashChipRealSize"), 0), itoa(ESP.getFlashChipRealSize(), number, 10));
  MQTTclient.publish(MQTTprefix(F("ESP"), F("FlashChipSize"), 0), itoa(ESP.getFlashChipSize(), number, 10));
  MQTTclient.publish(MQTTprefix(F("ESP"), F("FlashChipSpeed"), 0), itoa(ESP.getFlashChipSpeed(), number, 10));
  MQTTclient.publish(MQTTprefix(F("ESP"), F("FlashChipMode"), 0), itoa(ESP.getFlashChipMode(), number, 10));
  MQTTclient.publish(MQTTprefix(F("ESP"), F("FlashChipSizeByChipId"), 0), itoa(ESP.getFlashChipSizeByChipId(), number, 10));

  MQTTclient.publish(MQTTprefix(F("ESP"), F("SketchSize"), 0), itoa(ESP.getSketchSize(), number, 10));
  MQTTclient.publish(MQTTprefix(F("ESP"), F("SketchMD5"), 0), ESP.getSketchMD5().c_str());
  MQTTclient.publish(MQTTprefix(F("ESP"), F("FreeSketchSpace"), 0), itoa(ESP.getFreeSketchSpace(), number, 10));

  MQTTclient.publish(MQTTprefix(F("ESP"), F("ResetReason"), 0), ESP.getResetReason().c_str());
  MQTTclient.publish(MQTTprefix(F("ESP"), F("ResetInfo"), 0), ESP.getResetInfo().c_str());

  MQTTclient.publish(MQTTprefix(F("ESP"), F("Build"), 0), PSTR(__DATE__ " " __TIME__), true);
  MQTTclient.publish(MQTTprefix(F("ESP"), F("Version"), 0), PSTR(VERSION), true);
}

void MQTTupdate() {
  static unsigned long loopHeapMillis = 0;
  if (loopHeapMillis < millis()) {
    loopHeapMillis = millis() + 1000 * 10;

    // Time
    static int now_Minutes = -1;
    time_t t = time(nullptr);
    struct tm* tm = localtime(&t);
    int minutes = tm->tm_min;
    if (now_Minutes != minutes) {
      now_Minutes = minutes;
      int hours = tm->tm_hour;
      sprintf_P(number, PSTR("%02d:%02d"), hours, minutes);
      MQTTclient.publish(MQTTprefix(F("ESP"), F("Time"), 0), number);
    }

    // Heap
    static int now_FreeHeap = -1;
    int freeHeap = ESP.getFreeHeap();
    if (now_FreeHeap != freeHeap) {
      now_FreeHeap = freeHeap;
      MQTTclient.publish(MQTTprefix(F("ESP"), F("FreeHeap"), 0), itoa(now_FreeHeap, number, 10));
    }

    // RSSI
    static int now_RSSI = -1;
    int rssi = WiFi.RSSI();
    if (now_RSSI != rssi) {
      now_RSSI = rssi;
      MQTTclient.publish(MQTTprefix(F("ESP"), F("RSSI"), 0), itoa(rssi, number, 10));
    } 
  }
}

// https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
bool MQTTreconnect(const char* hostname, bool wait) {
  // Loop until we're reconnected
  while (!MQTTclient.connected()) {
    messageSerial.print(F("Attempting MQTT connection..."));
    // Attempt to connect
    if (MQTTclient.connect(hostname, MQTTprefix(F("connected"), 0), 0, true, String(F("false")).c_str())) {
      messageSerial.println(F("connected"));
      MQTTclient.publish(MQTTprefix(F("connected"), 0), String(F("true")).c_str(), true);
      MQTTclient.publish(MQTTprefix(F("ESP"), F("IP"), 0), WiFi.localIP().toString().c_str(), true);
      MQTTclient.subscribe(MQTTprefix(F("set"), F("#"), 0));
      MQTTclient.setBufferSize(512);
      MQTTinformation();
      return true;
    }
    else {
      messageSerial.print(F("failed, rc="));
      messageSerial.print(MQTTclient.state());
      if (wait == false) {
        messageSerial.println();
        break;
      }
      messageSerial.println(F(" try again in 5 seconds"));
      // Wait 5 seconds before retrying
      for (int i = 0; i < 5; ++i) {
        delay(1000);
#ifdef __ARDUINO_OTA_H
        ArduinoOTA.handle();
#endif
      }
    }
  }
  return false;
}
