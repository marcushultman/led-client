#include <string>

namespace encoding::base64 {

static constexpr auto kChars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline std::string encode(std::string_view buf) {
  std::string encoded;
  encoded.reserve(((buf.size() / 3) + (buf.size() % 3 > 0)) * 4);

  std::uint32_t temp{};
  auto it = buf.begin();

  for (std::size_t i = 0; i < buf.size() / 3; ++i) {
    temp = (*it++) << 16;
    temp += (*it++) << 8;
    temp += (*it++);
    encoded.append(1, kChars[(temp & 0x00FC0000) >> 18]);
    encoded.append(1, kChars[(temp & 0x0003F000) >> 12]);
    encoded.append(1, kChars[(temp & 0x00000FC0) >> 6]);
    encoded.append(1, kChars[(temp & 0x0000003F)]);
  }

  switch (buf.size() % 3) {
    case 1:
      temp = (*it++) << 16;
      encoded.append(1, kChars[(temp & 0x00FC0000) >> 18]);
      encoded.append(1, kChars[(temp & 0x0003F000) >> 12]);
      encoded.append(2, '=');
      break;
    case 2:
      temp = (*it++) << 16;
      temp += (*it++) << 8;
      encoded.append(1, kChars[(temp & 0x00FC0000) >> 18]);
      encoded.append(1, kChars[(temp & 0x0003F000) >> 12]);
      encoded.append(1, kChars[(temp & 0x00000FC0) >> 6]);
      encoded.append(1, '=');
      break;
  }

  return encoded;
}

}  // namespace encoding::base64
