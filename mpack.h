#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace msgpack {

using Buffer = std::vector<std::byte>;
using BufferView = std::span<const std::byte>;

class Packer {
public:
  void push(std::byte b) {
    buffer.push_back(b);
  }

  template <class T>
  requires (!std::is_same_v<T, std::byte>) &&
           requires(T b) { static_cast<std::byte>(b); }
  void push(T b) {
    return push(static_cast<std::byte>(b));
  }

  const Buffer &get() const {
    return buffer;
  }

private:
  Buffer buffer;
};

class Unpacker {
public:
  Unpacker(const BufferView &buffer) : buffer(buffer) {}

  std::byte read() {
    if (off == buffer.size()) throw;
    return buffer[off++];
  }

  size_t size() const {
    return buffer.size() - off;
  }

  bool at_end() const {
    return off == buffer.size();
  }

private:
  BufferView buffer;
  size_t off = 0;
};

template <class T> std::optional<T> ok(const T &value) { return value; }

template <class T>
struct impl {
  static void pack(Packer &packer, const T &value);
  static std::optional<T> unpack(Unpacker &unpacker);
};

template <class T>
void pack_one(Packer &packer, const T &value) {
  impl<T>::pack(packer, value);
}

template <class ...Ts>
Buffer pack(const Ts &...values) {
  Packer packer;
  (pack_one<Ts>(packer, values), ...);
  return packer.get();
}

template <class T>
std::optional<T> unpack_one(Unpacker &unpacker) {
  return impl<T>::unpack(unpacker);
}

template <class T>
std::optional<T> unpack(const BufferView &buffer) {
  Unpacker unpacker(buffer);
  const auto result = unpack_one<T>(unpacker);
  return result && unpacker.at_end() ? result : std::nullopt;
}

template <class ...Ts>
requires (sizeof...(Ts) > 1)
std::optional<std::tuple<Ts...>> unpack(const BufferView &buffer) {
  Unpacker unpacker(buffer);
  const auto result = std::make_tuple(unpack_one<Ts>(unpacker)...);
  const bool ok = (std::get<std::optional<Ts>>(result) && ...) && unpacker.at_end();
  return ok ? std::make_tuple(*std::get<std::optional<Ts>>(result)...) : std::nullopt;
}

namespace format {

constexpr std::byte positive_fixint {0x00};
constexpr std::byte fixmap          {0x80};
constexpr std::byte fixarray        {0x90};
constexpr std::byte fixstr          {0xa0};
constexpr std::byte nil             {0xc0};
constexpr std::byte false_          {0xc2};
constexpr std::byte true_           {0xc3};
constexpr std::byte bin_8           {0xc4};
constexpr std::byte bin_16          {0xc5};
constexpr std::byte bin_32          {0xc6};
constexpr std::byte ext_8           {0xc7};
constexpr std::byte ext_16          {0xc8};
constexpr std::byte ext_32          {0xc9};
constexpr std::byte float_32        {0xca};
constexpr std::byte float_64        {0xcb};
constexpr std::byte uint_8          {0xcc};
constexpr std::byte uint_16         {0xcd};
constexpr std::byte uint_32         {0xce};
constexpr std::byte uint_64         {0xcf};
constexpr std::byte int_8           {0xd0};
constexpr std::byte int_16          {0xd1};
constexpr std::byte int_32          {0xd2};
constexpr std::byte int_64          {0xd3};
constexpr std::byte fixext_1        {0xd4};
constexpr std::byte fixext_2        {0xd5};
constexpr std::byte fixext_4        {0xd6};
constexpr std::byte fixext_8        {0xd7};
constexpr std::byte fixext_16       {0xd8};
constexpr std::byte str_8           {0xd9};
constexpr std::byte str_16          {0xda};
constexpr std::byte str_32          {0xdb};
constexpr std::byte array_16        {0xdc};
constexpr std::byte array_32        {0xdd};
constexpr std::byte map_16          {0xde};
constexpr std::byte map_32          {0xdf};
constexpr std::byte negative_fixint {0xe0};

}

}

// nil
template <>
inline void msgpack::impl<std::nullptr_t>::pack(Packer &packer, const std::nullptr_t &) {
  packer.push(msgpack::format::nil);
}

template<>
inline std::optional<std::nullptr_t> msgpack::impl<std::nullptr_t>::unpack(Unpacker &unpacker) {
  if (unpacker.at_end() || unpacker.read() != msgpack::format::nil) return std::nullopt;
  return ok(nullptr);
}

// bool
template <>
inline void msgpack::impl<bool>::pack(Packer &packer, const bool &value) {
  packer.push(value ? msgpack::format::true_ : msgpack::format::false_);
}

