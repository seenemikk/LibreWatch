#define DT_DRV_COMPAT galaxycore_gc9a01a

#include "gc9a01a.h"

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_gc9a01a);

#define GC9A01A_CMD_DATA(...)   .data = ((uint8_t[]){__VA_ARGS__}),                 \
                                .data_len = (sizeof(((uint8_t[]){__VA_ARGS__})))    \

struct gc9a01a_config {
    struct spi_dt_spec bus;
    struct gpio_dt_spec cmd_data_gpio;
    struct gpio_dt_spec reset_gpio;
    struct gpio_dt_spec backlight_gpio;
    uint8_t width;
    uint8_t height;
};

struct gc9a01a_cmd {
    size_t data_len;
    uint8_t *data;
    uint8_t cmd;
};

static struct gc9a01a_cmd gc9a01a_init_cmds[] = {
    { .cmd = 0xef },
    { .cmd = 0xeb, GC9A01A_CMD_DATA(0x14) },
    { .cmd = 0xfe },
    { .cmd = 0xef },
    { .cmd = 0xeb, GC9A01A_CMD_DATA(0x14) },
    { .cmd = 0x84, GC9A01A_CMD_DATA(0x40) },
    { .cmd = 0x85, GC9A01A_CMD_DATA(0xff) },
    { .cmd = 0x86, GC9A01A_CMD_DATA(0xff) },
    { .cmd = 0x87, GC9A01A_CMD_DATA(0xff) },
    { .cmd = 0x88, GC9A01A_CMD_DATA(0x0a) },
    { .cmd = 0x89, GC9A01A_CMD_DATA(0x21) },
    { .cmd = 0x8a, GC9A01A_CMD_DATA(0x00) },
    { .cmd = 0x8b, GC9A01A_CMD_DATA(0x80) },
    { .cmd = 0x8c, GC9A01A_CMD_DATA(0x01) },
    { .cmd = 0x8d, GC9A01A_CMD_DATA(0x01) },
    { .cmd = 0x8e, GC9A01A_CMD_DATA(0xff) },
    { .cmd = 0x8f, GC9A01A_CMD_DATA(0xff) },
    { .cmd = 0xb6, GC9A01A_CMD_DATA(0x00, 0x00) },
    { .cmd = 0x36, GC9A01A_CMD_DATA(0x48) },
    { .cmd = 0x3a, GC9A01A_CMD_DATA(0x05) },
    { .cmd = 0x90, GC9A01A_CMD_DATA(0x08, 0x08, 0x08, 0x08) },
    { .cmd = 0xbd, GC9A01A_CMD_DATA(0x06) },
    { .cmd = 0xbc, GC9A01A_CMD_DATA(0x00) },
    { .cmd = 0xff, GC9A01A_CMD_DATA(0x60, 0x01, 0x04) },
    { .cmd = 0xc3, GC9A01A_CMD_DATA(0x13) },
    { .cmd = 0xc4, GC9A01A_CMD_DATA(0x13) },
    { .cmd = 0xc9, GC9A01A_CMD_DATA(0x22) },
    { .cmd = 0xbe, GC9A01A_CMD_DATA(0x11) },
    { .cmd = 0xe1, GC9A01A_CMD_DATA(0x10, 0x0e) },
    { .cmd = 0xdf, GC9A01A_CMD_DATA(0x21, 0x0c, 0x02) },
    { .cmd = 0xf0, GC9A01A_CMD_DATA(0x45, 0x09, 0x08, 0x08, 0x26, 0x2a) },
    { .cmd = 0xf1, GC9A01A_CMD_DATA(0x43, 0x70, 0x72, 0x36, 0x37, 0x6f) },
    { .cmd = 0xf2, GC9A01A_CMD_DATA(0x45, 0x09, 0x08, 0x08, 0x26, 0x2a) },
    { .cmd = 0xf3, GC9A01A_CMD_DATA(0x43, 0x70, 0x72, 0x36, 0x37, 0x6f) },
    { .cmd = 0xed, GC9A01A_CMD_DATA(0x1b, 0x0b) },
    { .cmd = 0xae, GC9A01A_CMD_DATA(0x77) },
    { .cmd = 0xcd, GC9A01A_CMD_DATA(0x63) },
    { .cmd = 0x70, GC9A01A_CMD_DATA(0x07, 0x07, 0x04, 0x0e, 0x0f, 0x09, 0x07, 0x08, 0x03) },
    { .cmd = 0xe8, GC9A01A_CMD_DATA(0x34) },
    { .cmd = 0x62, GC9A01A_CMD_DATA(0x18, 0x0d, 0x71, 0xed, 0x70, 0x70, 0x18, 0x0f, 0x71, 0xef, 0x70, 0x70) },
    { .cmd = 0x63, GC9A01A_CMD_DATA(0x18, 0x11, 0x71, 0xf1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xf3, 0x70, 0x70) },
    { .cmd = 0x64, GC9A01A_CMD_DATA(0x28, 0x29, 0xf1, 0x01, 0xf1, 0x00, 0x07) },
    { .cmd = 0x66, GC9A01A_CMD_DATA(0x3c, 0x00, 0xcd, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00) },
    { .cmd = 0x67, GC9A01A_CMD_DATA(0x00, 0x3c, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98) },
    { .cmd = 0x74, GC9A01A_CMD_DATA(0x10, 0x85, 0x80, 0x00, 0x00, 0x4e, 0x00) },
    { .cmd = 0x98, GC9A01A_CMD_DATA(0x3e, 0x07) },
    { .cmd = 0x35 },
    { .cmd = 0x21 },
};

