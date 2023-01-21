target_sources_ifdef(CONFIG_SMARTWATCH_PASSKEY app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/passkey_event.c)
target_sources_ifdef(CONFIG_SMARTWATCH_SENSORS app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sensors_event.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_event.c)
