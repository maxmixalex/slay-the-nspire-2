#pragma once
#include <stddef.h>

namespace spire::detail {
// A distinct placement argument keeps this freestanding overload independent
// of the hosted C++ <new> header, which the ARM/WASM toolchains do not ship.
struct InPlaceAddress { void* address; };
}

inline void* operator new(size_t, spire::detail::InPlaceAddress where) noexcept {
    return where.address;
}

namespace spire {
// Storage must be aligned for T and contain at least sizeof(T) bytes. The
// returned pointer starts the object's lifetime; no heap allocation occurs.
template<class T> inline T* value_construct_at(void* storage) noexcept {
    static_assert(__is_trivially_copyable(T), "State storage must be trivial");
    static_assert(noexcept(T{}), "State value initialization must not throw");
    return ::new (detail::InPlaceAddress{storage}) T{};
}

// Reconstruct directly at the old address. Assigning T{} can reserve an entire
// aggregate temporary on ARM's small stack. Value construction retains every
// default member initializer, including nonzero sentinels and nested arrays.
template<class T> inline void reset_in_place(T& value) noexcept {
    static_assert(__is_trivially_copyable(T), "State storage must be trivial");
    value.~T();
    value_construct_at<T>(static_cast<void*>(&value));
}
}
