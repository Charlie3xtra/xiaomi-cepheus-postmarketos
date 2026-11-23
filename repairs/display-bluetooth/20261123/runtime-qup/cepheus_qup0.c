// SPDX-License-Identifier: GPL-2.0-only
/* Experimental cepheus QUP0 enablement. Reboot to undo; intentionally no exit. */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include "overlay_blob.h"

#define QUP_PATH "/soc@0/geniqup@ac0000"
static int overlay_id = -1;

static bool already_created(struct device_node *np)
{
	struct platform_device *pdev = of_find_device_by_node(np);

	if (!pdev)
		return false;
	put_device(&pdev->dev);
	return true;
}

static int __init cepheus_qup0_init(void)
{
	struct device_node *qup, *child, *bt;
	const char *status;
	unsigned int enabled = 0;
	bool uart = false, debug = false;
	int ret = -EINVAL;

	if (!of_machine_is_compatible("xiaomi,cepheus"))
		return -ENODEV;
	qup = of_find_node_by_path(QUP_PATH);
	if (!qup)
		return -ENODEV;
	if (!of_device_is_compatible(qup, "qcom,geni-se-qup") ||
	    of_property_read_string(qup, "status", &status) ||
	    strcmp(status, "disabled") || !of_device_is_available(qup->parent) ||
	    !of_node_check_flag(qup->parent, OF_POPULATED_BUS) ||
	    already_created(qup))
		goto out;

	for_each_available_child_of_node(qup, child) {
		enabled++;
		if (already_created(child))
			goto put_child;
		if (of_node_name_eq(child, "serial") &&
		    !strcmp(kbasename(child->full_name), "serial@a84000") &&
		    of_device_is_compatible(child, "qcom,geni-uart")) {
			bt = of_get_child_by_name(child, "bluetooth");
			uart = bt && of_device_is_available(bt) &&
			       of_device_is_compatible(bt, "qcom,wcn3990-bt");
			of_node_put(bt);
			if (!uart)
				goto put_child;
		} else if (!strcmp(kbasename(child->full_name), "serial@a90000") &&
			   of_device_is_compatible(child, "qcom,geni-debug-uart")) {
			debug = true;
		} else {
			goto put_child;
		}
	}
	if (enabled != 2 || !uart || !debug)
		goto out;

	ret = of_overlay_fdt_apply(overlay_blob, sizeof(overlay_blob),
				   &overlay_id, NULL);
	if (!ret)
		pr_info("cepheus_qup0: enabled QUP0; BT and debug UART may probe; reboot to undo\n");
	goto out;
put_child:
	of_node_put(child);
out:
	of_node_put(qup);
	return ret;
}
module_init(cepheus_qup0_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Guarded cepheus QUP0 enablement; reboot-only rollback");
