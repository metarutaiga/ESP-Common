#pragma once

#include <WiFiClientSecure.h>
#include <ESP8266httpUpdate.h>

// DigiCert High Assurance EV Root CA
const char trustRoot[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

void HTTPSupdatefirmware(const char* name, const char* host, const char* path, WiFiClientSecure& client) {
  messageSerial.println(F("Starting OTA : "));
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, (String(F("https://")) + host + path + name).c_str());
  switch (ret) {
  case HTTP_UPDATE_FAILED:
    messageSerial.printf_P(PSTR("HTTP_UPDATE_FAILD Error (%d): %s\n"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    messageSerial.println(F("HTTP_UPDATE_NO_UPDATES"));
    break;

  case HTTP_UPDATE_OK:
    messageSerial.println(F("HTTP_UPDATE_OK"));
    break;
  } 
}

void HTTPSupdatefile(const char* name, const char* host, const char* path, WiFiClientSecure& client) {

  // Header
  int length = 0;
  {
    String get;
    get += F("GET ");
    get += FPSTR(path);
    get += FPSTR(name);
    get += F(" HTTP/1.1");
    get += F("\r\n");
    get += F("Host: ");
    get += FPSTR(host);
    get += F("\r\n");
    get += F("Connection: keep-alive");
    get += F("\r\n");
    get += F("\r\n");
    client.print(get);
    messageSerial.printf_P(PSTR("%s\n"), name);
  }
  for (int i = 0; i < 2000; i += 10) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    if (line.indexOf(F("Content-Length")) != -1) {
      length = strtol(line.c_str() + sizeof("Content-Length:"), nullptr, 10);
    }
    delay(10);
  }
  messageSerial.printf_P(PSTR("%d\n"), length);
  if (length == 0) {
    return;
  }

  // Buffer
  const int chunk = 256;
  byte* buffers[64] = {};

  // Data
  int count = 0;
  for (int i = 0; i < 2000; i += 10) {
    if (client.connected() == false)
      break;
    int c = client.read();
    if (c < 0) {
      delay(10);
      continue;
    }
    if (buffers[count / chunk] == nullptr) {
      buffers[count / chunk] = (byte*)malloc(chunk);
      messageSerial.printf_P(PSTR("%d/%d\n"), count, length);
    }
    buffers[count / chunk][count % chunk] = c;
    count++;
    if (count == length) {
      break;
    }
    i = 0;
  }

  // Directory
  LittleFS.mkdir(String(F("files")).c_str());

  // Update
  bool update = (count != 0);
  if (File file = LittleFS.open((String(F("files")) + F("/") + name).c_str(), "r")) {
    update = false;
    for (int i = 0; i < count; ++i) {
      if (buffers[i / chunk][i % chunk] != file.read()) {
        update = true;
        break;
      }
    }
  }
  if (update) {
    if (File file = LittleFS.open((String(F("files")) + F("/") + name).c_str(), "w")) {
      for (int i = 0; i < 64; ++i) {
        if (count <= 0) {
          break;
        }
        messageSerial.printf_P(PSTR("%d/%d\n"), i * chunk, i * chunk + count);
        if (count >= chunk) {
          file.write(buffers[i], chunk);
        }
        else {
          file.write(buffers[i], count);
        }
        count -= chunk;
      }      
    }
  }
  for (int i = 0; i < 64; ++i) {
    free(buffers[i]);
  }
}

void HTTPSupdatelist(const char* name, const char* host, const char* path) {

  // Connect
  X509List cert(trustRoot);
  WiFiClientSecure client;
  client.setTrustAnchors(&cert);
  if (client.connect(String(FPSTR(host)), 443) == false) {
    messageSerial.printf_P(PSTR("%s\n"), PSTR("failed"));
    return;
  }
  messageSerial.printf_P(PSTR("%s\n"), PSTR("connected"));
  
  // Get List
  HTTPSupdatefile(name, host, path, client);
  if (File file = LittleFS.open((String(F("files")) + F("/") + name).c_str(), "r")) {
    bool updateFirmware = false;
    for (int i = 0; i < 1024; ++i) {
      String name = file.readStringUntil('\n');
      if (name.isEmpty())
        break;
      if (i == 0) {
        if (name != String(FPSTR(version))) {
          updateFirmware = true;
        }
        continue;
      }
      HTTPSupdatefile(name.c_str(), host, path, client);
    }
    if (updateFirmware) {
      HTTPSupdatefirmware(String(F("firmware.bin")).c_str(), host, path, client);
    }
  }
}
