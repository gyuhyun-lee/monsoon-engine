#ifndef MONSOON_SIMD_H
#define MONSOON_SIMD_H

#if ARCH_ARM
#include <arm_neon.h>
#elif ARCH_X86_64
#include <immintrin.h>
#endif

#endif
