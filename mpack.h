#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <tuple>
#include <vector>

namespace msgpack {

using Buffer = std::vector<std::byte>;
using BufferView = std::span<const std::byte>;

class Packer {
public:
  void push(std::byte b) { buffer.push_back(b); }

  template <class T>
  requires (!std::is_same_v<T, std::byte>) &&
           requires(T b) { static_cast<std::byte>(b); }
  void push(T b) { return push(static_cast<std::byte>(b)); }

  const Buffer &get() const { return buffer; }

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

  size_t size() const { return buffer.size() - off; }
  bool at_end() const { return off == buffer.size(); }

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

template <class T> void pack_one(Packer &packer, const T &value) { impl<T>::pack(packer, value); }

template <class ...Ts>
Buffer pack(const Ts &...values) {
  Packer packer;
  (pack_one<Ts>(packer, values), ...);
  return packer.get();
}

template <class T> std::optional<T> unpack_one(Unpacker &unpacker) { return impl<T>::unpack(unpacker); }

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
  // use initializer list to force left-to-right eval
  const std::tuple<std::optional<Ts>...> result{unpack_one<Ts>(unpacker)...};
  const bool ok = (std::get<std::optional<Ts>>(result) && ...) && unpacker.at_end();
  if (ok) return {*std::get<std::optional<Ts>>(result)...};
  else return std::nullopt;
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

// syntactic sugar overdose :)
#define define_pack(T) template<> inline void msgpack::impl<T>::pack(Packer &packer, const T &value)
#define define_unpack(T) template<> inline std::optional<T> msgpack::impl<T>::unpack(Unpacker &unpacker)
#define $fail() return std::nullopt
#define $expect(cond) if (!static_cast<bool>(cond)) $fail()
#define $expect_read() ({ $expect(!unpacker.at_end()); unpacker.read(); })
#define $expect_byte(b) $expect($expect_read() == b)
#define $expect_in_urange(value, T) $expect((value) <= std::numeric_limits<T>::max())
#define $expect_in_range(value, T) $expect(std::numeric_limits<T>::min() <= (value) && (value) <= std::numeric_limits<T>::max())
#define $unwrap(opt_expr) ({ auto&& opt = (opt_expr); if (!opt) $fail(); *opt; })

// nil
define_pack(std::nullptr_t) { packer.push(format::nil); }
define_unpack(std::nullptr_t) { $expect_byte(format::nil); return nullptr; }

// bool
define_pack(bool) { packer.push(value ? format::true_ : format::false_); }
define_unpack(bool) {
  switch ($expect_read()) {
    case format::true_: return true;
    case format::false_: return false;
    default: $fail();
  }
}

// int
namespace msgpack::detail {

template <std::integral T> requires std::is_unsigned_v<T>
std::optional<T> unpack_uint(Unpacker &unpacker) {
  const uint64_t value = $unwrap(unpack_one<uint64_t>(unpacker));
  $expect_in_urange(value, T);
  return static_cast<T>(value);
}

template <std::integral T> requires std::is_signed_v<T>
std::optional<T> unpack_int(Unpacker &unpacker) {
  const int64_t value = $unwrap(unpack_one<int64_t>(unpacker));
  $expect_in_range(value, T);
  return static_cast<T>(value);
}

}

define_pack(uint8_t)    { pack_one<uint64_t>(packer, value); }
define_unpack(uint8_t)  { return detail::unpack_uint<uint8_t>(unpacker); }
define_pack(uint16_t)   { pack_one<uint64_t>(packer, value); }
define_unpack(uint16_t) { return detail::unpack_uint<uint16_t>(unpacker); }
define_pack(uint32_t)   { pack_one<uint64_t>(packer, value); }
define_unpack(uint32_t) { return detail::unpack_uint<uint32_t>(unpacker); }

define_pack(uint64_t) {
  constexpr uint64_t one = 1;
  if      (value < (one << 7))  goto pack8;
  else if (value < (one << 8))  { packer.push(format::uint_8);  goto pack8; }
  else if (value < (one << 16)) { packer.push(format::uint_16); goto pack16; }
  else if (value < (one << 32)) { packer.push(format::uint_32); goto pack32; }
  else                          { packer.push(format::uint_64); goto pack64; }

  return;

  pack64:
  packer.push(value >> 56); packer.push(value >> 48);
  packer.push(value >> 40); packer.push(value >> 32);
  pack32:
  packer.push(value >> 24); packer.push(value >> 16);
  pack16:
  packer.push(value >>  8);
  pack8:
  packer.push(value >>  0);
}

define_unpack(uint64_t) {
  auto read8 = [&]() { return std::to_integer<uint64_t>(unpacker.read()); };
  auto read16 = [&]() { return (read8()  <<  8) | read8(); };
  auto read32 = [&]() { return (read16() << 16) | read16(); };
  auto read64 = [&]() { return (read32() << 32) | read32(); };

  const std::byte first_byte = $expect_read();
  switch (first_byte) {
    case format::uint_8:   { $expect(unpacker.size() >= 1); return read8(); }
    case format::uint_16:  { $expect(unpacker.size() >= 2); return read16(); }
    case format::uint_32:  { $expect(unpacker.size() >= 4); return read32(); }
    case format::uint_64:  { $expect(unpacker.size() >= 8); return read64(); }

    default: {
      const uint8_t value = std::to_integer<uint8_t>(first_byte);
      if      ((value & 0b1000'0000) == 0)  return value;
      else if (value == 0b1110'0000)        return 0;
      else                                  $fail();
    }
  }
}

define_pack(int8_t)     { pack_one<int64_t>(packer, value); }
define_unpack(int8_t)   { return detail::unpack_int<int8_t>(unpacker); }
define_pack(int16_t)    { pack_one<int64_t>(packer, value); }
define_unpack(int16_t)  { return detail::unpack_int<int16_t>(unpacker); }
define_pack(int32_t)    { pack_one<int64_t>(packer, value); }
define_unpack(int32_t)  { return detail::unpack_int<int32_t>(unpacker); }

define_pack(int64_t) {
  constexpr int64_t one = 1;
  const uint64_t uvalue = value;
  if      (value >= 0)            pack_one<uint64_t>(packer, uvalue);
  else if (value >= (-one << 5))  packer.push(uvalue);
  else if (value >= (-one << 8))  { packer.push(format::int_8); goto pack8; }
  else if (value >= (-one << 16)) { packer.push(format::int_16); goto pack16; }
  else if (value >= (-one << 32)) { packer.push(format::int_32); goto pack32; }
  else                            { packer.push(format::int_64); goto pack64; }

  return;

  pack64:
  packer.push(uvalue >> 56); packer.push(uvalue >> 48);
  packer.push(uvalue >> 40); packer.push(uvalue >> 32);
  pack32:
  packer.push(uvalue >> 24); packer.push(uvalue >> 16);
  pack16:
  packer.push(uvalue >>  8);
  pack8:
  packer.push(uvalue >>  0);
}

define_unpack(int64_t) {
  auto read8 = [&]() { return std::to_integer<uint64_t>(unpacker.read()); };
  auto read16 = [&]() { return (read8()  <<  8) | read8(); };
  auto read32 = [&]() { return (read16() << 16) | read16(); };
  auto read64 = [&]() { return (read32() << 32) | read32(); };

  const std::byte first_byte = $expect_read();
  switch (first_byte) {
    case format::uint_8:   { $expect(unpacker.size() >= 1); return read8(); }
    case format::int_8:    { $expect(unpacker.size() >= 1); return static_cast<int8_t>(read8()); }
    case format::uint_16:  { $expect(unpacker.size() >= 2); return read16(); }
    case format::int_16:   { $expect(unpacker.size() >= 2); return static_cast<int16_t>(read16()); }
    case format::uint_32:  { $expect(unpacker.size() >= 4); return read32(); }
    case format::int_32:   { $expect(unpacker.size() >= 4); return static_cast<int32_t>(read32()); }
    case format::int_64:   { $expect(unpacker.size() >= 8); return read64(); }

    case format::uint_64: {
      $expect(unpacker.size() >= 8);
      const uint64_t bits = read64();
      $expect_in_range(bits, int64_t);
      return std::bit_cast<int64_t>(bits);
    }

    default: {
      const uint8_t value = std::to_integer<uint8_t>(first_byte);
      if      ((value & 0b1000'0000) == 0)            return value;
      else if ((value & 0b1110'0000) == 0b1110'0000)  return static_cast<int8_t>(value);
      else                                            $fail();
    }
  }

}

// the following technique is inspired by quantum bogosort
static_assert(sizeof(float) == 4);
static_assert(std::numeric_limits<float>::is_iec559);

define_pack(float) {
  packer.push(format::float_32);
  if constexpr (std::endian::native == std::endian::big) {
    for (const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&value), *end = ptr + 4; ptr < end; ptr++)
      packer.push(*ptr);
  } else {
    for (const uint8_t *start = reinterpret_cast<const uint8_t *>(&value), *ptr = start + 3; ptr >= start; ptr--)
      packer.push(*ptr);
  }
}

define_unpack(float) {
  auto read8 = [&]() { return std::to_integer<uint32_t>(unpacker.read()); };

  $expect_byte(format::float_32);
  $expect(unpacker.size() >= 4);
  if constexpr (std::endian::native == std::endian::big) {
    const uint32_t bit_value = read8() | (read8() << 8) | (read8() << 16) | (read8() << 24);
    return std::bit_cast<float>(bit_value);
  } else {
    const uint32_t bit_value = (read8() << 24) | (read8() << 16) | (read8() << 8) | read8();
    return std::bit_cast<float>(bit_value);
  }
}

static_assert(sizeof(double) == 8);
static_assert(std::numeric_limits<double>::is_iec559);

define_pack(double) {
  packer.push(format::float_64);
  if constexpr (std::endian::native == std::endian::big) {
    for (const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&value), *end = ptr + 8; ptr < end; ptr++)
      packer.push(*ptr);
  } else {
    for (const uint8_t *start = reinterpret_cast<const uint8_t *>(&value), *ptr = start + 7; ptr >= start; ptr--)
      packer.push(*ptr);
  }
}

define_unpack(double) {
  auto read8 = [&]() { return std::to_integer<uint64_t>(unpacker.read()); };

  $expect_byte(format::float_64);
  $expect(unpacker.size() >= 8);
  if constexpr (std::endian::native == std::endian::big) {
    const uint64_t bit_value = (read8() <<  0) | (read8() <<  8) | (read8() << 16) | (read8() << 24)
                             | (read8() << 32) | (read8() << 40) | (read8() << 48) | (read8() << 56);
    return std::bit_cast<double>(bit_value);
  } else {
    const uint64_t bit_value = (read8() << 56) | (read8() << 48) | (read8() << 40) | (read8() << 32)
                             | (read8() << 24) | (read8() << 16) | (read8() <<  8) | (read8() <<  0);
    return std::bit_cast<double>(bit_value);
  }
}

#undef $unwrap
#undef $expect_in_range
#undef $expect_in_urange
#undef $expect_byte
#undef $expect_read
#undef $expect
#undef $fail
#undef define_unpack
#undef define_pack
