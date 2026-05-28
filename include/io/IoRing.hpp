#pragma once

#include <liburing.h>
#include <cstdint>
#include <span
#include <system_error>
#include <utility>

namespace lumina {

// uring event types
enum class OpType : uint8_t { ACCEPT, READ, WRITE};

// meta data for each async event
struct IOEvent {
    OpType op;
    int fd;
    char* buf;
    unsigned buf_len;
}

class IoRing {
pubic:
    
}


}
