#include <cassert>

#include "mpack.h"

using namespace msgpack;

int main() {
  assert(*unpack<bool>(pack(true)));
  assert(!*unpack<bool>(pack(false)));
  assert(unpack<uint64_t>(pack(3)) == 3);
  assert(unpack<int>(pack(-3)) == -3);
  assert(!unpack<uint32_t>(pack(-7)));
}
