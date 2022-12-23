#define DT_DRV_COMPAT invensense_icm42605_i2c

#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "icm42605.h"
#include "icm42605_regs.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_icm42605);

struct icm42605_config {
    struct i2c_dt_spec bus;
    struct gpio_dt_spec int1_gpio;
    struct gpio_dt_spec int2_gpio;
};

struct icm42605_data {
    const struct device *dev;
    struct k_work work;
    struct gpio_callback int_gpio_cb;
    sensor_trigger_handler_t trigger_handler;
    struct sensor_trigger trigger;
    uint16_t step_count;
};

struct icm42605_init_cmd {
    uint16_t reg;
    uint8_t val;
    uint8_t delay_ms;
};

static struct icm42605_init_cmd init_commands[] = {
    { .reg = DEVICE_CONFIG,     .val = 0x01, .delay_ms = 10 },      // Software reset
    { .reg = ACCEL_CONFIG0,     .val = 0x49 },                      // Accel ODR=50Hz, FSR=4g
    { .reg = PWR_MGMT0,         .val = 0x02 },                      // Accel Low Power mode & gyro off
    { .reg = APEX_CONFIG0,      .val = 0x02 },                      // DMP Power Save mode off & DMP_ODR=50Hz
    { .reg = INT_CONFIG,        .val = 0x12 },                      // INT1/INT2 Pulsed mode, Push pull, Active low
    { .reg = INT_CONFIG1,       .val = 0x00 },                      // INT1/INT2 Async reset
    { .reg = SIGNAL_PATH_RESET, .val = 0x20, .delay_ms = 10 },      // DMP memory reset
    { .reg = APEX_CONFIG5,      .val = 0x88, .delay_ms = 10 },      // Mounting matrix 0
    { .reg = SIGNAL_PATH_RESET, .val = 0x40 },                      // DMP enable
    { .reg = INT_SOURCE6,       .val = 0x06, .delay_ms = 10 },      // Route sleep & wake to INT1
    // { .reg = INT_SOURCE7,       .val = 0x06, .delay_ms = 10 },   // TODO maybe Wake on Motion instead?
    { .reg = APEX_CONFIG0,      .val = 0x2a, .delay_ms = 50 },      // Enable Pedometer & Wake/Sleep
};

static int icm42605_write(const struct device *dev, uint8_t bank, uint8_t reg, uint8_t val);

static int icm42605_set_bank(const struct device *dev, uint8_t bank)
{
    __ASSERT_NO_MSG(dev && (bank <= 2 || bank == 4));

    int ret = 0;
    static uint8_t current_bank;
    if (bank == current_bank) return 0;

    if ((ret = icm42605_write(dev, current_bank, REG(BANK_SEL), bank)) == 0) current_bank = bank;

    return ret;
}

static int icm42605_write(const struct device *dev, uint8_t bank, uint8_t reg, uint8_t val)
{
    __ASSERT_NO_MSG(dev);

    const struct icm42605_config *config = dev->config;
    int ret = 0;

    pm_device_busy_set(dev);

    ret = icm42605_set_bank(dev, bank);
    if (ret) goto end;

    uint8_t tx[] = { reg, val };
    ret = i2c_write_dt(&config->bus, tx, 2);

end:
    pm_device_busy_clear(dev);
    return ret;
}

static int icm42605_read(const struct device *dev, uint8_t bank, uint8_t reg, uint8_t *val)
{
    __ASSERT_NO_MSG(dev && val);

    const struct icm42605_config *config = dev->config;
    int ret = 0;

    pm_device_busy_set(dev);

    ret = icm42605_set_bank(dev, bank);
    if (ret) goto end;

    ret = i2c_write_read_dt(&config->bus, &reg, 1, val, 1);

end:
    pm_device_busy_clear(dev);
    return ret;
}

static void icm42605_work_handler(struct k_work *work)
{
    int ret = 0;
    struct icm42605_data *data = CONTAINER_OF(work, struct icm42605_data, work);

    pm_device_busy_set(data->dev);

    uint8_t isr = 0;
    if ((ret = icm42605_read(data->dev, BANK(INT_STATUS3), REG(INT_STATUS3), &isr))) {
        LOG_ERR("Failed to read interrupt status (%02x)", ret);
        goto end;
    }

    if (data->trigger_handler == NULL) goto end;

    if (isr & INT_STATUS3_WAKE_INT) {
        if (data->trigger.chan == SENSOR_CHAN_ALL || data->trigger.chan == SENSOR_CHAN_ICM42605_WAKE) {
            struct sensor_trigger trig = { .type = SENSOR_TRIG_ICM42605_R2W, SENSOR_CHAN_ICM42605_WAKE };
            data->trigger_handler(data->dev, &trig);
        }
    } else if (isr & INT_STATUS3_SLEEP_INT) {
        if (data->trigger.chan == SENSOR_CHAN_ALL || data->trigger.chan == SENSOR_CHAN_ICM42605_SLEEP) {
            struct sensor_trigger trig = { .type = SENSOR_TRIG_ICM42605_R2W, SENSOR_CHAN_ICM42605_SLEEP };
            data->trigger_handler(data->dev, &trig);
        }
    }

end:
    pm_device_busy_clear(data->dev);
}

static void icm42605_int_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    struct icm42605_data *data = CONTAINER_OF(cb, struct icm42605_data, int_gpio_cb);
    k_work_submit(&data->work);
}