template<>
inline std::optional<bool> msgpack::impl<bool>::unpack(Unpacker &unpacker) {
  if (unpacker.at_end()) return std::nullopt;
  switch (unpacker.read()) {
    case msgpack::format::true_: return true;
    case msgpack::format::false_: return false;
    default: return std::nullopt;
  }
}

// int
template <>
inline void msgpack::impl<uint8_t>::pack(Packer &packer, const uint8_t &value) {
  pack_one(packer, static_cast<uint64_t>(value));
}

template<>
inline std::optional<uint8_t> msgpack::impl<uint8_t>::unpack(Unpacker &unpacker) {
  auto value = unpack_one<uint64_t>(unpacker);
  return value && (static_cast<uint8_t>(*value) == *value) ? ok(static_cast<uint8_t>(*value)) : std::nullopt;
}

template <>
inline void msgpack::impl<uint16_t>::pack(Packer &packer, const uint16_t &value) {
  pack_one(packer, static_cast<uint64_t>(value));
}

template<>
inline std::optional<uint16_t> msgpack::impl<uint16_t>::unpack(Unpacker &unpacker) {
  auto value = unpack_one<uint64_t>(unpacker);
  return value && (static_cast<uint16_t>(*value) == *value) ? ok(static_cast<uint16_t>(*value)) : std::nullopt;
}

template <>
inline void msgpack::impl<uint32_t>::pack(Packer &packer, const uint32_t &value) {
  pack_one(packer, static_cast<uint64_t>(value));
}

template<>
inline std::optional<uint32_t> msgpack::impl<uint32_t>::unpack(Unpacker &unpacker) {
  auto value = unpack_one<uint64_t>(unpacker);
  return value && (static_cast<uint32_t>(*value) == *value) ? ok(static_cast<uint16_t>(*value)) : std::nullopt;
}

template <>
inline void msgpack::impl<uint64_t>::pack(Packer &packer, const uint64_t &value) {
  constexpr uint64_t one = 1;
  if (value < (one << 7)) {
    packer.push(static_cast<uint8_t>(msgpack::format::positive_fixint) | value);
  } else if (value < (one << 8)) {
    packer.push(msgpack::format::uint_8);
    packer.push(value);
  } else if (value < (one << 16)) {
    packer.push(msgpack::format::uint_16);
    packer.push(value >> 8); packer.push(value >> 0);
  } else if (value < (one << 32)) {
    packer.push(msgpack::format::uint_32);
    packer.push(value >> 24); packer.push(value >> 16);
    packer.push(value >>  8); packer.push(value >>  0);
  } else {
    packer.push(msgpack::format::uint_64);
    packer.push(value >> 56); packer.push(value >> 48);
    packer.push(value >> 40); packer.push(value >> 32);
    packer.push(value >> 24); packer.push(value >> 16);
    packer.push(value >>  8); packer.push(value >>  0);
  }
}

