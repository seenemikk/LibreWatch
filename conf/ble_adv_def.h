#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/services/ams_client.h>

const struct {} ble_adv_def_include_once;

static const struct bt_data ad_unbonded[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                 (CONFIG_BT_DEVICE_APPEARANCE & BIT_MASK(__CHAR_BIT__)),
                 (CONFIG_BT_DEVICE_APPEARANCE >> __CHAR_BIT__)),
#if IS_ENABLED(CONFIG_SMARTWATCH_AMS)
    BT_DATA_BYTES(BT_DATA_SOLICIT128, BT_UUID_AMS_VAL),
#endif // IS_ENABLED(CONFIG_SMARTWATCH_AMS)
};

static const struct bt_data ad_bonded[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                 (CONFIG_BT_DEVICE_APPEARANCE & BIT_MASK(__CHAR_BIT__)),
                 (CONFIG_BT_DEVICE_APPEARANCE >> __CHAR_BIT__)),
#if IS_ENABLED(CONFIG_SMARTWATCH_AMS)
    BT_DATA_BYTES(BT_DATA_SOLICIT128, BT_UUID_AMS_VAL),
#endif // IS_ENABLED(CONFIG_SMARTWATCH_AMS)
};

static const struct bt_data sd[] = {};
