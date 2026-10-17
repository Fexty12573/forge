// Forge's plugin ABI uses core registers for floats; SDK libm uses VFP registers.
// --wrap keeps the SDK imports distinct from these local calling-convention bridges.
#define SDK_UNARY(name, type) \
    extern type __real_##name(type) __attribute__((pcs("aapcs-vfp"))); \
    type __wrap_##name(type value) { return __real_##name(value); }

#define SDK_BINARY(name, type) \
    extern type __real_##name(type, type) __attribute__((pcs("aapcs-vfp"))); \
    type __wrap_##name(type a, type b) { return __real_##name(a, b); }

SDK_UNARY(sinf, float)
SDK_UNARY(logf, float)
SDK_BINARY(powf, float)
SDK_UNARY(cosf, float)
SDK_BINARY(fmodf, float)
SDK_UNARY(acosf, float)
SDK_UNARY(floorf, float)
SDK_UNARY(sqrtf, float)
SDK_UNARY(ceilf, float)
SDK_UNARY(log, double)
SDK_BINARY(pow, double)
SDK_UNARY(sqrt, double)
SDK_BINARY(atan2f, float)
