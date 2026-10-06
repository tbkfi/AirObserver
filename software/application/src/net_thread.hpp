#pragma once

#include "context.hpp"
#include "sockets.hpp"

namespace sys {
namespace net {
    struct context {

        // zero-initialize
        char server[64] = {}; 
        int port = 0;
    };

    context& ctx (void);

    void thread ();
}   // net 
}   // sys