static int gc9a01a_transmit(const struct device *dev, struct gc9a01a_cmd *cmd)
{
    const struct gc9a01a_config *config = dev->config;
    int ret = 0;

    struct spi_buf buf = { .buf = &cmd->cmd, .len = 1 };
    struct spi_buf_set buf_set = { .buffers = &buf, .count = 1 };

    gpio_pin_set_dt(&config->cmd_data_gpio, 1);
    ret = spi_write_dt(&config->bus, &buf_set);
    if (ret) return ret;

    if (cmd->data == NULL || cmd->data_len == 0) return 0;

    buf.buf = cmd->data;
    buf.len = cmd->data_len;
    gpio_pin_set_dt(&config->cmd_data_gpio, 0);
    ret = spi_write_dt(&config->bus, &buf_set);

    return ret;
}

static int gc9a01a_blanking_on(const struct device *dev)
{
    const struct gc9a01a_config *config = dev->config;

    gpio_pin_set_dt(&config->backlight_gpio, 0);

    struct gc9a01a_cmd cmd = { .cmd = GC9A01A_CMD_DISP_OFF };
    return gc9a01a_transmit(dev, &cmd);
}

static int gc9a01a_blanking_off(const struct device *dev)
{
    const struct gc9a01a_config *config = dev->config;

    gpio_pin_set_dt(&config->backlight_gpio, 1);

    struct gc9a01a_cmd cmd = { .cmd = GC9A01A_CMD_DISP_ON };
    return gc9a01a_transmit(dev, &cmd);
}

static int gc9a01a_set_write_area(const struct device *dev, const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height)
{
    int ret = 0;

    uint16_t addr[2] = {0};
    addr[0] = sys_cpu_to_be16(x);
    addr[1] = sys_cpu_to_be16(x + width - 1);
    struct gc9a01a_cmd cmd = { .cmd = GC9A01A_CMD_COL_ADDR, .data = (uint8_t *)addr, .data_len = sizeof(addr) };
    ret = gc9a01a_transmit(dev, &cmd);
    if (ret) return ret;

    addr[0] = sys_cpu_to_be16(y);
    addr[1] = sys_cpu_to_be16(y + height - 1);
    cmd.cmd = GC9A01A_CMD_ROW_ADDR;
    ret = gc9a01a_transmit(dev, &cmd);

    return ret;
}

static int gc9a01a_write(const struct device *dev, const uint16_t x, const uint16_t y, const struct display_buffer_descriptor *desc, const void *buf)
{
    const struct gc9a01a_config *config = dev->config;
    int ret = 0;

    if (desc->pitch != desc->width) return -ENOTSUP;
    if (x + desc->width > config->width ||
        y + desc->height > config->height ||
        desc->width * desc->height * 2 > desc->buf_size) return -EINVAL;

    LOG_DBG("Writing (%dx%d) to (%dx%dx)", x, y, desc->width, desc->height);

    ret = gc9a01a_set_write_area(dev, x, y, desc->width, desc->height);
    if (ret) return ret;

    struct gc9a01a_cmd cmd = { .cmd = GC9A01A_CMD_MEM_WRITE, .data = buf, .data_len = desc->height * desc->width * 2 };
    ret = gc9a01a_transmit(dev, &cmd);

    return ret;
}

static int gc9a01a_read(const struct device *dev, const uint16_t x, const uint16_t y, const struct display_buffer_descriptor *desc, void *buf)
{
    return -ENOTSUP;
}

static void *gc9a01a_get_framebuffer(const struct device *dev)
{
    return NULL;
}

static int gc9a01a_set_brightness(const struct device *dev, const uint8_t brightness)
{
    // TODO
    return 0;
}

static int gc9a01a_set_contrast(const struct device *dev, const uint8_t contrast)
{
    return -ENOTSUP;
}

