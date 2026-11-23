// SPDX-License-Identifier: GPL-2.0
/* Temporary stock NOM vote for the SM8150 camera controller at 847 MHz. */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <dt-bindings/power/qcom-rpmpd.h>

static struct device *camera_cc;
static unsigned int previous_state;

static int __init camera_nominal_init(void)
{
    struct platform_device *pdev;
    struct device_node *node;
    struct generic_pm_domain_data *pd;
    int ret;

    if (!of_machine_is_compatible("xiaomi,cepheus"))
        return -ENODEV;
    node = of_find_node_by_path("/soc@0/clock-controller@ad00000");
    if (!node)
        return -ENODEV;
    if (!of_device_is_compatible(node, "qcom,sm8150-camcc")) {
        of_node_put(node);
        return -EINVAL;
    }
    pdev = of_find_device_by_node(node);
    of_node_put(node);
    if (!pdev)
        return -ENODEV;
    camera_cc = &pdev->dev;
    ret = pm_runtime_resume_and_get(camera_cc);
    if (ret < 0)
        goto put_device;
    pd = dev_gpd_data(camera_cc);
    if (!pd) {
        ret = -ENODEV;
        goto put_runtime;
    }
    previous_state = pd->performance_state;
    if (previous_state > RPMH_REGULATOR_LEVEL_NOM) {
        ret = -ERANGE;
        goto put_runtime;
    }
    ret = dev_pm_genpd_set_performance_state(camera_cc, RPMH_REGULATOR_LEVEL_NOM);
    if (ret)
        goto put_runtime;
    pr_info("cepheus camera: temporary MMCX NOM vote %u -> %u\n",
            previous_state, RPMH_REGULATOR_LEVEL_NOM);
    return 0;
put_runtime:
    pm_runtime_put(camera_cc);
put_device:
    put_device(camera_cc);
    camera_cc = NULL;
    return ret;
}

static void __exit camera_nominal_exit(void)
{
    int ret = dev_pm_genpd_set_performance_state(camera_cc, previous_state);
    if (ret)
        pr_err("cepheus camera: restoring MMCX vote failed: %d\n", ret);
    pm_runtime_put(camera_cc);
    put_device(camera_cc);
}
module_init(camera_nominal_init);
module_exit(camera_nominal_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Reversible stock camera-controller MMCX performance vote");
