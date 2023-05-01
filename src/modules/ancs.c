#include <stdlib.h>

#include <zephyr/kernel.h>

#include <bluetooth/services/ancs_client.h>

#define MODULE ancs
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>

#include "discovery_event.h"
#include "notification_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_LIBREWATCH_ANCS_LOG_LEVEL);

#define ATTR_REQUEST_TIMEOUT_MS         5000U
#define NOTIF_ATTR_COUNT                ARRAY_SIZE(notif_attributes)
#define APP_ATTR_COUNT                  ARRAY_SIZE(app_attributes)

enum work_state_type {
    WORK_STATE_NONE,
    WORK_STATE_TIMEOUT,
    WORK_STATE_REQUEST,
    WORK_STATE_PERFORM_ACTION,
};

static uint8_t notif_attributes[] = {
    BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER,
    BT_ANCS_NOTIF_ATTR_ID_TITLE,
    BT_ANCS_NOTIF_ATTR_ID_MESSAGE,
};

static uint8_t app_attributes[] = {
    BT_ANCS_APP_ATTR_ID_DISPLAY_NAME,
};

static sys_slist_t notif_queue = SYS_SLIST_STATIC_INIT(&notification_queue);
static size_t notif_queue_len;

static uint8_t attr_buffer[MAX(CONFIG_LIBREWATCH_ANCS_MAX_TITLE_SIZE, CONFIG_LIBREWATCH_ANCS_MAX_MESSAGE_SIZE)];
static K_MUTEX_DEFINE(lock);
static struct bt_ancs_client ancs_c;

static enum work_state_type work_state;
static uint32_t action_uid;
static uint8_t action_type;
static void work_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(work, work_cb);

static void request_notif_attributes(void);
static void start_timeout_counter(void);
static void perform_notif_action(uint32_t uid, uint8_t action);
static void write_cb(struct bt_ancs_client *ancs_c, uint8_t err);

static struct notification_event *get_notif_from_node(sys_snode_t *node)
{
    if (node == NULL) return NULL;
    struct app_event_header *header = CONTAINER_OF(node, struct app_event_header, node);
    return CONTAINER_OF(header, struct notification_event, header);
}

static struct notification_event *get_notif_head(void)
{
    k_mutex_lock(&lock, K_FOREVER);

    sys_snode_t *node = sys_slist_get(&notif_queue);
    struct notification_event *notif = get_notif_from_node(node);
    if (notif != NULL) {
        if (notif_queue_len == 0) {
            LOG_ERR("Notification queue length unsync");
        } else {
            notif_queue_len--;
        }
    }

    k_mutex_unlock(&lock);
    return notif;
}

static struct notification_event *peek_notif_head(void)
{
    k_mutex_lock(&lock, K_FOREVER);

    sys_snode_t *node = sys_slist_peek_head(&notif_queue);
    struct notification_event *notif = get_notif_from_node(node);

    k_mutex_unlock(&lock);
    return notif;
}

static void free_notif_head(void)
{
    k_mutex_lock(&lock, K_FOREVER);

    struct notification_event *event = get_notif_head();
    app_event_manager_free(event);

    k_mutex_unlock(&lock);
}

static void free_notifications(void)
{
    k_mutex_lock(&lock, K_FOREVER);

    struct notification_event *notif = NULL;
    while ((notif = get_notif_head()) != NULL) app_event_manager_free(notif);

    k_mutex_unlock(&lock);
}

static void append_notif(const struct bt_ancs_evt_notif *notif)
{
    struct notification_event *event = new_notification_event();
    if (event == NULL) return;

    event->info = *notif;
    event->app_id[0] = 0;
    event->app_name[0] = 0;
    event->title[0] = 0;
    event->message[0] = 0;
    event->pending_app_attributes = APP_ATTR_COUNT;
    event->pending_notif_attributes = NOTIF_ATTR_COUNT;

    k_mutex_lock(&lock, K_FOREVER);

    if (notif_queue_len < CONFIG_LIBREWATCH_ANCS_MAX_NOTIFICATIONS) {
        sys_slist_append(&notif_queue, &event->header.node);
        notif_queue_len++;
    } else {
        LOG_WRN("Number of active notifications exceeded allowed count");
        app_event_manager_free(event);
    }

    k_mutex_unlock(&lock);
}

static void send_notif(void)
{
    struct notification_event *notif = get_notif_head();
    if (notif != NULL) APP_EVENT_SUBMIT(notif);
}