static int icm42605_interrupt_init(const struct device *dev)
{
    const struct icm42605_config *config = dev->config;
    struct icm42605_data *data = dev->data;
    int ret = 0;

    if (!device_is_ready(config->int1_gpio.port)) {
        LOG_ERR("INT1 GPIO not ready");
        return -ENODEV;
    }
    if ((ret = gpio_pin_configure_dt(&config->int1_gpio, GPIO_INPUT))) {
        LOG_ERR("Failed configuring INT1 GPIO");
        return ret;
    }
    if ((ret = gpio_pin_interrupt_configure_dt(&config->int1_gpio, GPIO_INT_EDGE_TO_ACTIVE))) {
        LOG_ERR("Failed configuring INT1");
        return ret;
    }
    gpio_init_callback(&data->int_gpio_cb, icm42605_int_handler, BIT(config->int1_gpio.pin));
    if ((ret = gpio_add_callback(config->int1_gpio.port, &data->int_gpio_cb))) {
        LOG_ERR("Failed adding INT1 callback");
        return ret;
    }

    return 0;
}

static int icm42605_sensor_init(const struct device *dev) {
    int ret = 0;

    for (int i = 0; i < sizeof(init_commands) / sizeof(*init_commands); i++) {
        if ((ret = icm42605_write(dev, BANK(init_commands[i].reg), REG(init_commands[i].reg), init_commands[i].val))) return ret;
        if (init_commands[i].delay_ms) k_sleep(K_MSEC(init_commands[i].delay_ms));
    }

    return 0;
}

static int icm42605_init(const struct device *dev)
{
    const struct icm42605_config *config = dev->config;
    struct icm42605_data *data = dev->data;
    int ret = 0;

    data->dev = dev;
    k_work_init(&data->work, icm42605_work_handler);

    if (!device_is_ready(config->bus.bus)) {
        LOG_ERR("I2C not ready");
        return -ENODEV;
    }

    ret = pm_device_runtime_enable(dev);
    if ((ret != 0) && (ret != -ENOSYS)) {
        LOG_ERR("Failed enabling power management");
        return ret;
    }

    if ((ret = icm42605_interrupt_init(dev))) {
        LOG_ERR("Failed to enable interrupts");
        return ret;
    }

    if ((ret = icm42605_sensor_init(dev))) {
        LOG_ERR("Failed to configure sensor");
        return ret;
    }

    LOG_INF("ICM42605 Initialized");

    return 0;
}

#ifdef CONFIG_PM_DEVICE

static int icm42605_pm_action(const struct device *dev, enum pm_device_action action)
{
    switch (action) {
        case PM_DEVICE_ACTION_SUSPEND: {
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

static int icm42605_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    int ret = 0;
    uint16_t steps = 0;
    struct icm42605_data *data = dev->data;

    if ((ret = icm42605_read(dev, BANK(APEX_DATA0), REG(APEX_DATA0), &((uint8_t *)&steps)[0]))) goto err;
    if ((ret = icm42605_read(dev, BANK(APEX_DATA1), REG(APEX_DATA1), &((uint8_t *)&steps)[1]))) goto err;
    data->step_count = steps;

    return 0;

err:
    LOG_ERR("Failed reading step counter");
    return ret;
}

static int icm42605_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
    struct icm42605_data *data = dev->data;

    switch (chan) {
        case SENSOR_CHAN_ICM42605_STEPS: {
            val->val1 = data->step_count;
            break;
        }

        default: {
            return -ENOTSUP;
        }
    }

    return 0;
}

static int icm42605_trigger_set(const struct device *dev, const struct sensor_trigger *trig, sensor_trigger_handler_t handler)
{
    struct icm42605_data *data = dev->data;
    const struct icm42605_config *config = dev->config;

    if (data->trigger_handler) return -ENOSPC;
    if (trig == NULL || handler == NULL) return -EINVAL;
    if (trig->type != SENSOR_TRIG_ICM42605_R2W) return -ENOTSUP;
    if (trig->chan != SENSOR_CHAN_ALL && trig->chan != SENSOR_CHAN_ICM42605_WAKE && trig->chan != SENSOR_CHAN_ICM42605_SLEEP) return -ENOTSUP;

    gpio_pin_interrupt_configure_dt(&config->int1_gpio, GPIO_INT_DISABLE);

    data->trigger = *trig;
    data->trigger_handler = handler;

    gpio_pin_interrupt_configure_dt(&config->int1_gpio, GPIO_INT_EDGE_TO_ACTIVE);

    return 0;
}

static const struct sensor_driver_api icm42605_api = {
    .sample_fetch = icm42605_sample_fetch,
    .channel_get = icm42605_channel_get,
    .trigger_set = icm42605_trigger_set,
};

#define ICM42605_INIT(inst)                                                         \
    static const struct icm42605_config icm42605_config_ ## inst = {                \
        .bus = I2C_DT_SPEC_INST_GET(inst),                                          \
        .int1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {}),                \
        .int2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {}),                \
    };                                                                              \
                                                                                    \
    static struct icm42605_data icm42605_data_ ## inst;                             \
                                                                                    \
    PM_DEVICE_DT_INST_DEFINE(inst, icm42605_pm_action);                             \
                                                                                    \
    DEVICE_DT_INST_DEFINE(inst, icm42605_init, PM_DEVICE_DT_INST_GET(inst),         \
                          &icm42605_data_ ## inst, &icm42605_config_ ## inst,       \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &icm42605_api);

DT_INST_FOREACH_STATUS_OKAY(ICM42605_INIT)
