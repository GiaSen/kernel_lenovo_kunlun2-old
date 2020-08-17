/*
 * platform indepent driver interface
 *
 * Coypritht (c) 2017 Goodix
 */
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>

#include "gf_spi.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

int gf_parse_dts(struct gf_dev *gf_dev)
{
	int rc = 0;
	struct device *dev = &gf_dev->spi->dev;
	struct device_node *np = dev->of_node;

	gf_dev->pwr_gpio = of_get_named_gpio(np, "fp-gpio-power", 0);
	if (gf_dev->pwr_gpio < 0) {
		pr_err("[GF_] falied to get pwr gpio!\n");
		return gf_dev->pwr_gpio;
	}

	rc = devm_gpio_request(dev, gf_dev->pwr_gpio, "goodix_power");
	if (rc) {
		pr_err("[GF_] failed to request pwr gpio, rc = %d\n", rc);
		goto err_pwr;
	}

	gpio_direction_output(gf_dev->pwr_gpio, 1);
	pr_info("[GF_] zhjh succeed to output pwr gpio! value=%d\n", gpio_get_value(gf_dev->pwr_gpio));

	gf_dev->reset_gpio = of_get_named_gpio(np, "fp-gpio-reset", 0);
	if (gf_dev->reset_gpio < 0) {
		pr_err("[GF_] falied to get reset gpio!\n");
		return gf_dev->reset_gpio;
	}

	rc = devm_gpio_request(dev, gf_dev->reset_gpio, "goodix_reset");
	if (rc) {
		pr_err("[GF_] failed to request reset gpio, rc = %d\n", rc);
		goto err_reset;
	}
	gpio_direction_output(gf_dev->reset_gpio, 1);

	gf_dev->irq_gpio = of_get_named_gpio(np, "fp-gpio-irq", 0);
	if (gf_dev->irq_gpio < 0) {
		pr_err("[GF_] falied to get irq gpio!\n");
		return gf_dev->irq_gpio;
	}

	rc = devm_gpio_request(dev, gf_dev->irq_gpio, "goodix_irq");
	if (rc) {
		pr_err("[GF_} failed to request irq gpio, rc = %d\n", rc);
		goto err_irq;
	}
	gpio_direction_input(gf_dev->irq_gpio);

    return rc;

err_irq:
	devm_gpio_free(dev, gf_dev->reset_gpio);
err_pwr:
err_reset:
	return rc;
}

void gf_cleanup(struct gf_dev *gf_dev)
{
	pr_info("[GF_info] %s\n", __func__);
	
	if (gpio_is_valid(gf_dev->pwr_gpio)) {
		gpio_free(gf_dev->pwr_gpio);
		pr_info("[GF_] remove pwr_gpio success\n");
	}
	if (gpio_is_valid(gf_dev->irq_gpio)) {
		gpio_free(gf_dev->irq_gpio);
		pr_info("[GF_] remove irq_gpio success\n");
	}
	if (gpio_is_valid(gf_dev->reset_gpio)) {
		gpio_free(gf_dev->reset_gpio);
		pr_info("[GF_] remove reset_gpio success\n");
	}
}

int gf_power_on(struct gf_dev *gf_dev)
{
	int rc = 0;

	/* TODO: add your power control here */
	if (gf_dev == NULL) {
		pr_info("[GF_] Input buff is NULL.\n");
		return -1;
	}
	//gpio_direction_output(gf_dev->pwr_gpio, 1);
	gpio_set_value(gf_dev->pwr_gpio, 1);

	return rc;
}

int gf_power_off(struct gf_dev *gf_dev)
{
	int rc = 0;

	/* TODO: add your power control here */

	return rc;
}

int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{
	if (gf_dev == NULL) {
		pr_info("[GF_] Input buff is NULL.\n");
		return -1;
	}
	gpio_direction_output(gf_dev->reset_gpio, 1);
	gpio_set_value(gf_dev->reset_gpio, 0);
	mdelay(3);
	gpio_set_value(gf_dev->reset_gpio, 1);
	mdelay(delay_ms);
	return 0;
}

int gf_irq_num(struct gf_dev *gf_dev)
{
	if (gf_dev == NULL) {
		pr_info("[GF_] Input buff is NULL.\n");
		return -1;
	} else {
		return gpio_to_irq(gf_dev->irq_gpio);
	}
}
