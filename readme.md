# mpack

c++20 implementation for [msgpack](https://github.com/msgpack/msgpack/blob/master/spec.md)

"taste in design is subjective, with the exception of my own, which is unparalleled in its sophistication" - [hoog](https://www.youtube.com/@hoogyoutube)

```cpp
struct vec3 {
  float x;
  std::string y;
  uint8_t z;
};

int main() {
  vec3 v{1.2, "727", 0};
  msgpack::Buffer blob = msgpack::pack(v);
  std::optional<vec3> = msgpack::unpack<vec3>(blob);
}
```
