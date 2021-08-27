#if ARCH_ARM
internal float32x4_t
#elif ARCH_X86_64
internal _mm_128
#endif
SetPs128(float value)
{
#if ARCH_ARM
    return float32x4_t
#elif ARCH_X86_64
    return vdupq_n_f32(value);
#endif
}

internal 
