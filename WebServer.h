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
    for (int i = 0; i < 5000; i += WIFICLIENT_MAX_FLUSH_WAIT_MS) {
      if (ESP8266WebServer::client().flush(WIFICLIENT_MAX_FLUSH_WAIT_MS))
        break;
#ifdef PubSubClient_h
      MQTTclient.loop();
#endif
    }
  }
};
ESP8266WebServer_R webServer;

void WEBroot() {
  String html;

  // Head
  html += F("<html>");
  html += F("<head>");
  html += F(  "<title>");
  html += WiFi.hostname();
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

  // IP
  String ip = WiFi.localIP().toString();
  String gateway = WiFi.gatewayIP().toString();
  String subnet = WiFi.subnetMask().toString();
  String dns = WiFi.dnsIP().toString();
#if 0
  if (File file = LittleFS.open(String(F("ip")).c_str(), String(F("r")).c_str())) {
    ip = file.readStringUntil('\n'); ssid.trim();
    gateway = file.readStringUntil('\n'); ssid.trim();
    subnet = file.readStringUntil('\n'); ssid.trim();
    dns = file.readStringUntil('\n'); ssid.trim();
  }
#endif
  html += F("<form method='get' action='ip'>");
  html += F(  "<label>IP</label>");
  html += F(  "<input name='ip' length=32 value='"); html += ip + F("'>");
  html += F(  "<br>");
  html += F(  "<label>Gateway</label>");
  html += F(  "<input name='gateway' length=32 value='"); html += gateway + F("'>");
  html += F(  "<br>");
  html += F(  "<label>Subnet</label>");
  html += F(  "<input name='subnet' length=32 value='"); html += subnet + F("'>");
  html += F(  "<br>");
  html += F(  "<label>DNS</label>");
  html += F(  "<input name='dns' length=32 value='"); html += dns + F("'>");
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
  String mqttPort = F("1883");
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
  String ntp = F("pool.ntp.org");
  String ntpZone = F("9");
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
  struct tm* tm = localtime(&t);
  html += String(tm->tm_year);
  html += '.';
  html += String(tm->tm_mon + 1);
  html += '.';
  html += String(tm->tm_mday);
  html += ' ';
  html += String(tm->tm_hour);
  html += ':';
  html += String(tm->tm_min);
  html += ':';
  html += String(tm->tm_sec);
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

  // IP
  webServer.on(F("/ip"), []() {
    String ip = webServer.arg(F("ip"));
    String gateway = webServer.arg(F("gateway"));
    String subnet = webServer.arg(F("subnet"));
    String dns = webServer.arg(F("dns"));
    if (File file = LittleFS.open(String(F("ip")).c_str(), String(F("w")).c_str())) {
      file.println(IPAddress::isValid(ip) ? ip : String());
      file.println(IPAddress::isValid(gateway) ? gateway : String());
      file.println(IPAddress::isValid(subnet) ? subnet : String());
      file.println(IPAddress::isValid(dns) ? dns : String());
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
