/**
 * @file tests/hypercall_bench/main.c
 * @ref hypercall_benchmark - Hypercall benchmark test
 *
 * @page hypercall_benchmark Hypercall benchmark test.
 *
 * Perform a batch of VCPUOP_get_runstate_info hypercall to measure the
 * overhead of performing hypercalls.
 *
 * @see tests/hypercall_bench/main.c
 */
#include <stdint.h>
#include <xtf.h>

const char test_title[] = "Hypercall benchmark";

#define HYP_COUNT 1000000UL

void test_main(void)
{
    printk("Benchmarking %lu VCPUOP_get_runstate_info hypercalls\n", HYP_COUNT);

    if ( IS_DEFINED(CONFIG_HVM) )
    {
        uint32_t eax, ebx, ecx, edx, base;
        bool found = false;

        for ( base = XEN_CPUID_FIRST_LEAF;
              base < XEN_CPUID_FIRST_LEAF + 0x10000; base += 0x100 )
        {
            cpuid(base, &eax, &ebx, &ecx, &edx);

            if ( (ebx == XEN_CPUID_SIGNATURE_EBX) &&
                 (ecx == XEN_CPUID_SIGNATURE_ECX) &&
                 (edx == XEN_CPUID_SIGNATURE_EDX) &&
                 ((eax - base) >= 2) )
            {
                found = true;
                break;
            }
        }

        if ( !found )
            panic("Unable to locate Xen CPUID leaves\n");

        cpuid(base + 4, &eax, &ebx, &ecx, &edx);
        
        if ( eax & XEN_HVM_CPUID_PHYS_ADDR_ABI )
            printk("Using physical address ABI (\"HVMv2\")\n\n");
        else
            printk("Using traditionnal HVM ABI\n");
    }

    uint64_t prev = rdtsc();
    vcpu_runstate_info_t ri;

    for (unsigned long i = 0; i < HYP_COUNT; i++)
        hypercall_vcpu_op(VCPUOP_get_runstate_info, 0, &ri);

    uint64_t end = rdtsc();

    xtf_success("Took %"PRIu64"u cycles\n", end - prev);
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
