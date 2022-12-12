
#include "MemoryBadAlloc.h"

const char * MemoryBadAlloc::what() const noexcept {
    return "Fatal: does not have enough memory\n";
}