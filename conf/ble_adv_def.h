#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>

const struct {} ble_adv_def_include_once;

static const struct bt_data ad_unbonded[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                 (CONFIG_BT_DEVICE_APPEARANCE & BIT_MASK(__CHAR_BIT__)),
                 (CONFIG_BT_DEVICE_APPEARANCE >> __CHAR_BIT__)),
};

static const struct bt_data ad_bonded[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                 (CONFIG_BT_DEVICE_APPEARANCE & BIT_MASK(__CHAR_BIT__)),
                 (CONFIG_BT_DEVICE_APPEARANCE >> __CHAR_BIT__)),
};

static const struct bt_data sd[] = {};
