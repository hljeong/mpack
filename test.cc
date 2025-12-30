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

  assert(!unpack<uint32_t>(pack(-7)));
}
