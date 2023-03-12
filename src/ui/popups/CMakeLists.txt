target_sources_ifdef(CONFIG_SMARTWATCH_PASSKEY app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_passkey.c)
target_sources_ifdef(CONFIG_SMARTWATCH_ANCS app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_call.c)
target_sources_ifdef(CONFIG_SMARTWATCH_DFU app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_update.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_notification.c)
