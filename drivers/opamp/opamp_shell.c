/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include <stdlib.h>

/* Mapped 1-1 to enum opamp_config */
static const char *const config_lookup[] = {
	"NONE",
	"FOO",
	"BAR",
};

static int get_device_from_str(const struct shell *sh,
			       const char *dev_str,
			       const struct device **dev)
{
	*dev = shell_device_get_binding(dev_str);

	if (*dev == NULL) {
		shell_error(sh, "%s not %s", dev_str, "found");
		return -ENODEV;
	}

	if (!device_is_ready(*dev)) {
		shell_error(sh, "%s not %s", dev_str, "ready");
		return -ENODEV;
	}

	return 0;
}

static int cmd_enable(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	const char *dev_str;
	const struct device *dev;

	ARG_UNUSED(argc);

	dev_str = argv[1];
	ret = get_device_from_str(sh, dev_str, &dev);
	if (ret < 0) {
		return ret;
	}

	ret = opamp_enable(dev);
	if (ret < 0) {
		shell_error(sh, "failed to %s", "enable");
		return -EIO;
	}

	shell_print(sh, "%i", ret);
	return 0;
}

static int cmd_disable(const struct shell *sh, size_t argc, char **argv)
{
	const char *dev_str;
	int ret;
	const struct device *dev;

	ARG_UNUSED(argc);

	dev_str = argv[1];
	ret = get_device_from_str(sh, dev_str, &dev);
	if (ret < 0) {
		return ret;
	}

	ret = opamp_disable(dev);
	if (ret < 0) {
		shell_error(sh, "failed to %s", "disable");
		return -EIO;
	}

	return 0;
}

static int get_gain_from_str(const struct shell *sh,
				const char *timeout_str,
				int *timeout)
{
	long gain;
	char *end;

	gain = strtol(timeout_str, &end, 10);
	if ((*end != '\0') || (gain < 1)) {
		shell_error(sh, "%s not %s", timeout_str, "valid");
		return -EINVAL;
	}

	*timeout = gain;
	return 0;
}

static int get_config_from_str(const struct shell *sh,
	const char *config_str, struct opamp_config *config)
{
	// ARRAY_FOR_EACH(config_lookup, i) {
	// 	if (strcmp(config_lookup[i], config_str) == 0) {
	// 		*trigger = (enum opamp_trigger)i;
	// 		return 0;
	// 	}
	// }

	shell_error(sh, "%s not %s", config_str, "valid");
	return -EINVAL;
}

static int cmd_configure(const struct shell *sh, size_t argc, char **argv)
{
	const char *dev_str;
	int ret;
	const struct device *dev;
	const char *config_str;
	struct opamp_config config;

	dev_str = argv[1];
	ret = get_device_from_str(sh, dev_str, &dev);
	if (ret < 0) {
		return ret;
	}

	config_str = argv[2];
	ret = get_config_from_str(sh, config_str, &config);
	if (ret < 0) {
		return ret;
	}

	ret = opamp_configure(dev, config);
	if (ret < 0) {
		shell_error(sh, "failed to %s", "configure");
		return -EIO;
	}

	return 0;
}

static bool device_is_opamp(const struct device *dev)
{
	return DEVICE_API_IS(opamp, dev);
}

static void dsub_configure_lookup_0(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_opamp);

	entry->syntax = dev != NULL ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_configure_0, dsub_configure_lookup_0);

static void dsub_device_lookup_0(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_opamp);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_0, dsub_device_lookup_0);

#define ENABLE_HELP \
	("comp enable <device>")

#define DISABLE_HELP \
	("comp disable <device>")

#define CONFIGURE_HELP	\
	("comp configure <device> <config>")

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_comp,
	SHELL_CMD_ARG(enable, &dsub_device_0, ENABLE_HELP, cmd_enable, 2, 0),
	SHELL_CMD_ARG(disable, &dsub_device_0, DISABLE_HELP, cmd_disable, 2, 0),
	SHELL_CMD_ARG(configure, &dsub_configure_0, CONFIGURE_HELP, cmd_configure, 2, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(comp, &sub_comp, "OpAmp device commands", NULL);
