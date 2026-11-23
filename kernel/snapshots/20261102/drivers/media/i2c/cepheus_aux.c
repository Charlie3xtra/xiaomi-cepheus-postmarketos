// SPDX-License-Identifier: GPL-2.0
/* Cepheus auxiliary camera bring-up; mode tables recovered from vendor data. */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>

struct aux_reg { u16 addr, val; u8 width; u32 delay_us; };
#include "imx481-vendor.h"
#include "s5k3m5-vendor.h"
#include "s5k3t1-vendor.h"
struct aux_model {
	const char *name;
	u16 id_reg, id, width, height, line, frame;
	u32 code, pixel_rate, link_freq;
	const struct aux_reg *init, *mode;
	u32 ninit, nmode;
	bool sony;
};
static const struct aux_model models[] = {
	{ "imx481", 0x16, 0x481, 4656, 3496, 5120, 3550,
	  MEDIA_BUS_FMT_SRGGB10_1X10, 545461760, 654075000, imx481_init, imx481_mode,
	  ARRAY_SIZE(imx481_init), ARRAY_SIZE(imx481_mode), true },
	{ "s5k3m5", 0, 0x30d5, 4000, 3000, 4848, 3312,
	  MEDIA_BUS_FMT_SGRBG10_1X10, 481697280, 723210000, s5k3m5_init, s5k3m5_mode,
	  ARRAY_SIZE(s5k3m5_init), ARRAY_SIZE(s5k3m5_mode), false },
	{ "s5k3t1", 0, 0x3141, 2592, 1940, 13056, 2032,
	  MEDIA_BUS_FMT_SGRBG10_1X10, 797220250, 300000000, s5k3t1_init, s5k3t1_mode,
	  ARRAY_SIZE(s5k3t1_init), ARRAY_SIZE(s5k3t1_mode), false },
};
struct aux_sensor {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_ctrl_handler ctrls;
	struct mutex lock;
	struct i2c_client *client;
	const struct aux_model *m;
	struct clk *clk;
	struct gpio_desc *reset, *enable;
	struct regulator_bulk_data supplies[3];
	s64 link_freq;
	bool streaming;
};
static inline struct aux_sensor *to_aux(struct v4l2_subdev *sd)
{ return container_of(sd, struct aux_sensor, sd); }
static int aux_write(struct aux_sensor *a, u16 reg, u16 value, u8 width)
{
	u8 b[4] = { reg >> 8, reg & 255, 0, 0 };
	int ret;
	if (width == 1) b[2] = value;
	else { b[2] = value >> 8; b[3] = value; }
	ret = i2c_master_send(a->client, b, width + 2);
	return ret == width + 2 ? 0 : ret < 0 ? ret : -EIO;
}
static int aux_read_id(struct aux_sensor *a)
{
	u8 r[2] = { a->m->id_reg >> 8, a->m->id_reg & 255 }, b[2];
	struct i2c_msg msgs[] = {
		{ .addr = a->client->addr, .len = 2, .buf = r },
		{ .addr = a->client->addr, .flags = I2C_M_RD, .len = 2, .buf = b },
	};
	int ret = i2c_transfer(a->client->adapter, msgs, 2);
	if (ret != 2) return ret < 0 ? ret : -EIO;
	dev_info(&a->client->dev, "%s ID=%04x expected=%04x\n",
		 a->m->name, (b[0] << 8) | b[1], a->m->id);
	return ((b[0] << 8) | b[1]) == a->m->id ? 0 : -ENODEV;
}
static int aux_regs(struct aux_sensor *a, const struct aux_reg *r, u32 n)
{
	u32 i;
	int ret;
	for (i = 0; i < n; i++) {
		ret = aux_write(a, r[i].addr, r[i].val, r[i].width);
		if (ret) return dev_err_probe(&a->client->dev, ret, "write reg %04x failed\n", r[i].addr);
		if (r[i].delay_us) usleep_range(r[i].delay_us, r[i].delay_us + 100);
	}
	return 0;
}
static int aux_power_on(struct device *dev)
{
	struct aux_sensor *a = to_aux(dev_get_drvdata(dev));
	int ret = regulator_bulk_enable(3, a->supplies);
	if (ret) return ret;
	gpiod_set_value_cansleep(a->enable, 1);
	usleep_range(2000, 2500);
	ret = clk_prepare_enable(a->clk);
	if (ret) { gpiod_set_value_cansleep(a->enable, 0); regulator_bulk_disable(3, a->supplies); return ret; }
	usleep_range(1000, 1500);
	gpiod_set_value_cansleep(a->reset, 0);
	usleep_range(10000, 11000);
	return 0;
}
static int aux_power_off(struct device *dev)
{
	struct aux_sensor *a = to_aux(dev_get_drvdata(dev));
	gpiod_set_value_cansleep(a->reset, 1);
	clk_disable_unprepare(a->clk);
	gpiod_set_value_cansleep(a->enable, 0);
	regulator_bulk_disable(3, a->supplies);
	return 0;
}
static int aux_ctrl(struct v4l2_ctrl *ctrl)
{
	struct aux_sensor *a = container_of(ctrl->handler, struct aux_sensor, ctrls);
	int ret = 0;
	if (!pm_runtime_get_if_in_use(&a->client->dev)) return 0;
	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE: ret = aux_write(a, 0x0202, ctrl->val, 2); break;
	case V4L2_CID_ANALOGUE_GAIN: ret = aux_write(a, 0x0204, ctrl->val, 2); break;
	case V4L2_CID_VBLANK: ret = aux_write(a, 0x0340, a->m->height + ctrl->val, 2); break;
	case V4L2_CID_TEST_PATTERN: ret = aux_write(a, 0x0600, ctrl->val ? 2 : 0, 2); break;
	}
	pm_runtime_put(&a->client->dev);
	return ret;
}
static const struct v4l2_ctrl_ops aux_ctrl_ops = { .s_ctrl = aux_ctrl };
static const char * const aux_patterns[] = { "Disabled", "Color Bars" };
static int aux_stream(struct v4l2_subdev *sd, int on)
{
	struct aux_sensor *a = to_aux(sd);
	int ret = 0;
	mutex_lock(&a->lock);
	if (a->streaming == !!on) goto out;
	if (on) {
		ret = pm_runtime_resume_and_get(&a->client->dev);
		if (ret < 0) goto out;
		ret = aux_regs(a, a->m->init, a->m->ninit);
		if (!ret) ret = aux_regs(a, a->m->mode, a->m->nmode);
		if (!ret) ret = __v4l2_ctrl_handler_setup(&a->ctrls);
		if (!ret) ret = aux_write(a, 0x0100, a->m->sony ? 1 : 0x0100, a->m->sony ? 1 : 2);
		if (ret) { pm_runtime_put(&a->client->dev); goto out; }
		dev_info(&a->client->dev, "streaming %s %ux%u RAW10 four-lane D-PHY\n", a->m->name, a->m->width, a->m->height);
	} else {
		ret = aux_write(a, 0x0100, 0, a->m->sony ? 1 : 2);
		pm_runtime_put(&a->client->dev);
	}
	a->streaming = !!on;
out:
	mutex_unlock(&a->lock);
	return ret;
}
static void aux_format(struct aux_sensor *a, struct v4l2_mbus_framefmt *f)
{
	*f = (struct v4l2_mbus_framefmt) { .width = a->m->width, .height = a->m->height,
		.code = a->m->code, .field = V4L2_FIELD_NONE, .colorspace = V4L2_COLORSPACE_RAW,
		.xfer_func = V4L2_XFER_FUNC_NONE, .ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT,
		.quantization = V4L2_QUANTIZATION_FULL_RANGE };
}
static int aux_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_format *f)
{ if (f->pad) return -EINVAL; aux_format(to_aux(sd), &f->format); return 0; }
static int aux_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_format *f)
{ int ret = aux_get_fmt(sd, state, f); if (!ret && f->which == V4L2_SUBDEV_FORMAT_TRY) *v4l2_subdev_state_get_format(state, 0) = f->format; return ret; }
static int aux_enum_code(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_mbus_code_enum *c)
{ if (c->pad || c->index) return -EINVAL; c->code = to_aux(sd)->m->code; return 0; }
static int aux_enum_size(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_frame_size_enum *s)
{ struct aux_sensor *a=to_aux(sd); if (s->pad || s->index || s->code != a->m->code) return -EINVAL; s->min_width=s->max_width=a->m->width; s->min_height=s->max_height=a->m->height; return 0; }
static int aux_selection(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_selection *s)
{ struct aux_sensor *a=to_aux(sd); if (s->pad) return -EINVAL; switch(s->target) { case V4L2_SEL_TGT_CROP: case V4L2_SEL_TGT_CROP_DEFAULT: case V4L2_SEL_TGT_CROP_BOUNDS: case V4L2_SEL_TGT_NATIVE_SIZE: s->r=(struct v4l2_rect){0,0,a->m->width,a->m->height}; return 0; default: return -EINVAL; } }
static const struct v4l2_subdev_pad_ops aux_pad_ops = { .get_fmt=aux_get_fmt, .set_fmt=aux_set_fmt, .enum_mbus_code=aux_enum_code, .enum_frame_size=aux_enum_size, .get_selection=aux_selection };
static const struct v4l2_subdev_video_ops aux_video_ops = { .s_stream=aux_stream };
static const struct v4l2_subdev_ops aux_ops = { .pad=&aux_pad_ops, .video=&aux_video_ops };
static int aux_probe(struct i2c_client *client)
{
	struct device *dev=&client->dev;
	struct aux_sensor *a;
	struct v4l2_ctrl *c;
	struct v4l2_fwnode_device_properties props;
	int ret;
	a=devm_kzalloc(dev,sizeof(*a),GFP_KERNEL);
	if (!a) return -ENOMEM;
	a->m=i2c_get_match_data(client); a->client=client;
	mutex_init(&a->lock);
	v4l2_i2c_subdev_init(&a->sd,client,&aux_ops);
	/* Use the real model name for libcamera's sensor helper lookup. */
	snprintf(a->sd.name,sizeof(a->sd.name),"%s %d-%04x",a->m->name,i2c_adapter_id(client->adapter),client->addr);
	dev_info(dev,"probing %s\n",a->m->name);
	a->clk=devm_clk_get(dev,NULL); if (IS_ERR(a->clk)) return dev_err_probe(dev,PTR_ERR(a->clk),"clock get\n");
	ret=clk_set_rate(a->clk,19200000); if(ret) return dev_err_probe(dev,ret,"clock set\n");
	a->reset=devm_gpiod_get(dev,"reset",GPIOD_OUT_HIGH); if(IS_ERR(a->reset)) return dev_err_probe(dev,PTR_ERR(a->reset),"reset GPIO\n");
	a->enable=devm_gpiod_get(dev,"enable",GPIOD_OUT_LOW); if(IS_ERR(a->enable)) return dev_err_probe(dev,PTR_ERR(a->enable),"enable GPIO\n");
	a->supplies[0].supply="vio"; a->supplies[1].supply="vana"; a->supplies[2].supply="vdig";
	ret=devm_regulator_bulk_get(dev,3,a->supplies); if(ret) return dev_err_probe(dev,ret,"supplies\n");
	ret=aux_power_on(dev); if(ret) return dev_err_probe(dev,ret,"power on\n");
	ret=aux_read_id(a); if(ret) { dev_err_probe(dev,ret,"chip ID read\n"); goto poweroff; }
	v4l2_ctrl_handler_init(&a->ctrls,8); a->ctrls.lock=&a->lock;
	a->link_freq=a->m->link_freq;
	c=v4l2_ctrl_new_int_menu(&a->ctrls,NULL,V4L2_CID_LINK_FREQ,0,0,&a->link_freq); if(c) c->flags|=V4L2_CTRL_FLAG_READ_ONLY;
	v4l2_ctrl_new_std(&a->ctrls,NULL,V4L2_CID_PIXEL_RATE,a->m->pixel_rate,a->m->pixel_rate,1,a->m->pixel_rate);
	c=v4l2_ctrl_new_std(&a->ctrls,NULL,V4L2_CID_HBLANK,a->m->line-a->m->width,a->m->line-a->m->width,1,a->m->line-a->m->width); if(c) c->flags|=V4L2_CTRL_FLAG_READ_ONLY;
	v4l2_ctrl_new_std(&a->ctrls,&aux_ctrl_ops,V4L2_CID_VBLANK,a->m->frame-a->m->height,65535-a->m->height,1,a->m->frame-a->m->height);
	v4l2_ctrl_new_std(&a->ctrls,&aux_ctrl_ops,V4L2_CID_EXPOSURE,1,a->m->frame-20,1,a->m->frame-20);
	v4l2_ctrl_new_std(&a->ctrls,&aux_ctrl_ops,V4L2_CID_ANALOGUE_GAIN,a->m->sony?0:32,a->m->sony?960:512,1,a->m->sony?0:32);
	v4l2_ctrl_new_std_menu_items(&a->ctrls,&aux_ctrl_ops,V4L2_CID_TEST_PATTERN,1,0,0,aux_patterns);
	ret=v4l2_fwnode_device_parse(dev,&props); if(!ret) v4l2_ctrl_new_fwnode_properties(&a->ctrls,NULL,&props);
	ret=a->ctrls.error; if(ret) goto ctrlfree;
	a->sd.ctrl_handler=&a->ctrls; a->sd.flags|=V4L2_SUBDEV_FL_HAS_DEVNODE;
	a->sd.entity.function=MEDIA_ENT_F_CAM_SENSOR; a->pad.flags=MEDIA_PAD_FL_SOURCE;
	ret=media_entity_pads_init(&a->sd.entity,1,&a->pad); if(ret) goto ctrlfree;
	pm_runtime_set_active(dev); pm_runtime_enable(dev); pm_runtime_idle(dev);
	ret=v4l2_async_register_subdev_sensor(&a->sd);
	if(!ret) return 0;
	pm_runtime_disable(dev); media_entity_cleanup(&a->sd.entity);
ctrlfree:
	v4l2_ctrl_handler_free(&a->ctrls);
poweroff:
	aux_power_off(dev);
	return ret;
}
static void aux_remove(struct i2c_client *client)
{
	struct aux_sensor *a=to_aux(i2c_get_clientdata(client));
	v4l2_async_unregister_subdev(&a->sd); media_entity_cleanup(&a->sd.entity); v4l2_ctrl_handler_free(&a->ctrls);
	pm_runtime_disable(&client->dev); if(!pm_runtime_status_suspended(&client->dev)) aux_power_off(&client->dev);
	pm_runtime_set_suspended(&client->dev); mutex_destroy(&a->lock);
}
static const struct of_device_id aux_of_match[] = {
	{.compatible="sony,imx481",.data=&models[0]},
	{.compatible="samsung,s5k3m5",.data=&models[1]},
	{.compatible="samsung,s5k3t1",.data=&models[2]}, {}
};
MODULE_DEVICE_TABLE(of,aux_of_match);
static const struct dev_pm_ops aux_pm_ops = { SET_RUNTIME_PM_OPS(aux_power_off,aux_power_on,NULL) };
static struct i2c_driver aux_driver = { .driver={.name="cepheus_aux",.of_match_table=aux_of_match,.pm=&aux_pm_ops},.probe=aux_probe,.remove=aux_remove };
module_i2c_driver(aux_driver);
MODULE_DESCRIPTION("Cepheus auxiliary camera bring-up");
MODULE_LICENSE("GPL");
