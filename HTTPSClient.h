#pragma once

#include <WiFiClientSecure.h>

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

void HTTPSupdatefile(const char* name, const char* host, const char* path, int space = 4096) {

  // Connect
  X509List cert(trustRoot);
  WiFiClientSecure client;
  client.setTrustAnchors(&cert);
  messageSerial.printf_P(PSTR("%s\n"), name);
  if (client.connect(String(FPSTR(host)), 443) == false) {
    messageSerial.printf_P(PSTR("%s\n"), PSTR("failed"));
    return;
  }
  messageSerial.printf_P(PSTR("%s\n"), PSTR("connected"));

  // Header
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
    get += F("Connection: close");
    get += F("\r\n");
    get += F("\r\n");
    client.print(get);
  }
  for (int i = 0; i < 2000; i += 10) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      messageSerial.printf_P(PSTR("%s\n"), PSTR("header"));
      break;
    }
    delay(10);
  }

  // Buffer
  const int chunk = 1024;
  byte* buffers[16] = {};
  for (int i = 0; i < 16; ++i) {
    if (space <= 0) {
      break;
    }
    if (space >= chunk) {
      buffers[i] = (byte*)malloc(chunk);
    }
    else {
      buffers[i] = (byte*)malloc(space);
    }
    space -= chunk;
  }

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
    buffers[count / chunk][count % chunk] = c;
    count++;
    i = 0;
  }
  messageSerial.printf_P(PSTR("%d\n"), count);

  // Directory
  LittleFS.mkdir(String(FPSTR("files")).c_str());

  // Update
  bool update = (count != 0);
  File file = LittleFS.open((String(F("files")) + F("/") + name).c_str(), "r");
  if (file) {
    update = false;
    for (int i = 0; i < count; ++i) {
      if (buffers[i / chunk][i % chunk] != file.read()) {
        update = true;
        break;
      }
    }
    file.close();
  }
  if (update) {
    file = LittleFS.open((String(F("files")) + F("/") + name).c_str(), "w");
    if (file) {
      for (int i = 0; i < 16; ++i) {
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
      file.close();
    }
  }
  for (int i = 0; i < 16; ++i) {
    free(buffers[i]);
  }
}

void HTTPSupdatelist(const char* name, const char* host, const char* path, int space = 4096) {
  HTTPSupdatefile(name, host, path, space);
  File file = LittleFS.open((String(F("files")) + F("/") + name).c_str(), "r");
  if (file) {
    for (;;) {
      String name = file.readStringUntil('\n');
      if (name.isEmpty())
        break;
      HTTPSupdatefile(name.c_str(), host, path, space);
    }
    file.close();
  }
}
