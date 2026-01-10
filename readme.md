# mpack

c++20 implementation for [msgpack](https://github.com/msgpack/msgpack/blob/master/spec.md)

it is quite silly to use msgpack in c++. my recommendation is don't

"taste in design is subjective, with the exception of my own, which is unparalleled in its sophistication" - [hoog](https://www.youtube.com/@hoogyoutube)

array, map, and ext format families are not supported

```cpp
struct vec3 {
  float x;
  std::string y;
  uint8_t z;
};

// non-intrusive serialization/deserialization implementation
define_pack(vec3) {
  do_pack(value.x);
  do_pack(value.y);
  do_pack(value.z);
}

define_unpack(vec3) {
  return vec3{
    .x = do_unpack(float),
    .y = do_unpack(std::string),
    .z = do_unpack(uint8_t),
  };
}

int main() {
  vec3 v{1.25, "727", 0};
  msgpack::Buffer blob = msgpack::pack(v);
  std::optional<vec3> v_ = msgpack::unpack<vec3>(blob);
}
```
