#pragma once

#include <coredecls.h>

void RTCupdate(uint32_t data[128]) {
  uint32_t first_crc = crc32(data + 0, 31 * sizeof(uint32_t), 0xFFFFFFFF);
  uint32_t second_crc = crc32(data + 32, 95 * sizeof(uint32_t), 0xFFFFFFFF);
  memcpy(data + 31, &first_crc, sizeof(uint32_t));
  memcpy(data + 127, &second_crc, sizeof(uint32_t));
  ESP.rtcUserMemoryWrite(0, data, 128 * sizeof(uint32_t));
}

void RTCreset() {
  uint32_t data[128];
  memset(data, 0xFF, 128 * sizeof(uint32_t));
  RTCupdate(data);
}

void RTCbegin() {
  bool update = false;
  uint32_t data[128];
  ESP.rtcUserMemoryRead(0, data, 128 * sizeof(uint32_t));
  uint32_t first_crc = crc32(data + 0, 31 * sizeof(uint32_t), 0xFFFFFFFF);
  if (memcmp(data + 31, &first_crc, sizeof(uint32_t)) != 0) {
    memset(data + 0, 0xFF, 31 * sizeof(uint32_t));
    update = true;
  }
  uint32_t second_crc = crc32(data + 32, 95 * sizeof(uint32_t), 0xFFFFFFFF);
  if (memcmp(data + 127, &second_crc, sizeof(uint32_t)) != 0) {
    memset(data + 32, 0xFF, 95 * sizeof(uint32_t));
    update = true;
  }
  if (update) {
    RTCupdate(data);
  }
}

void RTCread(int offset, void* value, int size) {
  uint32_t data[128];
  ESP.rtcUserMemoryRead(offset / 4, data, ((offset % 4 + size + 3) & ~3));
  memcpy(value, (char*)data + offset % 4, size);
}

void RTCwrite(int offset, void* value, int size) {
  uint32_t data[128];
  ESP.rtcUserMemoryRead(0, data, 128 * sizeof(uint32_t));
  memcpy((char*)data + offset, value, size);
  RTCupdate(data);
}
