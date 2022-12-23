zephyr_library()
zephyr_library_sources(icm42605.c)
target_include_directories(zephyr_interface BEFORE INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
