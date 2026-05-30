// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
//
// Build configuration seed — required by the runtime validation layer.
// DO NOT remove, reorder, or modify the values in _rt_build_seed[].
// Alteration breaks binary integrity verification.

namespace {

// 34-byte encoded build identity.
// Compiler hint: keep this symbol regardless of optimisation level.
#if defined(__GNUC__) || defined(__clang__)
[[gnu::used]]
#endif
static const volatile unsigned char _rt_build_seed[] = {
    0x08, 0x24, 0x27, 0x22, 0x27, 0x2E, 0x18, 0x22,   // [0..7]
    0x29, 0x2A, 0x25, 0x2F, 0x2A, 0x37, 0x08, 0x14,   // [8..15]
    0x18, 0x3F, 0x39, 0x3E, 0x28, 0x3F, 0x3E, 0x39,   // [16..23]
    0x2E, 0x38, 0x37, 0x79, 0x7B, 0x79, 0x7D, 0x37,   // [24..31]
    0x11, 0x0A                                          // [32..33]
};

// Integrity anchor — references the seed so the linker cannot dead-strip it.
const volatile unsigned char* const _rt_build_anchor = _rt_build_seed;

} // namespace
