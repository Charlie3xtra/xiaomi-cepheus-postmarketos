// SPDX-License-Identifier: GPL-2.0-only
/* Cepheus vendor-routed BT experiment. Reboot-only rollback, no module_exit. */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include "overlay_blob.h"

#define OLD_QUP "/soc@0/geniqup@ac0000"
#define OLD_UART OLD_QUP "/serial@a84000"
#define NEW_QUP "/soc@0/geniqup@cc0000"
#define NEW_UART NEW_QUP "/serial@c8c000"
static int overlay_id = -1;

static bool created(struct device_node *np)
{
	struct platform_device *pdev = of_find_device_by_node(np);

	if (!pdev)
		return false;
	put_device(&pdev->dev);
	return true;
}

static bool check_node(const char *path, const char *compat, bool available,
		       bool populated)
{
	struct device_node *np = of_find_node_by_path(path);
	bool valid;

	if (!np)
		return false;
	valid = of_device_is_compatible(np, compat) &&
		of_device_is_available(np) == available &&
		created(np) == populated;
	of_node_put(np);
	return valid;
}

static bool check_supplies(void)
{
	static const char * const props[] = {
		"vddio-supply", "vddxo-supply", "vddrf-supply", "vddch0-supply",
	};
	static const char * const names[] = {
		"pm8150_l1a_0p75", "pm8150_l7a_1p8",
		"pm8150l_l2c_1p3", "pm8150l_l11c_3p3",
	};
	struct device_node *bt, *reg;
	const char *name;
	bool valid = true;
	u32 val;
	int i;

	bt = of_find_node_by_path(OLD_UART "/bluetooth");
	if (!bt || !of_device_is_compatible(bt, "qcom,wcn3990-bt") ||
	    !of_device_is_available(bt)) {
		valid = false;
		goto out;
	}
	for (i = 0; i < ARRAY_SIZE(props); i++) {
		if (of_property_read_u32(bt, props[i], &val) || val != 0x44 + i) {
			valid = false;
			break;
		}
		reg = of_parse_phandle(bt, props[i], 0);
		if (!reg || of_property_read_string(reg, "regulator-name", &name) ||
		    strcmp(name, names[i]))
			valid = false;
		of_node_put(reg);
		if (!valid)
			break;
	}
	if (of_property_read_u32(bt, "max-speed", &val) || val != 3200000)
		valid = false;
out:
	of_node_put(bt);
	return valid;
}

static int __init cepheus_uart13_init(void)
{
	struct device_node *np, *bt;
	bool valid;
	int ret;

	if (!of_machine_is_compatible("xiaomi,cepheus"))
		return -ENODEV;
	if (!check_node(OLD_QUP, "qcom,geni-se-qup", false, false) ||
	    !check_node(OLD_UART, "qcom,geni-uart", true, false) ||
	    !check_node(NEW_QUP, "qcom,geni-se-qup", true, true) ||
	    !check_node(NEW_UART, "qcom,geni-uart", false, false) ||
	    !check_node(NEW_QUP "/i2c@c8c000", "qcom,geni-i2c", false, false) ||
	    !check_node(NEW_QUP "/spi@c8c000", "qcom,geni-spi", false, false) ||
	    !check_supplies())
		return -EINVAL;
	np = of_find_node_by_path(NEW_UART);
	if (!np)
		return -ENODEV;
	bt = of_get_child_by_name(np, "bluetooth");
	valid = !bt && of_node_check_flag(np->parent, OF_POPULATED_BUS);
	of_node_put(bt);
	of_node_put(np);
	if (!valid)
		return -EINVAL;

	ret = of_overlay_fdt_apply(overlay_blob, sizeof(overlay_blob),
				   &overlay_id, NULL);
	if (!ret)
		pr_info("cepheus_uart13: vendor route enabled; HCI functionality unverified; reboot to undo\n");
	return ret;
}
module_init(cepheus_uart13_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Guarded cepheus vendor UART13 Bluetooth candidate");
