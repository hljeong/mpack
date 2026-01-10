#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include "mpack.h"

using namespace msgpack;

template <class ...Ts>
Buffer bytes(Ts... bytes) { return {std::byte(bytes)...}; }

std::string to_string(const Buffer &buffer) {
  std::stringstream ss;
  ss << '[';
  bool first = true;
  for (const std::byte b : buffer) {
    if (!first) ss << ", ";
    first = false;
    char b_str[5] = "0x00";
    snprintf(&b_str[2], 3, "%02x", std::to_integer<uint8_t>(b));
    ss << b_str;
  }
  ss << ']';
  return ss.str();
}

template <class T>
bool test(T value, const Buffer &expected) {
  const Buffer packed = pack(value);
  const std::string packed_str = to_string(packed);
  if (packed != expected) {
    std::cout << "pack(" << value << ") -> " << packed_str << ", expected " << to_string(expected) << std::endl << std::endl;
    return false;
  } else {
    std::cout << "pack(" << value << ") -> " << packed_str << std::endl;
  }

  const std::optional<T> unpacked_opt = unpack<T>(packed);
  if (!unpacked_opt) {
    std::cout << "failed to unpack " << packed_str << " (" << value << ")" << std::endl << std::endl;
    return false;
  }

  const T unpacked = *unpacked_opt;
  if (unpacked != value) {
    std::cout << "unpack(" << packed_str << ") -> " << unpacked << ", expected " << value << std::endl << std::endl;
    return false;
  } else {
    std::cout << "unpack(" << packed_str << ") -> " << unpacked << std::endl;
  }

  std::cout << std::endl;
  return true;
}

int main() {
  assert(test<bool>(true, bytes(0xc3)));
  assert(test<bool>(false, bytes(0xc2)));
  assert(test<uint64_t>(3, bytes(0x03)));
  assert(test<int>(-3, bytes(0xfd)));
  assert(test<std::nullptr_t>(nullptr, bytes(0xc0)));
  assert(test<float>(3.14159f, bytes(0xca, 0x40, 0x49, 0x0f, 0xd0)));
  assert(test<double>(3.14159265358979, bytes(0xcb, 0x40, 0x09, 0x21, 0xfb, 0x54, 0x44, 0x2d, 0x11)));
  assert(test<std::string>("", bytes(0xa0)));
  assert(test<std::string>("a", bytes(0xa1, 0x61)));
  // 31 chars
  assert(test<std::string>("abcdefghijklmnopqrstuvwxyzabcde",
                           bytes(0xbf, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
                                 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
                                 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
                                 0x78, 0x79, 0x7a, 0x61, 0x62, 0x63, 0x64, 0x65)));
  // 32 chars (str 8)
  assert(test<std::string>("abcdefghijklmnopqrstuvwxyzabcdef",
                           bytes(0xd9, 0x20, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
                                 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
                                 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
                                 0x77, 0x78, 0x79, 0x7a, 0x61, 0x62, 0x63, 0x64,
                                 0x65, 0x66)));
  // str 16 and str 32 are not tested for undisclosed reasons

  assert(!unpack<uint32_t>(pack(-7)));

  std::cout << "all tests passed" << std::endl;
}
