target_sources_ifdef(CONFIG_LIBREWATCH_PASSKEY app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_passkey.c)
target_sources_ifdef(CONFIG_LIBREWATCH_ANCS app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_call.c)
target_sources_ifdef(CONFIG_LIBREWATCH_DFU app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_update.c)
target_sources_ifdef(CONFIG_LIBREWATCH_ANCS app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_popup_notification.c)
