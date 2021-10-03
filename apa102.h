#pragma once

#include <string>
#include <vector>

#if __arm__
#include <wiringPi.h>
#include <wiringPiSPI.h>
#endif

namespace apa102 {

char const hex_chars[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

class LED {
 public:
  LED(int num_leds, int hz = 6000000) : _num_leds{num_leds} {
#if __arm__
    wiringPiSetup();
    if (wiringPiSPISetup(0, hz) < 0) {
      printf("wiringPiSPISetup failed\n");
    }
#endif
    int trailing = _num_leds / 16;
    if (_num_leds % 16 != 0) {
      ++trailing;
    }
    int capacity = 4 + 4 * _num_leds + trailing;
    _buf.resize(capacity);
    for (auto i = 0; i < _num_leds; ++i) {
      _buf[(1 + i) * 4] = 0xff;
    }
    for (auto i = 0; i < trailing; ++i) {
      _buf[(1 + _num_leds) * 4 + i] = 0xff;
    }
  }

  void set(int i, uint8_t r, uint8_t g, uint8_t b) {
    printf("%d: %d/%d/%d\n", i, (1 + i) * 4 + 1, (1 + i) * 4 + 2, (1 + i) * 4 + 3);
    _buf[(1 + i) * 4 + 1] = b;
    _buf[(1 + i) * 4 + 2] = g;
    _buf[(1 + i) * 4 + 3] = r;
  }

  void show() {
    std::string s;
    for (auto byte : _buf) {
      s += "\\x";
      s += hex_chars[(byte & 0xF0) >> 4];
      s += hex_chars[(byte & 0x0F) >> 0];
    }
    printf("%s\n", s.c_str());
#if __arm__
    wiringPiSPIDataRW(0, _buf, _buf.size());
#endif
  }

  int _num_leds;
  std::vector<uint8_t> _buf;
};

}  // namespace apa102
