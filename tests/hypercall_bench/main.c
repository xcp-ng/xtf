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

#include <xen/fastabi.h>
#include <xen/vcpu.h>

const char test_title[] = "Hypercall benchmark";

#define HYP_COUNT 1000000UL

struct vcpu_op_get_runstate_info
{
    unsigned long vcpuid;
    struct vcpu_runstate_info runstate;
};

static inline
long xen_hypercall_vcpu_get_runstate_info(enum xen_hypercall_vendor vendor,
                                          struct vcpu_op_get_runstate_info *param)
{
    register long reg0 __asm__("rax") = __HYPERVISOR_FASTABI_MASK | __HYPERVISOR_vcpu_op;
    register uint64_t reg1 __asm__("rdi") = 4;
    register uint64_t reg2 __asm__("rsi") = param->vcpuid;
    register uint64_t reg3 __asm__("r8");
    register uint64_t reg4 __asm__("r9");
    register uint64_t reg5 __asm__("r10");
    register uint64_t reg6 __asm__("r11");
    register uint64_t reg7 __asm__("r12");
    register uint64_t reg8 __asm__("r13");

    if ( vendor == Intel )
        __asm__ volatile ("vmcall" : "=r"(reg3), "=r"(reg4), "=r"(reg5), "=r"(reg6), "=r"(reg7), "=r"(reg8), "+r"(reg0)
                                   : "r"(reg1), "r"(reg2)
                                   : "memory");
    else
        __asm__ volatile ("vmmcall" : "=r"(reg3), "=r"(reg4), "=r"(reg5), "=r"(reg6), "=r"(reg7), "=r"(reg8), "+r"(reg0)
                                    : "r"(reg1), "r"(reg2)
                                    : "memory");

    param->runstate.state = reg3;
    param->runstate.state_entry_time = reg4;
    param->runstate.time[0] = reg5;
    param->runstate.time[1] = reg6;
    param->runstate.time[2] = reg7;
    param->runstate.time[3] = reg8;
    return reg0;
}

void test_main(void)
{
    int has_fastabi;
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
        
        if ( eax & XEN_HVM_CPUID_FASTABI )
            has_fastabi = true;
    }

    uint64_t prev = rdtsc();
    vcpu_runstate_info_t ri;
    printk("Using traditionnal HVM ABI\n");

    for (unsigned long i = 0; i < HYP_COUNT; i++)
        hypercall_vcpu_op(VCPUOP_get_runstate_info, 0, &ri);

    uint64_t end = rdtsc();

    printk("Latest recorded runstate: %d", ri.state);
    xtf_success("Took %"PRIu64"u cycles\n", end - prev);

    if (!has_fastabi)
        return;

    prev = rdtsc();
    printk("Using FastABI (\"HVMv2\")\n\n");
    struct vcpu_op_get_runstate_info op;
    op.vcpuid = 0;

    for (unsigned long i = 0; i < HYP_COUNT; i++)
        xen_hypercall_vcpu_get_runstate_info(Intel, &op);

    end = rdtsc();
    
    printk("Latest recorded runstate: %d", op.runstate.state);
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
