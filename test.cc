#include <cassert>

#include "mpack.h"

using namespace msgpack;

int main() {
  assert(unpack(pack(3)) == 3);
}
