#include <span>
#include <string>
#include <variant>
#include <vector>

namespace msgpack {

using Buffer = std::vector<std::byte>;
using BufferView = std::span<const std::byte>;

struct Error : std::exception {
  std::string msg;
  explicit Error(std::string m) : msg(std::move(m)) {}
  const char* what() const noexcept override { return msg.c_str(); }
};

template <class T>
class Result {
public:
  constexpr Result(const T& value) : home(value), fine(true) {}
  constexpr Result(const Error& error) : ball(error), fine(false) {}

  ~Result() { if (fine) home.~T(); else ball.~Error(); }

  constexpr bool ok() const { return fine; }
  constexpr bool err() const { return !fine; }

  constexpr const T& value() && { if (!fine) throw ball; return home; }
  constexpr const T& operator*() && { return std::move(*this).value(); }
  constexpr const T* operator->() && { return &std::move(*this).value(); }

  constexpr const Error& error() && { if (fine) throw Error("there is no error"); return ball; }

private:
  union {
    T home;
    Error ball;
  };
  bool fine;
};


template <> class Result<void> {
public:
  constexpr Result() : fine(true) {}
  constexpr Result(const Error& error) : ball(error), fine(false) {}

  ~Result() { if (!fine) ball.~Error(); }

  constexpr bool ok() const { return fine; }
  constexpr bool err() const { return !fine; }

  // oh egads what are these. thank you generative large language model
  constexpr void value() && { if (!fine) throw ball;  }
  constexpr void operator*() && { return std::move(*this).value(); }

  constexpr const Error& error() const & { if (fine) { throw Error("there is no error"); } return ball; }

private:
  union { Error ball; };
  bool fine;
};

template <class T> Result<T> ok(const T &value) { return Result<T>(value); }
template <class T> Result<T> err(const std::string &msg) { return Error(msg); }

template <class T>
struct Type {
  Result<void> pack(Buffer &buffer);
  static Result<T> unpack(const BufferView &buffer, size_t off);
};

template <class T>
Result<Buffer> pack(const T &value) {
  Buffer buffer;
  auto result = Type<T>::pack(buffer);
  return result ? ok(buffer) : Result<Buffer>(result.err());
}

}
