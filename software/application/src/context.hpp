#pragma once
#include "bme690.hpp"

// copywrong. made by Don Pablo

// store system context
// make it accessible from different parts of code, and make use of thread safe
// structures? like std::atomic
// It should be as generic as possible.
// avoid the use of C++ STL as much as possible, as we avoid dynamic allocation.

namespace sys {

class ctx {
public:
    ctx (ctx &other) = delete;
    void operator= (const ctx &) = delete;

    static ctx& instance ()
    {
        // no dynamic allocation AND lazy initialization!
        static ctx instance; 
        return instance;
    }

    // fields are accessed with -> 
    sensor::BME690::context* bme690_data;

private:
    ctx () = default;
};

} // namespace sys
