#ifndef XTF_X86_SEV_H
#define XTF_X86_SEV_H

#include <xen/xen.h>
#include <arch/lib.h>

static inline uint64_t c_bit_mask(void)
{
    unsigned int enc_bit;

    enc_bit = cpuid_ebx(0x8000000) & 0x3f;

    return 1ULL << enc_bit;
}

#endif /* XTF_X86_SEV_H */

