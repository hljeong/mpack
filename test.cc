#include <cassert>

#include "mpack.h"

using namespace msgpack;

int main() {
  assert(unpack<uint64_t>(*pack(uint64_t(3))) == 3);
}