template<>
inline std::optional<uint64_t> msgpack::impl<uint64_t>::unpack(Unpacker &unpacker) {
  if (unpacker.at_end()) return std::nullopt;

  auto read = [&]() { return static_cast<uint64_t>(unpacker.read()); };

  std::byte first_byte = unpacker.read();
  switch (first_byte) {
    case msgpack::format::uint_8: {
      return unpacker.at_end() ? std::nullopt : ok(read());
    }

    case msgpack::format::uint_16: {
      return unpacker.size() < 2 ? std::nullopt : ok((read() << 8) | (read() << 0));
    }

    case msgpack::format::uint_32: {
      return unpacker.size() < 4 ? std::nullopt : ok((read() << 24) | (read() << 16) |
                                                     (read() <<  8) | (read() <<  0));
    }

    case msgpack::format::uint_64: {
      return unpacker.size() < 8 ? std::nullopt : ok((read() << 56) | (read() << 48) |
                                                     (read() << 40) | (read() << 32) |
                                                     (read() << 24) | (read() << 16) |
                                                     (read() <<  8) | (read() <<  0));
    }

    default: {
      uint8_t value = static_cast<uint8_t>(first_byte);
      if ((value & 0b1000'0000) == 0) {
        return value;
      } else if (value == 0b1110'0000) {
        return 0;
      } else {
        return std::nullopt;
      }
    }
  }
}

template <>
inline void msgpack::impl<int8_t>::pack(Packer &packer, const int8_t &value) {
  pack_one<int64_t>(packer, value);
}

template<>
inline std::optional<int8_t> msgpack::impl<int8_t>::unpack(Unpacker &unpacker) {
  auto value = unpack_one<int64_t>(unpacker);
  return value && (static_cast<int8_t>(*value) == *value) ? ok(static_cast<int8_t>(*value)) : std::nullopt;
}

template <>
inline void msgpack::impl<int16_t>::pack(Packer &packer, const int16_t &value) {
  pack_one<int64_t>(packer, value);
}

template<>
inline std::optional<int16_t> msgpack::impl<int16_t>::unpack(Unpacker &unpacker) {
  auto value = unpack_one<int64_t>(unpacker);
  return value && (static_cast<int16_t>(*value) == *value) ? ok(static_cast<int16_t>(*value)) : std::nullopt;
}

template <>
inline void msgpack::impl<int32_t>::pack(Packer &packer, const int32_t &value) {
  pack_one<int64_t>(packer, value);
}

template<>
inline std::optional<int32_t> msgpack::impl<int32_t>::unpack(Unpacker &unpacker) {
  auto value = unpack_one<int64_t>(unpacker);
  return value && (static_cast<int32_t>(*value) == *value) ? ok(static_cast<int32_t>(*value)) : std::nullopt;
}

template <>
inline void msgpack::impl<int64_t>::pack(Packer &packer, const int64_t &value) {
  constexpr int64_t one = 1;
  const uint64_t uvalue = value;
  if (value >= 0) {
    pack_one<uint64_t>(packer, uvalue);
  } else if (value > (-one << 5)) {
    packer.push(static_cast<uint8_t>(msgpack::format::negative_fixint) | value);
  } else if (value >= (-one << 8)) {
    packer.push(msgpack::format::int_8);
    packer.push(uvalue);
  } else if (value >= (-one << 16)) {
    packer.push(msgpack::format::int_16);
    packer.push(uvalue >> 8); packer.push(uvalue >> 0);
  } else if (value >= (-one << 32)) {
    packer.push(msgpack::format::int_32);
    packer.push(uvalue >> 24); packer.push(uvalue >> 16);
    packer.push(uvalue >>  8); packer.push(uvalue >>  0);
  } else {
    packer.push(msgpack::format::int_64);
    packer.push(uvalue >> 56); packer.push(uvalue >> 48);
    packer.push(uvalue >> 40); packer.push(uvalue >> 32);
    packer.push(uvalue >> 24); packer.push(uvalue >> 16);
    packer.push(uvalue >>  8); packer.push(uvalue >>  0);
  }
}

template<>
inline std::optional<int64_t> msgpack::impl<int64_t>::unpack(Unpacker &unpacker) {
  if (unpacker.at_end()) return std::nullopt;

  auto read = [&]() { return static_cast<uint64_t>(unpacker.read()); };

  std::byte first_byte = unpacker.read();
  switch (first_byte) {
    case msgpack::format::uint_8: {
      return unpacker.at_end() ? std::nullopt : ok<int64_t>(read());
    }

    case msgpack::format::int_8: {
      return unpacker.at_end() ? std::nullopt : ok<int64_t>(static_cast<int8_t>(read()));
    }

    case msgpack::format::uint_16: {
      return unpacker.size() < 2 ? std::nullopt : ok<int64_t>((read() << 8) | (read() << 0));
    }

    case msgpack::format::int_16: {
      return unpacker.size() < 2 ? std::nullopt
                                 : ok<int64_t>(static_cast<int16_t>((read() << 8) | (read() << 0)));
    }

    case msgpack::format::uint_32: {
      return unpacker.size() < 4 ? std::nullopt : ok((read() << 24) | (read() << 16) |
                                                     (read() <<  8) | (read() <<  0));
    }

    case msgpack::format::int_32: {
      return unpacker.size() < 4 ? std::nullopt
                                 : ok<int64_t>(static_cast<int32_t>(read() << 24) | (read() << 16) |
                                                                   (read() <<  8) | (read() <<  0));
    }

    case msgpack::format::uint_64: {
      if (unpacker.size() < 8) return std::nullopt;
      uint64_t bits = (read() << 56) | (read() << 48) |
                      (read() << 40) | (read() << 32) |
                      (read() << 24) | (read() << 16) |
                      (read() <<  8) | (read() <<  0);
      return bits >= (static_cast<uint64_t>(1) << 63) ? std::nullopt : ok<int64_t>(bits);
    }

    case msgpack::format::int_64: {
      return unpacker.size() < 8 ? std::nullopt
                                 : ok<int64_t>((read() << 56) | (read() << 48) |
                                               (read() << 40) | (read() << 32) |
                                               (read() << 24) | (read() << 16) |
                                               (read() <<  8) | (read() <<  0));
    }

    default: {
      uint8_t value = static_cast<uint8_t>(first_byte);
      if ((value & 0b1000'0000) == 0) {
        return value;
      } else if ((value & 0b1110'0000) == 0b1110'0000) {
        return static_cast<int8_t>(value);
      } else {
        return std::nullopt;
      }
    }
  }
}
