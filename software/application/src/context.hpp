#pragma once

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

    int a;

private:
    ctx () = default;

    // static std::unique_ptr <ctx> ctx_ptr;
    // static inline ctx& instance;
};

} // namespace sys