static void work_state_request(void)
{
    struct notification_event *notif = NULL;
    while ((notif = peek_notif_head()) != NULL) {
        if (notif->info.evt_id != BT_ANCS_EVENT_ID_NOTIFICATION_REMOVED &&
           (notif->pending_app_attributes != 0 || notif->pending_notif_attributes != 0)) {
            break;
        }
        send_notif();
    }
    if (notif == NULL) return;

    int err = 0;
    if (notif->pending_notif_attributes == 0) {
        err = bt_ancs_request_app_attr(&ancs_c, notif->app_id, strlen(notif->app_id), write_cb);
    } else {
        if (notif->pending_notif_attributes != NOTIF_ATTR_COUNT) return;
        err = bt_ancs_request_attrs(&ancs_c, &notif->info, write_cb);
    }

    if (err) {
        LOG_ERR("Failed to request attributes (%d)", err);
        if (err != -EBUSY) {
            // Some error with the request, move onto the next notification
            free_notif_head();
            request_notif_attributes();
        }
        return;
    }

    start_timeout_counter();
}

static void work_state_timeout(void)
{
    LOG_ERR("Notification timed out");
    free_notif_head();
    request_notif_attributes();
}

static void notification_action_cb(struct bt_ancs_client *ancs_c, uint8_t err)
{
    request_notif_attributes();
}

static void work_state_perform_action(void)
{
    int ret = bt_ancs_notification_action(&ancs_c, action_uid, action_type, notification_action_cb);
    if (ret == -EBUSY) perform_notif_action(action_uid, action_type);
}

static void work_cb(struct k_work *work)
{
    k_mutex_lock(&lock, K_FOREVER);

    int state = work_state;
    work_state = WORK_STATE_NONE;

    switch (state) {
        case WORK_STATE_REQUEST: {
            work_state_request();
            break;
        }

        case WORK_STATE_TIMEOUT: {
            work_state_timeout();
            break;
        }

        case WORK_STATE_PERFORM_ACTION: {
            work_state_perform_action();
            break;
        }

        case WORK_STATE_NONE:
        default: {
            LOG_ERR("Invalid work state");
        }
    }

    k_mutex_unlock(&lock);
}

static void request_notif_attributes(void)
{
    k_mutex_lock(&lock, K_FOREVER);

    if (work_state != WORK_STATE_NONE) goto unlock;

    int ret = k_work_reschedule(&work, K_NO_WAIT);
    if (ret == 0) {
        LOG_ERR("Work already queued");
    } else if (ret < 0) {
        LOG_ERR("Failed to add work to queue (%d)", ret);
    } else {
        work_state = WORK_STATE_REQUEST;
    }

unlock:
    k_mutex_unlock(&lock);
}

static void start_timeout_counter(void)
{
    k_mutex_lock(&lock, K_FOREVER);

    if (work_state != WORK_STATE_NONE) goto unlock;

    k_work_reschedule(&work, K_MSEC(ATTR_REQUEST_TIMEOUT_MS));
    work_state = WORK_STATE_TIMEOUT;

unlock:
    k_mutex_unlock(&lock);
}

static void perform_notif_action(uint32_t uid, uint8_t action)
{
    if (action >= NOTIFICATION_ACTION_COUNT) return;

    k_mutex_lock(&lock, K_FOREVER);

    if (work_state != WORK_STATE_NONE) goto unlock;

    k_work_reschedule(&work, K_NO_WAIT);
    work_state = WORK_STATE_PERFORM_ACTION;
    action_uid = uid;
    action_type = action;

unlock:
    k_mutex_unlock(&lock);
}

static void write_cb(struct bt_ancs_client *ancs_c, uint8_t err)
{
    switch (err) {
        case BT_ATT_ERR_ANCS_NP_UNKNOWN_COMMAND: {
            LOG_ERR("Unknown command ID");
            break;
        }

        case BT_ATT_ERR_ANCS_NP_INVALID_COMMAND: {
            LOG_ERR("Invalid command format");
            break;
        }

        case BT_ATT_ERR_ANCS_NP_INVALID_PARAMETER: {
            LOG_ERR("Invalid parameters");
            break;
        }

        case BT_ATT_ERR_ANCS_NP_ACTION_FAILED: {
            LOG_ERR("Failed to perform action");
            break;
        }

        default: {
            return;
        }
    }

    request_notif_attributes();
}

static void notification_source_cb(struct bt_ancs_client *ancs, int err, const struct bt_ancs_evt_notif *notif)
{
    if (err) return;

    append_notif(notif);
    request_notif_attributes();
}

