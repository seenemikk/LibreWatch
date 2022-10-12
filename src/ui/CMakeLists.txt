zephyr_linker_sources(DATA_SECTIONS iterables.ld)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui.c)

add_subdirectory(popups)
add_subdirectory(apps)
