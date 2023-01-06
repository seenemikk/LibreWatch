#define DT_DRV_COMPAT ite_it7259

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "it7259.h"

#define LOG_LEVEL CONFIG_KSCAN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kscan_it7259);

#define MAX_CMD_LEN     5
#define CMD_RETRY_CNT   5

#if CONFIG_IT7259_CB_COUNT < 1
#error "Max callback count cannot be less than 1"
#endif // CONFIG_IT7259_CB_COUNT < 1

struct it7259_config {
    struct i2c_dt_spec bus;
    struct gpio_dt_spec reset_gpio;
    struct gpio_dt_spec int_gpio;
};

struct it7259_data {
    const struct device *dev;
    struct k_work work;
    struct gpio_callback int_gpio_cb;
    kscan_callback_t cb[CONFIG_IT7259_CB_COUNT];
    atomic_t cb_enabled;
    bool pressed;
    uint8_t x;
    uint8_t y;
};

static int it7259_send_cmd(const struct device *dev, uint8_t *cmd, size_t cmd_len)
{
    if (cmd == NULL || cmd_len > MAX_CMD_LEN) return -EINVAL;

    const struct it7259_config *config = dev->config;
    int ret = 0;

    uint8_t tx[MAX_CMD_LEN + 1] = { IT7259_CMD_BUFFER };
    memcpy(&tx[1], cmd, cmd_len);

    pm_device_busy_set(dev);

    ret = i2c_write_dt(&config->bus, tx, cmd_len + 1);
    if (ret) goto end;

    tx[0] = IT7259_QUERY_BUFFER;
    uint8_t rx[1] = {0};
    for (int i = 0; i < CMD_RETRY_CNT; i++) {
        ret = i2c_write_read_dt(&config->bus, tx, 1, rx, sizeof(rx));
        if (ret == 0 && IT7259_QUERY_BUF_STATUS(rx[0]) == IT7259_QUERY_BUF_DONE) goto end;
        if (cmd[0] == IT7259_CMD_SET_PWR_MODE) goto end; // SET_PWR_MODE doesn't answer with an ACK, so we can just break

        k_sleep(K_MSEC(10));
    }
    ret = -EIO;

end:
    pm_device_busy_clear(dev);
    return ret;
}

static int it7259_get_cmd_resp(const struct device *dev, uint8_t *resp, size_t resp_len)
{
    if (resp == NULL) return -EINVAL;

    const struct it7259_config *config = dev->config;
    int ret = 0;

    pm_device_busy_set(dev);

    uint8_t tx[] = { IT7259_CMD_RESP_BUFFER };
    ret = i2c_write_read_dt(&config->bus, tx, sizeof(tx), resp, resp_len);

    pm_device_busy_clear(dev);

    return ret;
}

static void it7259_work_handler(struct k_work *work)
{
    struct it7259_data *data = CONTAINER_OF(work, struct it7259_data, work);
    const struct it7259_config *config = data->dev->config;

    pm_device_busy_set(data->dev);

    uint8_t point_data[0x0e] = {0};
    for (int i = 0; i < CMD_RETRY_CNT; i++) {
        if (i2c_write_read_dt(&config->bus, (uint8_t[]){ IT7259_POINT_INF_BUFFER }, 1, point_data, sizeof(point_data)) == 0) break;

        if (i == CMD_RETRY_CNT - 1) {
            LOG_ERR("Failed reading point data");
            goto end;
        }
        k_sleep(K_MSEC(10));
    }

    if (point_data[0] == 0) {
        data->pressed = false;
    } else if ((point_data[0] >> 4) == 0 && point_data[0] & 1) {
        data->x = point_data[2];
        data->y = point_data[4];
        data->pressed = true;
    } else {
        goto end;
    }

    LOG_DBG("%s @ (%d, %d)", data->pressed ? "Pressed" : "Released", data->x, data->y);

    if (!atomic_get(&data->cb_enabled)) goto end;

    for (int i = 0; i < CONFIG_IT7259_CB_COUNT; i++) {
        if (data->cb[i]) data->cb[i](data->dev, data->y, data->x, data->pressed);
    }

end:
    pm_device_busy_clear(data->dev);
}

static void it7259_int_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    struct it7259_data *data = CONTAINER_OF(cb, struct it7259_data, int_gpio_cb);

    k_work_submit(&data->work);
}

static int it7259_config(const struct device *dev, kscan_callback_t callback)
{
    struct it7259_data *data = dev->data;

    if (callback == NULL) {
        LOG_ERR("Invalid callback");
        return -EINVAL;
    }

    for (int i = 0; i < CONFIG_IT7259_CB_COUNT; i++) {
        if (data->cb[i] != NULL) continue;

        data->cb[i] = callback;
        return 0;
    }

    return -ENOSPC;
}

