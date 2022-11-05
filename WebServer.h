#pragma once

#include <ESP8266WebServer.h>

class ESP8266WebServer_R : public ESP8266WebServer {
public:
  void removeAllHandler() {
    RequestHandlerType* handler = _firstHandler;
    while (handler) {
      RequestHandlerType* next = handler->next();
      delete handler;
      handler = next;
    }
    _currentHandler = nullptr;
    _firstHandler = nullptr;
    _lastHandler = nullptr;
  }
  void sendContent(const String& content) {
    ESP8266WebServer::sendContent(content);
    ESP8266WebServer::client().flush(5000);
  }
};
ESP8266WebServer_R webServer;

void WEBroot() {
  String html;

  // Head
  html += F("<html>");
  html += F("<head>");
  html += F(  "<title>");
  html += hostname;
  html += F(  "</title>");
  html += F("</head>");
  html += F("<body>");

  // Chunked
  webServer.chunkedResponseModeStart(200, F("text/html"));
  webServer.sendContent(html); html.clear();

  // SSID
  String ssid;
  if (File file = LittleFS.open(String(F("ssid")).c_str(), String(F("r")).c_str())) {
    ssid = file.readStringUntil('\n'); ssid.trim();
  }
  html += F("<form method='get' action='ssid'>");
  html += F(  "<label>SSID</label>");
  html += F(  "<input name='ssid' length=32 value='"); html += ssid + F("'>");
  html += F(  "<br>");
  html += F(  "<label>PASS</label>");
  html += F(  "<input type='password' name='pass' length=32>");
  html += F(  "<input type='submit'>");
  html += F("</form>");
  webServer.sendContent(html); html.clear();

  // OTA
  String ota;
  if (File file = LittleFS.open(String(F("ota")).c_str(), String(F("r")).c_str())) {
    ota = file.readStringUntil('\n'); ota.trim();
  }
  html += F("<form method='get' action='ota'>");
  html += F(  "<label>OTA</label>");
  html += F(  "<input name='ota' length=32 value='"); html += ota + F("'>");
  html += F(  "<input type='submit'>");
  html += F("</form>");
  webServer.sendContent(html); html.clear();

  // MQTT
#ifdef PubSubClient_h
  String mqtt;
  String mqttPort;
  if (File file = LittleFS.open(String(F("mqtt")).c_str(), String(F("r")).c_str())) {
    mqtt = file.readStringUntil('\n'); mqtt.trim();
    mqttPort = file.readStringUntil('\n'); mqttPort.trim();
  }
  html += F("<form method='get' action='mqtt'>");
  html += F(  "<label>MQTT</label>");
  html += F(  "<input name='mqtt' length=32 value='"); html += mqtt + F("'>");
  html += F(  "<br>");
  html += F(  "<label>PORT</label>");
  html += F(  "<input name='port' length=32 value='"); html += mqttPort + F("'>");
  html += F(  "<input type='submit'>");
  html += F("</form>");
  webServer.sendContent(html); html.clear();
#endif

  // NTP
  String ntp;
  String ntpZone;
  time_t t = time(nullptr);
  if (File file = LittleFS.open(String(F("ntp")).c_str(), String(F("r")).c_str())) {
    ntp = file.readStringUntil('\n'); ntp.trim();
    ntpZone = file.readStringUntil('\n'); ntpZone.trim();
  }
  html += F("<form method='get' action='ntp'>");
  html += F(  "<label>NTP</label>");
  html += F(  "<input name='name' length=32 value='"); html += ntp + F("'>");
  html += F(  "<br>");
  html += F(  "<label>ZONE</label>");
  html += F(  "<input name='zone' length=32 value='"); html += ntpZone + F("'>");
  html += F(  "<input type='submit'>");
  html += F("</form>");
  html += asctime(localtime(&t));
  webServer.sendContent(html); html.clear();

  // Reset
  html += F("<form method='get' action='reset'>");
  html += F(  "<button type='submit'>Reset</button>");
  html += F("</form>");

  // Tail
  html += F("</body>");
  html += F("</html>");
  webServer.sendContent(html); html.clear();
  webServer.chunkedResponseFinalize();
}

void WEBsetup() {
  webServer.removeAllHandler();
  webServer.onNotFound([]() {
    webServer.send(404, F("text/plain"), F("404"));
  });

  // SSID
  webServer.on(F("/ssid"), []() {
    String ssid = webServer.arg(F("ssid"));
    String pass = webServer.arg(F("pass"));
    if (File file = LittleFS.open(String(F("ssid")).c_str(), String(F("w")).c_str())) {
      file.println(ssid);
      file.println(pass);
    }
    webServer.sendHeader(F("Location"), F("/"), true);
    webServer.send(302, F("text/plain"), F(""));
  });

  // OTA
  webServer.on(F("/ota"), []() {
    String ota = webServer.arg(F("ota"));
    if (File file = LittleFS.open(String(F("ota")).c_str(), String(F("w")).c_str())) {
      file.println(ota);
    }
    webServer.sendHeader(F("Location"), F("/"), true);
    webServer.send(302, F("text/plain"), F(""));
  });

  // MQTT
#ifdef PubSubClient_h
  webServer.on(F("/mqtt"), []() {
    String mqtt = webServer.arg(F("mqtt"));
    String port = webServer.arg(F("port"));
    if (File file = LittleFS.open(String(F("mqtt")).c_str(), String(F("w")).c_str())) {
      file.println(mqtt);
      file.println(port);
      MQTTclient.setServer(strdup(mqtt.c_str()), port.toInt());
    }
    webServer.sendHeader(F("Location"), F("/"), true);
    webServer.send(302, F("text/plain"), F(""));
  });
#endif

  // NTP
  webServer.on(F("/ntp"), []() {
    String name = webServer.arg(F("name"));
    String zone = webServer.arg(F("zone"));
    if (File file = LittleFS.open(String(F("ntp")).c_str(), String(F("w")).c_str())) {
      file.println(name);
      file.println(zone);
      configTime(zone.toInt() * 3600, 0, strdup(name.c_str()));
    }
    webServer.sendHeader(F("Location"), F("/"), true);
    webServer.send(302, F("text/plain"), F(""));
  });

  // Reset
  webServer.on(F("/reset"), []() {
    forceReset = true;
    webServer.sendHeader(F("Location"), F("/"), true);
    webServer.send(302, F("text/plain"), F(""));
  });

  // Format
  webServer.on(F("/format_fs"), []() {
    LittleFS.format();
    webServer.sendHeader(F("Location"), F("/"), true);
    webServer.send(302, F("text/plain"), F(""));
  });
}

void WEBloop() {
  webServer.handleClient();
}
