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

#define HYP_COUNT 5000000UL

static inline uint64_t rdtscp(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");
    return ((uint64_t)hi << 32) | lo;
}

#if defined(CONFIG_HVM)
static inline
long xen_hypercall_vcpu_get_runstate_info(enum xen_hypercall_vendor vendor,
                                          struct vcpu_runstate_info *param,
                                          uint32_t vcpuid)
{
    register long reg0 __asm__("rax") = __HYPERVISOR_FASTABI_MASK | __HYPERVISOR_vcpu_op;
    register uint64_t reg1 __asm__("rdi") = 4;
    register uint64_t reg2 __asm__("rsi") = vcpuid;
    register uint64_t reg3 __asm__("r8");
    register uint64_t reg4 __asm__("r9");
    register uint64_t reg5 __asm__("r10");
    register uint64_t reg6 __asm__("r11");
    register uint64_t reg7 __asm__("r12");

    if ( vendor == Intel )
        __asm__ volatile ("vmcall" : "+r"(reg2), "=r"(reg3), "=r"(reg4), "=r"(reg5), "=r"(reg6), "=r"(reg7), "+r"(reg0)
                                   : "r"(reg1)
                                   : "memory");
    else
        __asm__ volatile ("vmmcall" : "+r"(reg2), "=r"(reg3), "=r"(reg4), "=r"(reg5), "=r"(reg6), "=r"(reg7), "+r"(reg0)
                                    : "r"(reg1)
                                    : "memory");

    param->state = reg2;
    param->state_entry_time = reg3;
    param->time[0] = reg4;
    param->time[1] = reg5;
    param->time[2] = reg6;
    param->time[3] = reg7;
    return reg0;
}
#else
static inline
long xen_hypercall_vcpu_get_runstate_info(enum xen_hypercall_vendor vendor,
                                          struct vcpu_runstate_info *param,
                                          uint32_t vcpuid)
{
    return -EINVAL;
}
#endif

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
    else
        has_fastabi = false;

    #ifdef CONFIG_HVM
    printk("Using traditionnal HVM ABI\n");
    #else
    printk("Using PV ABI\n");
    #endif
    
    uint64_t prev, end;
    
    vcpu_runstate_info_t runstate;
    hypercall_vcpu_op(VCPUOP_get_runstate_info, 0, &runstate);
    prev = runstate.time[RUNSTATE_running];

    vcpu_runstate_info_t ri;

    for (unsigned long i = 0; i < HYP_COUNT; i++)
        hypercall_vcpu_op(VCPUOP_get_runstate_info, 0, &ri);

    hypercall_vcpu_op(VCPUOP_get_runstate_info, 0, &runstate);
    end = runstate.time[RUNSTATE_running];

    printk("Latest recorded runstate: %d\n", ri.state);
    xtf_success("Average: %"PRIu64" ns/hypercall\n", (end - prev) / HYP_COUNT);

    if (!has_fastabi)
        return;

    printk("\nUsing FastABI (\"HVMv2\")\n");

    hypercall_vcpu_op(VCPUOP_get_runstate_info, 0, &runstate);
    prev = runstate.time[RUNSTATE_running];

    for (unsigned long i = 0; i < HYP_COUNT; i++)
        xen_hypercall_vcpu_get_runstate_info(Intel, &ri, 0);

    hypercall_vcpu_op(VCPUOP_get_runstate_info, 0, &runstate);
    end = runstate.time[RUNSTATE_running];
    
    printk("Latest recorded runstate: %d\n", runstate.state);
    xtf_success("Average: %"PRIu64" ns/hypercall\n", (end - prev) / HYP_COUNT);
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
