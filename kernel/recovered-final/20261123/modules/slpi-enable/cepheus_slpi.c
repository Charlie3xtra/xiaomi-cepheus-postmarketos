// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include "overlay_blob.h"

static int overlay_id = -1;
static int __init cepheus_slpi_init(void)
{
    struct device_node *n, *mem;
    struct resource r;
    int ret;

    if (!of_machine_is_compatible("xiaomi,cepheus"))
        return -ENODEV;
    n = of_find_node_by_path("/soc@0/remoteproc@2400000");
    if (!n)
        return -ENODEV;
    if (!of_device_is_compatible(n, "qcom,sm8150-slpi-pas") ||
        of_device_is_available(n)) {
        of_node_put(n);
        return -EINVAL;
    }
    mem = of_parse_phandle(n, "memory-region", 0);
    of_node_put(n);
    if (!mem)
        return -EINVAL;
    ret = of_address_to_resource(mem, 0, &r);
    if (!of_property_present(mem, "no-map"))
        ret = -EINVAL;
    of_node_put(mem);
    if (ret || r.start != 0x97b00000 || resource_size(&r) != 0x1400000)
        return -EINVAL;
    return of_overlay_fdt_apply(overlay_blob, sizeof(overlay_blob), &overlay_id, NULL);
}
static void __exit cepheus_slpi_exit(void)
{
    if (overlay_id >= 0)
        of_overlay_remove(&overlay_id);
}
module_init(cepheus_slpi_init);
module_exit(cepheus_slpi_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cepheus SLPI enablement with verified reserved-memory guard");
