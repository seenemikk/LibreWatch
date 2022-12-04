target_sources_ifdef(CONFIG_SMARTWATCH_BLE_PASSKEY app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_passkey.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_notification.c)