static void gc9a01a_get_capabilities(const struct device *dev, struct display_capabilities *capabilities)
{
    const struct gc9a01a_config *config = dev->config;

    memset(capabilities, 0, sizeof(*capabilities));
    capabilities->x_resolution = config->width;
    capabilities->y_resolution = config->height;

    capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
    capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;

    capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int gc9a01a_set_pixel_format(const struct device *dev, const enum display_pixel_format pixel_format)
{
    return -ENOTSUP;
}

static int gc9a01a_set_orientation(const struct device *dev, const enum display_orientation orientation)
{
    if (orientation == DISPLAY_ORIENTATION_NORMAL) return 0;

    LOG_ERR("Changing display orientation not implemented");
    return -ENOTSUP;
}

#ifdef CONFIG_PM_DEVICE

static void gc9a01_sleep_in(const struct device *dev)
{
    struct gc9a01a_cmd cmd = { .cmd = GC9A01A_CMD_SLEEP_IN };
    gc9a01a_transmit(dev, &cmd);
}

#endif // CONFIG_PM_DEVICE

static void gc9a01_sleep_out(const struct device *dev)
{
    struct gc9a01a_cmd cmd = { .cmd = GC9A01A_CMD_SLEEP_OUT };
    gc9a01a_transmit(dev, &cmd);
    k_sleep(K_MSEC(120));
}

static void gc9a01a_reset(const struct device *dev)
{
    const struct gc9a01a_config *config = dev->config;

    gpio_pin_set_dt(&config->reset_gpio, 1);
    k_sleep(K_MSEC(5));
    gpio_pin_set_dt(&config->reset_gpio, 0);
    k_sleep(K_MSEC(50));
}

static int gc9a01a_cmd_init(const struct device *dev)
{
    int ret = 0;

    for (int i = 0; i < sizeof(gc9a01a_init_cmds) / sizeof(*gc9a01a_init_cmds); i++) {
        ret = gc9a01a_transmit(dev, &gc9a01a_init_cmds[i]);
        if (ret) return ret;
    }

    return 0;
}

static int gc9a01a_init(const struct device *dev)
{
    const struct gc9a01a_config *config = dev->config;
    int ret = 0;

    if (!spi_is_ready(&config->bus)) {
        LOG_ERR("SPI not ready");
        return -ENODEV;
    }

    if (!device_is_ready(config->backlight_gpio.port)) {
        LOG_ERR("Backlight not ready");
        return -ENODEV;
    }
    if (gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_INACTIVE)) {
        LOG_ERR("Failed configuring backlight");
        return -EIO;
    }

    if (!device_is_ready(config->cmd_data_gpio.port)) {
        LOG_ERR("CMD GPIO not ready");
        return -ENODEV;
    }
    if (gpio_pin_configure_dt(&config->cmd_data_gpio, GPIO_OUTPUT)) {
        LOG_ERR("Failed configuring CMD GPIO");
        return -EIO;
    }

    if (!device_is_ready(config->reset_gpio.port)) {
        LOG_ERR("Reset GPIO not ready");
        return -ENODEV;
    }
    if (gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE)) {
        LOG_ERR("Failed configuring reset GPIO");
        return -EIO;
    }

    gc9a01a_reset(dev);

    ret = gc9a01a_cmd_init(dev);
    if (ret) return ret;

    gc9a01_sleep_out(dev);

    gc9a01a_blanking_on(dev);

    return 0;
}

#ifdef CONFIG_PM_DEVICE

static int gc9a01a_pm_action(const struct device *dev, enum pm_device_action action)
{
    switch (action) {
        case PM_DEVICE_ACTION_RESUME: {
            gc9a01_sleep_out(dev);
            gc9a01a_blanking_off(dev);
            break;
        }

        case PM_DEVICE_ACTION_SUSPEND: {
            gc9a01a_blanking_on(dev);
            gc9a01_sleep_in(dev);
            break;
        }

        default: {
            return -ENOTSUP;
        }
    }

    return 0;
}

#endif // CONFIG_PM_DEVICE

static const struct display_driver_api gc9a01a_api = {
    .blanking_on = gc9a01a_blanking_on,
    .blanking_off = gc9a01a_blanking_off,
    .write = gc9a01a_write,
    .read = gc9a01a_read,
    .get_framebuffer = gc9a01a_get_framebuffer,
    .set_brightness = gc9a01a_set_brightness,
    .set_contrast = gc9a01a_set_contrast,
    .get_capabilities = gc9a01a_get_capabilities,
    .set_pixel_format = gc9a01a_set_pixel_format,
    .set_orientation = gc9a01a_set_orientation,
};

#define GC9A01A_INIT(inst)                                                                                  \
    static const struct gc9a01a_config gc9a01a_config_ ## inst = {                                          \
        .bus = SPI_DT_SPEC_INST_GET(inst, (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8)), 0),    \
        .cmd_data_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, cmd_data_gpios, {}),                                \
        .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),                                      \
        .backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, backlight_gpios, {}),                              \
        .width = DT_INST_PROP(inst, width),                                                                 \
        .height = DT_INST_PROP(inst, height),                                                               \
    };                                                                                                      \
                                                                                                            \
    PM_DEVICE_DT_INST_DEFINE(inst, gc9a01a_pm_action);			                                            \
                                                                                                            \
    DEVICE_DT_INST_DEFINE(inst, gc9a01a_init, PM_DEVICE_DT_INST_GET(inst),                                  \
                          NULL, &gc9a01a_config_ ## inst,                                                   \
                          POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &gc9a01a_api);

DT_INST_FOREACH_STATUS_OKAY(GC9A01A_INIT)
