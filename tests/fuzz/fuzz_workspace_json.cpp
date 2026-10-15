#include "fuzz_bodies.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    fuzz::workspaceJson(data, size);
    return 0;
}
