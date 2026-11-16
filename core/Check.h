// Slufter - contract checks that work with and without exception support
// Author: Jan de Kort
// Created: 2026-11-15
#pragma once

#include <cstdlib>
#include <stdexcept>

// Native builds throw std::logic_error; the WebAssembly build is compiled
// without exceptions and traps instead.
inline void require(bool condition, const char* message)
{
    if (!condition)
    {
#if defined(__cpp_exceptions)
        throw std::logic_error(message);
#else
        (void) message;
        std::abort();
#endif
    }
}
