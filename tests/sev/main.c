/**
 * @file tests/sev/main.c
 * @ref test-sev
 *
 * @page test-sev sev
 *
 * @todo Docs for test-sev
 *
 * @see tests/sev/main.c
 */
#include <xtf.h>
#include <xtf/hypercall.h>
#include <arch/sev.h>

const char test_title[] = "Test sev";

const char *secret_data = "This is some secret data";

void test_main(void)
{
    printk("(GUEST): C-bit mask: %"PRIx64"\n", c_bit_mask());

    printk("(GUEST): Secret data at %p\n", secret_data);
    printk("(GUEST): Secret data is: %s\n", secret_data);

    HYPERCALL2(long, __HYPERVISOR_sev_hox_demo_op, secret_data, strlen(secret_data));

    xtf_success(NULL);
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
