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

  uint8_t readi() {
    return static_cast<uint8_t>(read());
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
  static bool pack(Packer &packer, const T &value);
  static std::optional<T> unpack(Unpacker &unpacker);
};

template <class T>
bool pack_one(Packer &packer, const T &value) {
  return impl<T>::pack(packer, value);
}

template <class ...Ts>
std::optional<Buffer> pack(const Ts &...values) {
  Packer packer;
  return (pack_one<Ts>(packer, values) && ...) ? ok(packer.get()) : std::nullopt;
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

}

template <>
inline bool msgpack::impl<uint64_t>::pack(Packer &packer, const uint64_t &value) {
  packer.push(value >> 56);
  packer.push(value >> 48);
  packer.push(value >> 40);
  packer.push(value >> 32);
  packer.push(value >> 24);
  packer.push(value >> 16);
  packer.push(value >>  8);
  packer.push(value >>  0);
  return true;
}

template<>
inline std::optional<uint64_t> msgpack::impl<uint64_t>::unpack(Unpacker &unpacker) {
  if (unpacker.size() < sizeof(uint64_t)) return std::nullopt;
  uint64_t value = 0;
  for (size_t i = 0; i < sizeof(uint64_t); i++) {
    value <<= 8;
    value |= unpacker.readi();
  }
  return value;
}