static int it7259_disable_callback(const struct device *dev)
{
    struct it7259_data *data = dev->data;

    atomic_set(&data->cb_enabled, false);

    return 0;
}

static int it7259_enable_callback(const struct device *dev)
{
    struct it7259_data *data = dev->data;

    atomic_set(&data->cb_enabled, true);

    return 0;
}

#ifdef CONFIG_PM_DEVICE

static void it7259_sleep_in(const struct device *dev)
{
    it7259_send_cmd(dev, (uint8_t[]){ IT7259_CMD_SET_PWR_MODE, IT7259_SUBCMD_SET_PWR_MODE, IT7259_PWR_IDLE }, 3);
}

#endif // CONFIG_PM_DEVICE

static void it7259_reset(const struct device *dev)
{
    const struct it7259_config *config = dev->config;

    gpio_pin_set_dt(&config->reset_gpio, 1);
    k_sleep(K_MSEC(5));
    gpio_pin_set_dt(&config->reset_gpio, 0);
    k_sleep(K_MSEC(105));
}


static int it7259_init(const struct device *dev)
{
    const struct it7259_config *config = dev->config;
    struct it7259_data *data = dev->data;
    int ret = 0;

    data->dev = dev;
    k_work_init(&data->work, it7259_work_handler);

    if (!device_is_ready(config->bus.bus)) {
        LOG_ERR("I2C not ready");
        return -ENODEV;
    }

    ret = pm_device_runtime_enable(dev);
    if ((ret != 0) && (ret != -ENOSYS)) {
        LOG_ERR("Failed enabling power management");
        return ret;
    }

    if (!device_is_ready(config->int_gpio.port)) {
        LOG_ERR("Interrupt GPIO not ready");
        return -ENODEV;
    }
    if ((ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT))) {
        LOG_ERR("Failed configuring interrupt GPIO");
        return ret;
    }
    if ((ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE))) {
        LOG_ERR("Failed configuring interrupt");
        return ret;
    }
    gpio_init_callback(&data->int_gpio_cb, it7259_int_handler, BIT(config->int_gpio.pin));
    if ((ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb))) {
        LOG_ERR("Failed adding interrupt callback");
        return ret;
    }

    if (!device_is_ready(config->reset_gpio.port)) {
        LOG_ERR("Reset GPIO not ready");
        return -ENODEV;
    }
    if ((ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE))) {
        LOG_ERR("Failed configuring reset GPIO");
        return ret;
    }

    it7259_reset(dev);

    uint8_t name[0x0a] = {0};
    if ((ret = it7259_send_cmd(dev, (uint8_t[]){ IT7259_CMD_DEVICE_NAME }, 1))) {
        LOG_ERR("Failed getting device name");
        return ret;
    }
    if ((ret = it7259_get_cmd_resp(dev, name, sizeof(name)))) {
        LOG_ERR("Failed getting device name");
        return ret;
    }
    if (!strstr(name, IT7259_DEVICE_NAME)) {
        LOG_ERR("Device name mismatch");
        return -ENODEV;
    }

    if ((ret = it7259_send_cmd(dev, (uint8_t[]){ IT7259_CMD_SET_CAP_INF, IT7259_SUBCMD_SET_INT, IT7259_INT_ENABLED, IT7259_INT_FALLING }, 4))) {
        LOG_ERR("Failed configuring device interrupt");
        return ret;
    }

    LOG_DBG("IT7259 Initialized");

    return 0;
}

#ifdef CONFIG_PM_DEVICE

static int it7259_pm_action(const struct device *dev, enum pm_device_action action)
{
    switch (action) {
        case PM_DEVICE_ACTION_SUSPEND: {
            it7259_sleep_in(dev);
            break;
        }

        case PM_DEVICE_ACTION_RESUME: {
            break;
        }

        default: {
            return -ENOTSUP;
        }
    }

    return 0;
}

#endif // CONFIG_PM_DEVICE

static const struct kscan_driver_api it7259_api = {
    .config = it7259_config,
    .disable_callback = it7259_disable_callback,
    .enable_callback = it7259_enable_callback,
};

#define IT7259_INIT(inst)                                                               \
    static const struct it7259_config it7259_config_ ## inst = {                        \
        .bus = I2C_DT_SPEC_INST_GET(inst),                                              \
        .reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),                  \
        .int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {}),                      \
    };                                                                                  \
                                                                                        \
    static struct it7259_data it7259_data_ ## inst;                                     \
                                                                                        \
    PM_DEVICE_DT_INST_DEFINE(inst, it7259_pm_action);                                   \
                                                                                        \
    DEVICE_DT_INST_DEFINE(inst, it7259_init, PM_DEVICE_DT_INST_GET(inst),               \
                          &it7259_data_ ## inst, &it7259_config_ ## inst,               \
                          POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY, &it7259_api);

DT_INST_FOREACH_STATUS_OKAY(IT7259_INIT)