static void data_source_cb(struct bt_ancs_client *ancs, const struct bt_ancs_attr_response *response)
{
    k_work_cancel_delayable(&work);

    k_mutex_lock(&lock, K_FOREVER);

    work_state = WORK_STATE_NONE;

    struct notification_event *notif = peek_notif_head();
    if (notif == NULL) {
        LOG_ERR("Received notification attribute, but there are no notification in the queue");
        goto unlock;
    }

    if (notif->info.notif_uid != response->notif_uid) {
        LOG_ERR("Received notification attribute for an unexpected notification");
        goto request;
    }

    switch (response->command_id) {
        case BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES: {
            if (notif->pending_notif_attributes == 0) {
                LOG_ERR("Did not expect to receive notification attribute");
                goto request;
            }

            switch (response->attr.attr_id) {
                case BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER: {
                    snprintk(notif->app_id, MIN(sizeof(notif->app_id), response->attr.attr_len + 1), "%s", response->attr.attr_data);

                    // If we request app attributes and the app id in the response doesn't fit into response->app_id
                    // then the response is ignored and we don't receive data source callback - ancs_attr_parser.c : app_id_parse
                    // So to prevent that from happening, we don't request app attributes if app_id doesn't fit.
                    if (response->attr.attr_len >= sizeof(response->app_id) || response->attr.attr_len == 0) {
                        notif->pending_app_attributes = 0;
                    }
                    break;
                }

                case BT_ANCS_NOTIF_ATTR_ID_TITLE: {
                    snprintk(notif->title, MIN(sizeof(notif->title), response->attr.attr_len + 1), "%s", response->attr.attr_data);
                    break;
                }

                case BT_ANCS_NOTIF_ATTR_ID_MESSAGE: {
                    snprintk(notif->message, MIN(sizeof(notif->message), response->attr.attr_len + 1), "%s", response->attr.attr_data);
                    break;
                }

                default: {
                    LOG_ERR("Received unexpected notification attribute (%d)", response->attr.attr_id);
                    goto request;
                }
            }

            notif->pending_notif_attributes--;
            break;
        }

        case BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES: {
            if (notif->pending_app_attributes == 0) {
                LOG_ERR("Did not expect to receive app attribute");
                goto request;
            }

            if (response->attr.attr_id != BT_ANCS_APP_ATTR_ID_DISPLAY_NAME) {
                LOG_ERR("Received unexpected app attribute (%d)", response->attr.attr_id);
                goto request;
            }

            snprintk(notif->app_name, MIN(sizeof(notif->app_name), response->attr.attr_len + 1), "%s", response->attr.attr_data);
            notif->pending_app_attributes = 0;
            break;
        }

        default: {
            break;
        }
    }

request:
    request_notif_attributes();
unlock:
    k_mutex_unlock(&lock);
}

static void init(void)
{
    k_work_cancel_delayable(&work);
    free_notifications();
    bt_ancs_client_init(&ancs_c);
    work_state = WORK_STATE_NONE;
}

static void discovery_completed(struct bt_gatt_dm *dm)
{
    init();

    int err = bt_ancs_handles_assign(dm, &ancs_c);
    if (err) {
        LOG_ERR("Failed to assign ANCS client handles (%d)", err);
        return;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(notif_attributes); i++) {
        err = bt_ancs_register_attr(&ancs_c, notif_attributes[i], attr_buffer, sizeof(attr_buffer));
        if (err) {
            LOG_ERR("Failed to register notification attributes (%d)", err);
            return;
        }
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(app_attributes); i++) {
        err = bt_ancs_register_app_attr(&ancs_c, app_attributes[i], attr_buffer, sizeof(attr_buffer));
        if (err) {
            LOG_ERR("Failed to register app attributes (%d)", err);
            return;
        }
    }

    err = bt_ancs_subscribe_notification_source(&ancs_c, notification_source_cb);
	if (err) {
		LOG_ERR("Failed to subscribe to notification source (%d)", err);
        return;
	}

	err = bt_ancs_subscribe_data_source(&ancs_c, data_source_cb);
	if (err) {
		LOG_ERR("Failed to subscribe to data source (%d)", err);
        return;
	}
}

static void handle_ble_peer_event(struct ble_peer_event *evt)
{
    if (evt->state != PEER_STATE_DISCONNECTED) return;
    init();
}

static void handle_discovery_event(struct discovery_event *event)
{
    if (event->type != DISCOVERY_EVENT_ANCS) return;
    discovery_completed(event->gatt_dm);
}

static void handle_notification_action_event(struct notification_action_event *event)
{
    perform_notif_action(event->uid, event->action);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_ble_peer_event(aeh)) {
        handle_ble_peer_event(cast_ble_peer_event(aeh));
        return false;
    }

    if (is_discovery_event(aeh)) {
        handle_discovery_event(cast_discovery_event(aeh));
        return false;
    }

    if (is_notification_action_event(aeh)) {
        handle_notification_action_event(cast_notification_action_event(aeh));
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event_event);
APP_EVENT_SUBSCRIBE(MODULE, discovery_event);
APP_EVENT_SUBSCRIBE(MODULE, notification_action_event);
