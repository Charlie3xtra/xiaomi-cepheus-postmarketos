// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/of.h>
#include "overlay_blob.h"
static int overlay_id = -1;
static int __init overlay_init(void)
{
	struct device_node *n;
	u32 value;
	int ret;
	if (!of_machine_is_compatible("xiaomi,cepheus")) return -ENODEV;
	n = of_find_node_by_path("/soc@0/cci@ac4a000/i2c-bus@1/camera@1a");
	if (!n) return -ENODEV;
	ret = of_property_read_u32(n,"vio-supply",&value);
	of_node_put(n);
	if (ret || value != 195) return -EINVAL;
	n = of_find_node_by_phandle(190);
	ret = !n || !of_device_is_compatible(n,"qcom,sm8150-camcc");
	of_node_put(n);
	if (ret) return -EINVAL;
	n = of_find_node_by_path("/soc@0/rsc@18200000/pm8150l-rpmh-regulators/bob");
	ret = !n || n->phandle != 242;
	of_node_put(n);
	if (ret) return -EINVAL;
	return of_overlay_fdt_apply(overlay_blob, sizeof(overlay_blob), &overlay_id, NULL);
}
static void __exit overlay_exit(void)
{
	if (overlay_id >= 0) of_overlay_remove(&overlay_id);
}
module_init(overlay_init);
module_exit(overlay_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Temporary cepheus auxiliary camera device tree overlay");
